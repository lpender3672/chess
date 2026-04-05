#include "position.h"
#include "bitboard.h"
#include "magic.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* ── Zobrist tables ────────────────────────────────────────────────────── */
static uint64_t zobrist_piece[COLOR_NB][PIECE_NB][64];
static uint64_t zobrist_ep[8];
static uint64_t zobrist_castling[16];
static uint64_t zobrist_side;

static uint64_t xorshift64(uint64_t *state) {
    *state ^= *state << 13;
    *state ^= *state >> 7;
    *state ^= *state << 17;
    return *state;
}

__attribute__((constructor))
static void zobrist_init(void) {
    uint64_t seed = 0xdeadbeefcafeULL;
    for (int c = 0; c < COLOR_NB; c++)
        for (int p = 0; p < PIECE_NB; p++)
            for (int s = 0; s < 64; s++)
                zobrist_piece[c][p][s] = xorshift64(&seed);
    for (int f = 0; f < 8; f++)
        zobrist_ep[f] = xorshift64(&seed);
    for (int i = 0; i < 16; i++)
        zobrist_castling[i] = xorshift64(&seed);
    zobrist_side = xorshift64(&seed);
}

/* ── Piece helpers ─────────────────────────────────────────────────────── */
static void add_piece(Position *pos, Color c, PieceType pt, Square sq) {
    Bitboard b = sq_bb(sq);
    pos->pieces[c][pt]  |= b;
    pos->by_color[c]    |= b;
    pos->occupied       |= b;
    pos->hash           ^= zobrist_piece[c][pt][sq];
}

static void remove_piece(Position *pos, Color c, PieceType pt, Square sq) {
    Bitboard b = sq_bb(sq);
    pos->pieces[c][pt]  &= ~b;
    pos->by_color[c]    &= ~b;
    pos->occupied       &= ~b;
    pos->hash           ^= zobrist_piece[c][pt][sq];
}

static inline PieceType piece_on(const Position *pos, Color c, Square sq) {
    Bitboard b = sq_bb(sq);
    for (int pt = 0; pt < PIECE_NB; pt++)
        if (pos->pieces[c][pt] & b) return (PieceType)pt;
    return PIECE_NB;
}

/* ── Castling rights mask ──────────────────────────────────────────────── */
static inline uint8_t castle_mask(Square sq) {
    switch (sq) {
        case A1: return (uint8_t)~CASTLE_WQ;
        case H1: return (uint8_t)~CASTLE_WK;
        case E1: return (uint8_t)~(CASTLE_WK | CASTLE_WQ);
        case A8: return (uint8_t)~CASTLE_BQ;
        case H8: return (uint8_t)~CASTLE_BK;
        case E8: return (uint8_t)~(CASTLE_BK | CASTLE_BQ);
        default: return 0xFF;
    }
}

/* ── FEN parsing ───────────────────────────────────────────────────────── */
void pos_from_fen(Position *pos, const char *fen) {
    memset(pos, 0, sizeof(*pos));
    pos->ep_square = SQ_NONE;

    static const char piece_chars[] = "PNBRQKpnbrqk";
    int sq = 56;  /* start at a8 */

    while (*fen && *fen != ' ') {
        char c = *fen++;
        if (c == '/') { sq -= 16; continue; }
        if (isdigit(c)) { sq += c - '0'; continue; }
        const char *pc = strchr(piece_chars, c);
        if (pc) {
            int idx = (int)(pc - piece_chars);
            Color col = (idx < 6) ? WHITE : BLACK;
            PieceType pt = (PieceType)(idx % 6);
            add_piece(pos, col, pt, (Square)sq++);
        }
    }
    if (!*fen++) return;

    pos->side = (*fen++ == 'w') ? WHITE : BLACK;
    if (pos->side == BLACK) pos->hash ^= zobrist_side;
    if (!*fen++) return;  /* advance past space before castling */

    while (*fen && *fen != ' ') {
        switch (*fen++) {
            case 'K': pos->castling |= CASTLE_WK; break;
            case 'Q': pos->castling |= CASTLE_WQ; break;
            case 'k': pos->castling |= CASTLE_BK; break;
            case 'q': pos->castling |= CASTLE_BQ; break;
        }
    }
    pos->hash ^= zobrist_castling[pos->castling];
    if (!*fen++) return;

    if (*fen != '-') {
        int file = *fen++ - 'a';
        int rank = *fen   - '1';
        pos->ep_square = (Square)(rank * 8 + file);
        pos->hash ^= zobrist_ep[file];
    }
    while (*fen && *fen != ' ') fen++;
    if (*fen) fen++;

    sscanf(fen, "%d %d", &pos->halfmove, &pos->fullmove);
}

/* ── FEN serialisation ─────────────────────────────────────────────────── */
void pos_to_fen(const Position *pos, char *buf, size_t len) {
    static const char piece_chars[COLOR_NB][PIECE_NB] = {
        { 'P','N','B','R','Q','K' },   /* WHITE */
        { 'p','n','b','r','q','k' },   /* BLACK */
    };

    char tmp[128];
    char *p = tmp;

    /* piece placement */
    for (int rank = 7; rank >= 0; rank--) {
        int empty = 0;
        for (int file = 0; file < 8; file++) {
            Square sq = (Square)(rank * 8 + file);
            Bitboard b = sq_bb(sq);
            char c = 0;
            for (int col = 0; col < COLOR_NB && !c; col++)
                for (int pt = 0; pt < PIECE_NB && !c; pt++)
                    if (pos->pieces[col][pt] & b)
                        c = piece_chars[col][pt];
            if (c) {
                if (empty) *p++ = (char)('0' + empty), empty = 0;
                *p++ = c;
            } else {
                empty++;
            }
        }
        if (empty) *p++ = (char)('0' + empty);
        if (rank) *p++ = '/';
    }
    *p++ = ' ';

    /* side to move */
    *p++ = (pos->side == WHITE) ? 'w' : 'b';
    *p++ = ' ';

    /* castling */
    if (pos->castling) {
        if (pos->castling & CASTLE_WK) *p++ = 'K';
        if (pos->castling & CASTLE_WQ) *p++ = 'Q';
        if (pos->castling & CASTLE_BK) *p++ = 'k';
        if (pos->castling & CASTLE_BQ) *p++ = 'q';
    } else {
        *p++ = '-';
    }
    *p++ = ' ';

    /* en passant */
    if (pos->ep_square != SQ_NONE) {
        *p++ = (char)('a' + (pos->ep_square % 8));
        *p++ = (char)('1' + (pos->ep_square / 8));
    } else {
        *p++ = '-';
    }

    *p = '\0';
    snprintf(buf, len, "%s %d %d", tmp, pos->halfmove, pos->fullmove);
}

/* ── Make move ─────────────────────────────────────────────────────────── */
void pos_make_move(Position *pos, Move m, UndoInfo *undo) {
    Color    us    = pos->side;
    Color    them  = us ^ 1;
    Square   from  = MOVE_FROM(m);
    Square   to    = MOVE_TO(m);
    uint32_t flags = MOVE_FLAGS(m);

    undo->ep_square = pos->ep_square;
    undo->castling  = pos->castling;
    undo->halfmove  = pos->halfmove;
    undo->hash      = pos->hash;
    undo->move      = m;
    undo->captured  = PIECE_NB;

    /* remove variable hash contributions that are about to change */
    pos->hash ^= zobrist_castling[pos->castling];
    if (pos->ep_square != SQ_NONE)
        pos->hash ^= zobrist_ep[pos->ep_square % 8];
    pos->ep_square = SQ_NONE;

    PieceType pt = piece_on(pos, us, from);

    /* flag decoding — guard specials behind !is_promo to avoid aliasing
     * (queen-promo-capture flags = 0x33, whose lower nibble == FLAG_EP = 0x03) */
    int is_promo  = !!(flags & 0x20);
    int is_ep     = !is_promo && ((flags & 0x0F) == 0x03);
    int is_castle = !is_promo && ((flags & 0x0F) == 0x02);
    int is_dbl_pp = !is_promo && ((flags & 0x0F) == 0x01);

    /* capture */
    if (is_ep) {
        Square cap = (Square)(to + (us == WHITE ? -8 : 8));
        undo->captured = PAWN;
        remove_piece(pos, them, PAWN, cap);
    } else if (flags & 0x10) {
        undo->captured = piece_on(pos, them, to);
        remove_piece(pos, them, undo->captured, to);
    }

    /* move piece */
    remove_piece(pos, us, pt, from);
    if (is_promo) {
        /* lower 2 bits encode offset from KNIGHT: 0=knight,1=bishop,2=rook,3=queen */
        add_piece(pos, us, (PieceType)(KNIGHT + (flags & 0x03)), to);
    } else {
        add_piece(pos, us, pt, to);
    }

    /* move rook for castling */
    if (is_castle) {
        if      (to == G1) { remove_piece(pos, WHITE, ROOK, H1); add_piece(pos, WHITE, ROOK, F1); }
        else if (to == C1) { remove_piece(pos, WHITE, ROOK, A1); add_piece(pos, WHITE, ROOK, D1); }
        else if (to == G8) { remove_piece(pos, BLACK, ROOK, H8); add_piece(pos, BLACK, ROOK, F8); }
        else if (to == C8) { remove_piece(pos, BLACK, ROOK, A8); add_piece(pos, BLACK, ROOK, D8); }
    }

    /* set en-passant square for double pawn push */
    if (is_dbl_pp) {
        pos->ep_square = (Square)((from + to) / 2);
        pos->hash ^= zobrist_ep[pos->ep_square % 8];
    }

    /* update castling rights */
    pos->castling &= castle_mask(from) & castle_mask(to);
    pos->hash ^= zobrist_castling[pos->castling];

    /* halfmove clock */
    if (pt == PAWN || undo->captured != PIECE_NB) pos->halfmove = 0;
    else pos->halfmove++;

    if (us == BLACK) pos->fullmove++;
    pos->side  = them;
    pos->hash ^= zobrist_side;
}

/* ── Unmake move ───────────────────────────────────────────────────────── */
void pos_unmake_move(Position *pos, const UndoInfo *undo) {
    /* flip side back to the mover */
    pos->side ^= 1;
    Color    us    = pos->side;
    Color    them  = us ^ 1;
    Move     m     = undo->move;
    Square   from  = MOVE_FROM(m);
    Square   to    = MOVE_TO(m);
    uint32_t flags = MOVE_FLAGS(m);

    int is_promo  = !!(flags & 0x20);
    int is_ep     = !is_promo && ((flags & 0x0F) == 0x03);
    int is_castle = !is_promo && ((flags & 0x0F) == 0x02);

    /* move piece back */
    if (is_promo) {
        remove_piece(pos, us, (PieceType)(KNIGHT + (flags & 0x03)), to);
        add_piece(pos, us, PAWN, from);
    } else {
        PieceType pt = piece_on(pos, us, to);
        remove_piece(pos, us, pt, to);
        add_piece(pos, us, pt, from);
    }

    /* restore captured piece */
    if (is_ep) {
        add_piece(pos, them, PAWN, (Square)(to + (us == WHITE ? -8 : 8)));
    } else if (undo->captured != PIECE_NB) {
        add_piece(pos, them, undo->captured, to);
    }

    /* move rook back for castling */
    if (is_castle) {
        if      (to == G1) { remove_piece(pos, WHITE, ROOK, F1); add_piece(pos, WHITE, ROOK, H1); }
        else if (to == C1) { remove_piece(pos, WHITE, ROOK, D1); add_piece(pos, WHITE, ROOK, A1); }
        else if (to == G8) { remove_piece(pos, BLACK, ROOK, F8); add_piece(pos, BLACK, ROOK, H8); }
        else if (to == C8) { remove_piece(pos, BLACK, ROOK, D8); add_piece(pos, BLACK, ROOK, A8); }
    }

    pos->ep_square = undo->ep_square;
    pos->castling  = undo->castling;
    pos->halfmove  = undo->halfmove;
    pos->hash      = undo->hash;
    if (us == BLACK) pos->fullmove--;
}

/* ── Misc ──────────────────────────────────────────────────────────────── */
PieceType pos_piece_on(const Position *pos, Square sq) {
    return piece_on(pos, WHITE, sq) != PIECE_NB
         ? piece_on(pos, WHITE, sq)
         : piece_on(pos, BLACK, sq);
}

int sq_attacked(const Position *pos, Square sq, Color by) {
    Bitboard occ = pos->occupied;
    Color def = by ^ 1;
    return !!(
        (knight_attacks[sq]       & pos->pieces[by][KNIGHT])
      | (pawn_attacks[def][sq]    & pos->pieces[by][PAWN])
      | (king_attacks[sq]         & pos->pieces[by][KING])
      | (rook_attacks(sq, occ)    & (pos->pieces[by][ROOK]   | pos->pieces[by][QUEEN]))
      | (bishop_attacks(sq, occ)  & (pos->pieces[by][BISHOP] | pos->pieces[by][QUEEN]))
    );
}

int pos_in_check(const Position *pos, Color side) {
    Square   ksq = (Square)__builtin_ctzll(pos->pieces[side][KING]);
    Color    opp = side ^ 1;
    Bitboard occ = pos->occupied;
    return !!(
        (knight_attacks[ksq]       & pos->pieces[opp][KNIGHT])
      | (pawn_attacks[side][ksq]   & pos->pieces[opp][PAWN])
      | (king_attacks[ksq]         & pos->pieces[opp][KING])
      | (rook_attacks(ksq, occ)    & (pos->pieces[opp][ROOK]   | pos->pieces[opp][QUEEN]))
      | (bishop_attacks(ksq, occ)  & (pos->pieces[opp][BISHOP] | pos->pieces[opp][QUEEN]))
    );
}
