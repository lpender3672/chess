#pragma once
#include <stdint.h>
#include <stddef.h>

/* ── Bitboard ──────────────────────────────────────────────────────────── */
typedef uint64_t Bitboard;

/* ── Squares ───────────────────────────────────────────────────────────── */
typedef enum {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    SQ_NONE = 64
} Square;

/* ── Pieces ────────────────────────────────────────────────────────────── */
typedef enum { WHITE, BLACK, COLOR_NB } Color;

typedef enum {
    PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, PIECE_NB
} PieceType;

/* ── Move encoding (32-bit) ─────────────────────────────────────────────
 *  bits  0- 5 : from square
 *  bits  6-11 : to square
 *  bits 12-13 : promotion piece (0=none/knight,1=bishop,2=rook,3=queen)
 *  bits 14-15 : flags  00=quiet  01=double pawn  10=castling  11=en passant
 *  bit     16 : capture flag
 *  bit     17 : promotion flag
 * ────────────────────────────────────────────────────────────────────── */
typedef uint32_t Move;

#define MOVE_NONE  0u
#define MK_MOVE(from, to, flags) \
    ((Move)((from) | ((to) << 6) | ((flags) << 12)))

#define MOVE_FROM(m)   ((Square)((m) & 0x3F))
#define MOVE_TO(m)     ((Square)(((m) >> 6) & 0x3F))
#define MOVE_FLAGS(m)  (((m) >> 12) & 0x3F)

/* ── Move list ─────────────────────────────────────────────────────────── */
#define MAX_MOVES 256

typedef struct {
    Move  moves[MAX_MOVES];
    int   count;
} MoveList;
