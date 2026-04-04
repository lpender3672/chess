#include "movegen.h"
#include "bitboard.h"
#include "magic.h"
#include <string.h>

/* ── Move encoding helpers ─────────────────────────────────────────────── */
#define FLAG_QUIET      0x00
#define FLAG_DOUBLE_PP  0x01
#define FLAG_CASTLE     0x02
#define FLAG_EP         0x03
#define FLAG_CAPTURE    0x10
#define FLAG_PROMO      0x20
#define FLAG_PROMO_CAP  0x30

static inline void push(MoveList *list, Square from, Square to, uint32_t flags) {
    list->moves[list->count++] = MK_MOVE(from, to, flags);
}

static void gen_promotions(MoveList *list, Square from, Square to, int capture) {
    uint32_t base = capture ? FLAG_PROMO_CAP : FLAG_PROMO;
    push(list, from, to, base | (QUEEN  - KNIGHT));
    push(list, from, to, base | (ROOK   - KNIGHT));
    push(list, from, to, base | (BISHOP - KNIGHT));
    push(list, from, to, base | 0);  /* knight promotion */
}

/* ── Core generation ───────────────────────────────────────────────────── */
int movegen_generate(const Position *pos, MoveList *list, GenType type) {
    list->count = 0;
    Color  us  = pos->side;
    Color  them = us ^ 1;
    Bitboard occ    = pos->occupied;
    Bitboard ours   = pos->by_color[us];
    Bitboard theirs = pos->by_color[them];
    Bitboard empty  = ~occ;

    /* ── Pawns ── */
    Bitboard pawns = pos->pieces[us][PAWN];
    int push_dir   = (us == WHITE) ? 8 : -8;
    Bitboard promo_rank = (us == WHITE) ? RANK_BB[7] : RANK_BB[0];
    Bitboard start_rank = (us == WHITE) ? RANK_BB[1] : RANK_BB[6];

    if (type == GEN_ALL) {
        /* Single pushes */
        Bitboard single = (us == WHITE ? pawns << 8 : pawns >> 8) & empty;
        Bitboard promos = single & promo_rank;
        single &= ~promo_rank;
        Bitboard tmp = single;
        while (tmp) { Square to = bb_pop(&tmp); push(list, (Square)(to - push_dir), to, FLAG_QUIET); }
        while (promos) { Square to = bb_pop(&promos); gen_promotions(list, (Square)(to - push_dir), to, 0); }

        /* Double pushes — pawns that single-pushed from their start rank */
        Bitboard dbl = (us == WHITE ? (single & (start_rank << 8)) << 8
                                    : (single & (start_rank >> 8)) >> 8) & empty;
        while (dbl) { Square to = bb_pop(&dbl); push(list, (Square)(to - 2 * push_dir), to, FLAG_DOUBLE_PP); }
    }

    /* Captures */
    Bitboard att_left  = (us == WHITE ? (pawns & ~FILE_BB[0]) << 7 : (pawns & ~FILE_BB[0]) >> 9) & theirs;
    Bitboard att_right = (us == WHITE ? (pawns & ~FILE_BB[7]) << 9 : (pawns & ~FILE_BB[7]) >> 7) & theirs;
    Bitboard cap_promo_l = att_left & promo_rank; att_left &= ~promo_rank;
    Bitboard cap_promo_r = att_right & promo_rank; att_right &= ~promo_rank;
    int cap_dir_l = (us == WHITE) ? 7 : -9;
    int cap_dir_r = (us == WHITE) ? 9 : -7;
    while (att_left)  { Square to = bb_pop(&att_left);  push(list, (Square)(to - cap_dir_l), to, FLAG_CAPTURE); }
    while (att_right) { Square to = bb_pop(&att_right); push(list, (Square)(to - cap_dir_r), to, FLAG_CAPTURE); }
    while (cap_promo_l) { Square to = bb_pop(&cap_promo_l); gen_promotions(list, (Square)(to - cap_dir_l), to, 1); }
    while (cap_promo_r) { Square to = bb_pop(&cap_promo_r); gen_promotions(list, (Square)(to - cap_dir_r), to, 1); }

    /* En passant */
    if (pos->ep_square != SQ_NONE) {
        Bitboard ep_att = pawn_attacks[them][pos->ep_square] & pawns;
        while (ep_att) { Square from = bb_pop(&ep_att); push(list, from, pos->ep_square, FLAG_EP); }
    }

    /* ── Knights ── */
    Bitboard knights = pos->pieces[us][KNIGHT];
    while (knights) {
        Square from = bb_pop(&knights);
        Bitboard targets = knight_attacks[from] & (type == GEN_ALL ? ~ours : theirs);
        while (targets) {
            Square to = bb_pop(&targets);
            push(list, from, to, (theirs & sq_bb(to)) ? FLAG_CAPTURE : FLAG_QUIET);
        }
    }

    /* ── Bishops ── */
    Bitboard bishops = pos->pieces[us][BISHOP];
    while (bishops) {
        Square from = bb_pop(&bishops);
        Bitboard targets = bishop_attacks(from, occ) & (type == GEN_ALL ? ~ours : theirs);
        while (targets) {
            Square to = bb_pop(&targets);
            push(list, from, to, (theirs & sq_bb(to)) ? FLAG_CAPTURE : FLAG_QUIET);
        }
    }

    /* ── Rooks ── */
    Bitboard rooks = pos->pieces[us][ROOK];
    while (rooks) {
        Square from = bb_pop(&rooks);
        Bitboard targets = rook_attacks(from, occ) & (type == GEN_ALL ? ~ours : theirs);
        while (targets) {
            Square to = bb_pop(&targets);
            push(list, from, to, (theirs & sq_bb(to)) ? FLAG_CAPTURE : FLAG_QUIET);
        }
    }

    /* ── Queens ── */
    Bitboard queens = pos->pieces[us][QUEEN];
    while (queens) {
        Square from = bb_pop(&queens);
        Bitboard targets = queen_attacks(from, occ) & (type == GEN_ALL ? ~ours : theirs);
        while (targets) {
            Square to = bb_pop(&targets);
            push(list, from, to, (theirs & sq_bb(to)) ? FLAG_CAPTURE : FLAG_QUIET);
        }
    }

    /* ── King ── */
    {
        Square from = (Square)__builtin_ctzll(pos->pieces[us][KING]);
        Bitboard targets = king_attacks[from] & (type == GEN_ALL ? ~ours : theirs);
        while (targets) {
            Square to = bb_pop(&targets);
            push(list, from, to, (theirs & sq_bb(to)) ? FLAG_CAPTURE : FLAG_QUIET);
        }

        /* Castling — king must not be in check on from or mid square.
         * The destination square is checked by movegen_is_legal via make+in_check. */
        if (type == GEN_ALL) {
            Color opp = us ^ 1;
            if (us == WHITE) {
                if ((pos->castling & CASTLE_WK) && !(occ & 0x60ULL)
                    && !sq_attacked(pos, E1, opp) && !sq_attacked(pos, F1, opp))
                    push(list, E1, G1, FLAG_CASTLE);
                if ((pos->castling & CASTLE_WQ) && !(occ & 0xEULL)
                    && !sq_attacked(pos, E1, opp) && !sq_attacked(pos, D1, opp))
                    push(list, E1, C1, FLAG_CASTLE);
            } else {
                if ((pos->castling & CASTLE_BK) && !(occ & 0x6000000000000000ULL)
                    && !sq_attacked(pos, E8, opp) && !sq_attacked(pos, F8, opp))
                    push(list, E8, G8, FLAG_CASTLE);
                if ((pos->castling & CASTLE_BQ) && !(occ & 0xE00000000000000ULL)
                    && !sq_attacked(pos, E8, opp) && !sq_attacked(pos, D8, opp))
                    push(list, E8, C8, FLAG_CASTLE);
            }
        }
    }

    return list->count;
}

int movegen_is_legal(Position *pos, Move m) {
    UndoInfo undo;
    pos_make_move(pos, m, &undo);
    int legal = !pos_in_check(pos, pos->side ^ 1);
    pos_unmake_move(pos, &undo);
    return legal;
}

void move_to_uci(Move m, char *buf) {
    Square   from  = MOVE_FROM(m);
    Square   to    = MOVE_TO(m);
    uint32_t flags = MOVE_FLAGS(m);
    buf[0] = 'a' + (from % 8);
    buf[1] = '1' + (from / 8);
    buf[2] = 'a' + (to % 8);
    buf[3] = '1' + (to / 8);
    if (flags & 0x20) {
        static const char promo_chars[] = "nbrq";
        buf[4] = promo_chars[flags & 0x03];
        buf[5] = '\0';
    } else {
        buf[4] = '\0';
    }
}

Move move_from_uci(const Position *pos, const char *uci) {
    if (!uci || uci[0] < 'a' || uci[0] > 'h') return MOVE_NONE;
    Square from = (Square)(((uci[1] - '1') * 8) + (uci[0] - 'a'));
    Square to   = (Square)(((uci[3] - '1') * 8) + (uci[2] - 'a'));

    PieceType promo = PIECE_NB;
    if (uci[4]) {
        switch (uci[4]) {
            case 'n': promo = KNIGHT; break;
            case 'b': promo = BISHOP; break;
            case 'r': promo = ROOK;   break;
            case 'q': promo = QUEEN;  break;
        }
    }

    MoveList list;
    movegen_generate(pos, &list, GEN_ALL);
    for (int i = 0; i < list.count; i++) {
        Move m = list.moves[i];
        if (MOVE_FROM(m) != from || MOVE_TO(m) != to) continue;
        uint32_t flags = MOVE_FLAGS(m);
        if (promo != PIECE_NB) {
            if (!(flags & 0x20)) continue;
            if ((PieceType)(KNIGHT + (flags & 0x03)) != promo) continue;
        } else if (flags & 0x20) continue;
        if (movegen_is_legal((Position*)pos, m)) return m;
    }
    return MOVE_NONE;
}

Move move_from_san(Position *pos, const char *san) {
    if (!san || !san[0]) return MOVE_NONE;

    /* strip trailing check/annotation characters */
    char buf[16];
    int len = 0;
    for (int i = 0; san[i] && len < 15; i++) {
        char c = san[i];
        if (c == '+' || c == '#' || c == '!' || c == '?') break;
        buf[len++] = c;
    }
    buf[len] = '\0';
    if (len == 0) return MOVE_NONE;

    /* castling */
    int is_oo  = (strcmp(buf, "O-O")   == 0 || strcmp(buf, "0-0")   == 0);
    int is_ooo = (strcmp(buf, "O-O-O") == 0 || strcmp(buf, "0-0-0") == 0);
    if (is_oo || is_ooo) {
        Square king_sq = (pos->side == WHITE) ? E1 : E8;
        Square to_sq   = is_ooo ? ((pos->side == WHITE) ? C1 : C8)
                                : ((pos->side == WHITE) ? G1 : G8);
        MoveList list;
        movegen_generate(pos, &list, GEN_ALL);
        for (int i = 0; i < list.count; i++) {
            Move m = list.moves[i];
            if (MOVE_FROM(m) == king_sq && MOVE_TO(m) == to_sq && movegen_is_legal(pos, m))
                return m;
        }
        return MOVE_NONE;
    }

    /* promotion: find '=' and extract piece */
    PieceType promo = PIECE_NB;
    char *eq = strchr(buf, '=');
    if (eq) {
        switch (eq[1]) {
            case 'N': promo = KNIGHT; break;
            case 'B': promo = BISHOP; break;
            case 'R': promo = ROOK;   break;
            case 'Q': promo = QUEEN;  break;
        }
        *eq = '\0';
        len = (int)strlen(buf);
    }

    /* piece type from first char */
    int idx = 0;
    PieceType pt = PAWN;
    if (len > 0 && buf[0] >= 'A' && buf[0] <= 'Z') {
        switch (buf[0]) {
            case 'N': pt = KNIGHT; break;
            case 'B': pt = BISHOP; break;
            case 'R': pt = ROOK;   break;
            case 'Q': pt = QUEEN;  break;
            case 'K': pt = KING;   break;
        }
        idx = 1;
    }

    /* destination: last two chars */
    if (len < 2) return MOVE_NONE;
    int dest_file = buf[len - 2] - 'a';
    int dest_rank = buf[len - 1] - '1';
    if (dest_file < 0 || dest_file > 7 || dest_rank < 0 || dest_rank > 7)
        return MOVE_NONE;
    Square to = (Square)(dest_rank * 8 + dest_file);

    /* disambiguation: chars between piece char and destination (skip 'x') */
    int disambig_file = -1, disambig_rank = -1;
    for (int i = idx; i < len - 2; i++) {
        char c = buf[i];
        if (c == 'x') continue;
        if (c >= 'a' && c <= 'h') disambig_file = c - 'a';
        else if (c >= '1' && c <= '8') disambig_rank = c - '1';
    }

    /* find the matching legal move */
    MoveList list;
    movegen_generate(pos, &list, GEN_ALL);
    for (int i = 0; i < list.count; i++) {
        Move m = list.moves[i];
        if (MOVE_TO(m) != to) continue;

        Square from = MOVE_FROM(m);
        if (!(pos->pieces[pos->side][pt] & sq_bb(from))) continue;
        if (disambig_file >= 0 && (from % 8) != (unsigned)disambig_file) continue;
        if (disambig_rank >= 0 && (from / 8) != (unsigned)disambig_rank) continue;

        uint32_t flags = MOVE_FLAGS(m);
        if (promo != PIECE_NB) {
            if (!(flags & 0x20)) continue;
            if ((PieceType)(KNIGHT + (flags & 0x03)) != promo) continue;
        } else {
            if (flags & 0x20) continue;
        }

        if (movegen_is_legal(pos, m)) return m;
    }
    return MOVE_NONE;
}
