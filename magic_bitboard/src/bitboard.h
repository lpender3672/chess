#pragma once
#include "types.h"

/* ── Bit utilities ─────────────────────────────────────────────────────── */
static inline Bitboard sq_bb(Square s)        { return (Bitboard)1 << s; }
static inline int      bb_empty(Bitboard b)   { return b == 0; }
static inline Bitboard bb_lsb(Bitboard b)     { return b & -b; }

#ifdef USE_POPCNT
    #include <nmmintrin.h>
    static inline int popcount(Bitboard b) { return (int)__builtin_popcountll(b); }
#else
    int popcount(Bitboard b);
#endif

/* Returns index of LSB and clears it */
static inline Square bb_pop(Bitboard *b) {
    Square s = (Square)__builtin_ctzll(*b);
    *b &= *b - 1;
    return s;
}

/* ── Pre-computed non-sliding attack tables ────────────────────────────── */
extern Bitboard knight_attacks[64];
extern Bitboard king_attacks[64];
extern Bitboard pawn_attacks[2][64];  /* [color][square] */

void bb_init(void);  /* populate all static tables */

/* ── Rank / file masks ─────────────────────────────────────────────────── */
extern const Bitboard RANK_BB[8];
extern const Bitboard FILE_BB[8];
