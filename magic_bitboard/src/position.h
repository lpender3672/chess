#pragma once
#include "types.h"

#define FEN_STARTPOS "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

/* ── Castling rights bitmask ───────────────────────────────────────────── */
#define CASTLE_WK  1
#define CASTLE_WQ  2
#define CASTLE_BK  4
#define CASTLE_BQ  8

/* ── Position ──────────────────────────────────────────────────────────── */
typedef struct {
    Bitboard  pieces[COLOR_NB][PIECE_NB];  /* pieces[color][type] */
    Bitboard  occupied;                    /* all pieces */
    Bitboard  by_color[COLOR_NB];          /* all pieces of a color */

    Color     side;
    Square    ep_square;                   /* en passant target, or SQ_NONE */
    uint8_t   castling;                    /* CASTLE_* flags */
    int       halfmove;                    /* fifty-move counter */
    int       fullmove;

    uint64_t  hash;                        /* Zobrist hash */
} Position;

/* ── Undo state (what make_move needs to reverse) ──────────────────────── */
typedef struct {
    Square   ep_square;
    uint8_t  castling;
    int      halfmove;
    uint64_t hash;
    Move     move;
    PieceType captured;
} UndoInfo;

/* ── Interface ─────────────────────────────────────────────────────────── */
void     pos_from_fen(Position *pos, const char *fen);
void     pos_to_fen(const Position *pos, char *buf, size_t len);
void     pos_make_move(Position *pos, Move m, UndoInfo *undo);
void     pos_unmake_move(Position *pos, const UndoInfo *undo);
int      pos_in_check(const Position *pos, Color side);

/* Returns the piece type on a square, or PIECE_NB if empty */
PieceType pos_piece_on(const Position *pos, Square sq);

/* Returns 1 if square sq is attacked by any piece of color 'by' */
int sq_attacked(const Position *pos, Square sq, Color by);
