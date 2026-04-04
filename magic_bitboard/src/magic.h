#pragma once
#include "types.h"

/* ── Magic bitboard lookup for sliding pieces ──────────────────────────
 *
 *  For each square, a magic number M is chosen such that:
 *      index = ((occupancy & mask[sq]) * M[sq]) >> shift[sq]
 *  maps to a unique entry in an attack table with no collisions.
 *
 *  With USE_BMI2, PEXT replaces the multiply+shift entirely:
 *      index = _pext_u64(occupancy, mask[sq])
 * ────────────────────────────────────────────────────────────────────── */

typedef struct {
    Bitboard  mask;       /* relevant occupancy bits for this square */
    Bitboard  magic;      /* magic multiplier */
    int       shift;      /* 64 - popcount(mask) */
    Bitboard *attacks;    /* pointer into flat attack pool */
} MagicEntry;

extern MagicEntry rook_magic[64];
extern MagicEntry bishop_magic[64];

void magic_init(void);  /* must be called once at startup after bb_init() */

static inline Bitboard rook_attacks(Square sq, Bitboard occ) {
#ifdef USE_BMI2
    #include <immintrin.h>
    return rook_magic[sq].attacks[_pext_u64(occ, rook_magic[sq].mask)];
#else
    const MagicEntry *e = &rook_magic[sq];
    return e->attacks[((occ & e->mask) * e->magic) >> e->shift];
#endif
}

static inline Bitboard bishop_attacks(Square sq, Bitboard occ) {
#ifdef USE_BMI2
    #include <immintrin.h>
    return bishop_magic[sq].attacks[_pext_u64(occ, bishop_magic[sq].mask)];
#else
    const MagicEntry *e = &bishop_magic[sq];
    return e->attacks[((occ & e->mask) * e->magic) >> e->shift];
#endif
}

static inline Bitboard queen_attacks(Square sq, Bitboard occ) {
    return rook_attacks(sq, occ) | bishop_attacks(sq, occ);
}
