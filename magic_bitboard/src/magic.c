#include "magic.h"
#include "bitboard.h"
#include <string.h>

MagicEntry rook_magic[64];
MagicEntry bishop_magic[64];

/* Flat attack pools — rooks: max 4096 entries/sq, bishops: max 512 entries/sq */
static Bitboard rook_pool[64 * 4096];
static Bitboard bishop_pool[64 * 512];

/* ── Occupancy mask ────────────────────────────────────────────────────── *
 *  Relevant blocker squares along attack rays, excluding board edges.
 *  Goes from rank 1..6 and file b..g only, so edge squares are omitted
 *  regardless of where the slider sits (handles corner/edge squares correctly).
 * ─────────────────────────────────────────────────────────────────────── */
static Bitboard occupancy_mask(Square sq, int rook) {
    Bitboard mask = 0;
    int r0 = sq / 8, f0 = sq % 8, r, f;
    if (rook) {
        for (r = r0+1; r <= 6; r++) mask |= sq_bb((Square)(f0 + r*8));
        for (r = r0-1; r >= 1; r--) mask |= sq_bb((Square)(f0 + r*8));
        for (f = f0+1; f <= 6; f++) mask |= sq_bb((Square)(f  + r0*8));
        for (f = f0-1; f >= 1; f--) mask |= sq_bb((Square)(f  + r0*8));
    } else {
        for (r = r0+1, f = f0+1; r <= 6 && f <= 6; r++, f++) mask |= sq_bb((Square)(f + r*8));
        for (r = r0+1, f = f0-1; r <= 6 && f >= 1; r++, f--) mask |= sq_bb((Square)(f + r*8));
        for (r = r0-1, f = f0+1; r >= 1 && f <= 6; r--, f++) mask |= sq_bb((Square)(f + r*8));
        for (r = r0-1, f = f0-1; r >= 1 && f >= 1; r--, f--) mask |= sq_bb((Square)(f + r*8));
    }
    return mask;
}

/* ── Slow reference attack function (used to build tables) ────────────── */
static Bitboard slider_attacks_slow(Square sq, Bitboard occ, int rook) {
    const int rook_dirs[4][2]   = {{1,0},{-1,0},{0,1},{0,-1}};
    const int bishop_dirs[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
    const int (*dirs)[2] = rook ? rook_dirs : bishop_dirs;

    Bitboard attacks = 0;
    int r0 = sq / 8, f0 = sq % 8;
    for (int d = 0; d < 4; d++) {
        int r = r0 + dirs[d][0], f = f0 + dirs[d][1];
        while (r >= 0 && r < 8 && f >= 0 && f < 8) {
            Square s = (Square)(r * 8 + f);
            attacks |= sq_bb(s);
            if (occ & sq_bb(s)) break;
            r += dirs[d][0];
            f += dirs[d][1];
        }
    }
    return attacks;
}

/* ── Magic number generator ─────────────────────────────────────────────
 *  Uses xorshift64* PRNG and the sparse-random-number trick to find a
 *  magic multiplier with no bad collisions (constructive collisions — same
 *  attack set for different occupancies — are fine and common).
 * ─────────────────────────────────────────────────────────────────────── */
static uint64_t rng_s;

static uint64_t rng64(void) {
    rng_s ^= rng_s >> 12;
    rng_s ^= rng_s << 25;
    rng_s ^= rng_s >> 27;
    return rng_s * 2685821657736338717ULL;
}

static uint64_t sparse_rand(void) {
    return rng64() & rng64() & rng64();
}

static uint64_t find_magic(Square sq, int rook) {
    Bitboard mask = occupancy_mask(sq, rook);
    int      bits = popcount(mask);
    int      size = 1 << bits;

    Bitboard subsets[4096];
    Bitboard ref_attacks[4096];
    Bitboard table[4096];

    /* enumerate all subsets of mask via Carry-Rippler */
    Bitboard sub = 0;
    for (int i = 0; i < size; i++) {
        subsets[i]     = sub;
        ref_attacks[i] = slider_attacks_slow(sq, sub, rook);
        sub = (sub - mask) & mask;
    }

    for (;;) {
        uint64_t magic = sparse_rand();

        /* cheap filter: at least 6 high bits set in (mask * magic) */
        if (popcount((mask * magic) >> 56) < 6) continue;

        memset(table, 0, sizeof(Bitboard) * (size_t)size);
        int fail = 0;
        for (int i = 0; i < size && !fail; i++) {
            int idx = (int)((subsets[i] * magic) >> (64 - bits));
            if (!table[idx])
                table[idx] = ref_attacks[i];
            else if (table[idx] != ref_attacks[i])
                fail = 1;
        }
        if (!fail) return magic;
    }
}

/* ── Fill attack table for one square ─────────────────────────────────── */
static void fill_attacks(MagicEntry *e, Square sq, int rook) {
    Bitboard mask = e->mask;
    Bitboard sub  = 0;
    do {
        int idx = (int)(((sub & mask) * e->magic) >> e->shift);
        e->attacks[idx] = slider_attacks_slow(sq, sub, rook);
        sub = (sub - mask) & mask;
    } while (sub);
}

/* ── Public initialisation ─────────────────────────────────────────────── */
void magic_init(void) {
    /* deterministic seed — same magics every run */
    rng_s = 0xdeadbeef12345678ULL;

    size_t rook_offset = 0, bishop_offset = 0;
    for (int sq = 0; sq < 64; sq++) {
        /* Rook */
        rook_magic[sq].mask    = occupancy_mask((Square)sq, 1);
        rook_magic[sq].magic   = find_magic((Square)sq, 1);
        rook_magic[sq].shift   = 64 - popcount(rook_magic[sq].mask);
        rook_magic[sq].attacks = rook_pool + rook_offset;
        rook_offset           += (size_t)1 << popcount(rook_magic[sq].mask);
        fill_attacks(&rook_magic[sq], (Square)sq, 1);

        /* Bishop */
        bishop_magic[sq].mask    = occupancy_mask((Square)sq, 0);
        bishop_magic[sq].magic   = find_magic((Square)sq, 0);
        bishop_magic[sq].shift   = 64 - popcount(bishop_magic[sq].mask);
        bishop_magic[sq].attacks = bishop_pool + bishop_offset;
        bishop_offset           += (size_t)1 << popcount(bishop_magic[sq].mask);
        fill_attacks(&bishop_magic[sq], (Square)sq, 0);
    }
}
