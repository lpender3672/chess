#include "magic.h"
#include "bitboard.h"
#include <string.h>

MagicEntry rook_magic[64];
MagicEntry bishop_magic[64];

/* Attack tables are flat arrays shared across all squares.
 * Each square gets a slice of the pool; the pointer in MagicEntry.attacks
 * points into the right offset.  Rooks need up to 2^12 = 4096 entries per
 * square (12 relevant blocker bits on an open board); bishops need up to
 * 2^9 = 512 (corner squares have only 6 relevant bits, but we allocate the
 * maximum to keep the offset arithmetic simple). */
static Bitboard rook_pool[64 * 4096];
static Bitboard bishop_pool[64 * 512];

/* ── Occupancy mask ────────────────────────────────────────────────────── *
 *
 * For a slider on square sq, the "occupancy mask" is the set of squares
 * whose occupancy actually affects where the slider can move.  Edge squares
 * are excluded: a rook on a1 sliding north will always stop at a8 regardless
 * of whether a8 is occupied (it cannot go further), so a8 doesn't need to be
 * in the mask.  Excluding edges reduces the number of bits in the mask,
 * which reduces the size of the attack sub-table and speeds up the search
 * for a valid magic number.
 *
 * The key implementation detail: we loop ranks 1..6 and files b..g (i.e.
 * indices 1..6 in both dimensions) rather than masking out the edge ranks/
 * files with a bitwise AND after the fact.  The AND approach is wrong for
 * sliders that already sit on an edge — e.g. a rook on a1 (file 0) would
 * have its entire file zeroed out, leaving an empty mask.  By iterating from
 * the slider's own rank/file outward and clamping at 6, we naturally include
 * the correct interior squares in every case.
 * ─────────────────────────────────────────────────────────────────────── */
static Bitboard occupancy_mask(Square sq, int rook) {
    Bitboard mask = 0;
    int r0 = sq / 8, f0 = sq % 8, r, f;
    if (rook) {
        /* Walk each ray from the slider outward, stopping before the board edge */
        for (r = r0+1; r <= 6; r++) mask |= sq_bb((Square)(f0 + r*8));  /* north */
        for (r = r0-1; r >= 1; r--) mask |= sq_bb((Square)(f0 + r*8));  /* south */
        for (f = f0+1; f <= 6; f++) mask |= sq_bb((Square)(f  + r0*8)); /* east  */
        for (f = f0-1; f >= 1; f--) mask |= sq_bb((Square)(f  + r0*8)); /* west  */
    } else {
        /* Four diagonal rays */
        for (r = r0+1, f = f0+1; r <= 6 && f <= 6; r++, f++) mask |= sq_bb((Square)(f + r*8)); /* NE */
        for (r = r0+1, f = f0-1; r <= 6 && f >= 1; r++, f--) mask |= sq_bb((Square)(f + r*8)); /* NW */
        for (r = r0-1, f = f0+1; r >= 1 && f <= 6; r--, f++) mask |= sq_bb((Square)(f + r*8)); /* SE */
        for (r = r0-1, f = f0-1; r >= 1 && f >= 1; r--, f--) mask |= sq_bb((Square)(f + r*8)); /* SW */
    }
    return mask;
}

/* ── Slow reference attack function ───────────────────────────────────── *
 *
 * Computes the exact attack set for a slider on sq given an occupancy
 * bitboard occ.  This is O(n) in board size and used only at initialisation
 * time to fill the magic attack tables — never during search.
 *
 * Each ray walks outward one square at a time.  If it hits an occupied
 * square it adds that square (captures are legal) and then stops (the
 * slider is blocked).
 * ─────────────────────────────────────────────────────────────────────── */
static Bitboard slider_attacks_slow(Square sq, Bitboard occ, int rook) {
    // This is like the function I originally wrote in 2019 for my first chess program

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
            if (occ & sq_bb(s)) break;  /* blocked — include the blocker, then stop */
            r += dirs[d][0];
            f += dirs[d][1];
        }
    }
    return attacks;
}

/* ── Magic number search ────────────────────────────────────────────────
 *
 * A "magic number" is a 64-bit multiplier that, combined with a right shift,
 * maps any occupancy subset of the mask to a unique index into the attack
 * table — a perfect (or near-perfect) hash.
 *
 * The mapping for square sq with n = popcount(mask) relevant bits is:
 *
 *   index = (occupancy * magic) >> (64 - n)
 *
 * We need this to be collision-free for all 2^n subsets, meaning no two
 * different occupancies that produce different attack sets must map to the
 * same index.  (Two occupancies that happen to produce the same attack set
 * CAN share an index — this is called a "constructive collision" and is fine.)
 *
 * The search strategy:
 *   1. Enumerate all 2^n subsets of the mask (via Carry-Rippler).
 *   2. Pre-compute the correct attack set for each subset.
 *   3. Draw a candidate magic from a sparse random number generator
 *      (AND of three randoms biases toward numbers with few set bits,
 *      which empirically find valid magics faster).
 *   4. Apply a cheap quality filter: the top 8 bits of (mask * magic)
 *      should have at least 6 set bits — weak magics rarely pass this.
 *   5. Test all 2^n subsets: if any two with different attacks collide, discard
 *      and try a new candidate.  Otherwise the magic is valid.
 *
 * With a fixed seed the search is fully deterministic; the same magics are
 * generated on every run.
 * ─────────────────────────────────────────────────────────────────────── */
static uint64_t rng_s;

/* xorshift64* — fast, high-quality 64-bit PRNG */
static uint64_t rng64(void) {
    rng_s ^= rng_s >> 12;
    rng_s ^= rng_s << 25;
    rng_s ^= rng_s >> 27;
    return rng_s * 2685821657736338717ULL;
}

/* Sparse random: AND of three outputs gives numbers with ~few set bits,
 * which tend to be better magic candidates */
static uint64_t sparse_rand(void) {
    return rng64() & rng64() & rng64();
}

static uint64_t find_magic(Square sq, int rook) {
    Bitboard mask = occupancy_mask(sq, rook);
    int      bits = popcount(mask);   /* number of relevant blocker squares */
    int      size = 1 << bits;        /* number of occupancy subsets to test */

    Bitboard subsets[4096];
    Bitboard ref_attacks[4096];
    Bitboard table[4096];

    /* Carry-Rippler subset enumeration.
     *
     * Goal: visit every subset of `mask` — i.e. every bitboard where only
     * bits that are set in mask can be set — exactly once.
     *
     * The naive approach would loop i from 0..(size-1) and use i directly as
     * a bitmask, but i iterates over all integers, not over subsets of mask.
     * You would need to scatter i's bits into the positions of mask, which
     * requires either BMI2 pdep or an inner loop.
     *
     * Carry-Rippler does it in one expression per iteration:
     *
     *   sub = (sub - mask) & mask
     *
     * Starting from sub=0, each step produces the next subset.  The subtraction
     * borrows through the bits of mask exactly as binary counting would, but
     * the & mask clears all bits outside the mask positions, so the carry only
     * propagates within the mask's own bit positions — skipping the gaps
     * between them.  The sequence visits all 2^n subsets before returning to 0.
     *
     * Crucially the subsets come out already positioned at the correct board
     * squares, so they can be passed directly to slider_attacks_slow() as
     * occupancy bitboards with no translation step. */
    Bitboard sub = 0;
    for (int i = 0; i < size; i++) {
        subsets[i]     = sub;
        ref_attacks[i] = slider_attacks_slow(sq, sub, rook);
        sub = (sub - mask) & mask;
    }

    for (;;) {
        uint64_t magic = sparse_rand();

        /* Quick filter: at least 6 of the top 8 bits of (mask*magic) must be
         * set.  This rejects obviously poor candidates cheaply. */
        if (popcount((mask * magic) >> 56) < 6) continue;

        /* Test all subsets for collisions */
        memset(table, 0, sizeof(Bitboard) * (size_t)size);
        int fail = 0;
        for (int i = 0; i < size && !fail; i++) {
            int idx = (int)((subsets[i] * magic) >> (64 - bits));
            if (!table[idx])
                table[idx] = ref_attacks[i];          /* first use of this slot */
            else if (table[idx] != ref_attacks[i])
                fail = 1;                              /* destructive collision — discard */
            /* table[idx] == ref_attacks[i]: constructive collision — fine */
        }
        if (!fail) return magic;
    }
}

/* ── Fill attack table for one square ─────────────────────────────────── *
 *
 * Once a valid magic is known, populate the attack sub-table by walking
 * every occupancy subset again and writing the correct attack bitboard at
 * the hashed index.  At query time the same hash is applied to the real
 * occupancy (masked to the relevant bits) to look up the pre-computed result
 * in O(1).
 * ─────────────────────────────────────────────────────────────────────── */
static void fill_attacks(MagicEntry *e, Square sq, int rook) {
    Bitboard mask = e->mask;
    Bitboard sub  = 0;
    do {
        int idx = (int)(((sub & mask) * e->magic) >> e->shift);
        e->attacks[idx] = slider_attacks_slow(sq, sub, rook);
        sub = (sub - mask) & mask;  /* next subset via Carry-Rippler — see find_magic() */
    } while (sub);
}

/* ── Public initialisation ─────────────────────────────────────────────── *
 *
 * Called once at startup.  For each of the 64 squares we:
 *   1. Compute the occupancy mask.
 *   2. Search for a valid magic number.
 *   3. Record the shift (64 - popcount(mask)) needed to produce the index.
 *   4. Point attacks[] into the shared pool at the correct offset.
 *   5. Fill the attack table.
 *
 * The fixed seed makes the magic search deterministic so the same numbers
 * are found every run (no need to serialise them to disk for this engine).
 * ─────────────────────────────────────────────────────────────────────── */
void magic_init(void) {
    rng_s = 0xdeadbeef12345678ULL;  /* fixed seed — deterministic magic search */

    size_t rook_offset = 0, bishop_offset = 0;
    for (int sq = 0; sq < 64; sq++) {
        /* ── Rook ── */
        rook_magic[sq].mask    = occupancy_mask((Square)sq, 1);
        rook_magic[sq].magic   = find_magic((Square)sq, 1);
        rook_magic[sq].shift   = 64 - popcount(rook_magic[sq].mask);
        rook_magic[sq].attacks = rook_pool + rook_offset;
        rook_offset           += (size_t)1 << popcount(rook_magic[sq].mask);
        fill_attacks(&rook_magic[sq], (Square)sq, 1);

        /* ── Bishop ── */
        bishop_magic[sq].mask    = occupancy_mask((Square)sq, 0);
        bishop_magic[sq].magic   = find_magic((Square)sq, 0);
        bishop_magic[sq].shift   = 64 - popcount(bishop_magic[sq].mask);
        bishop_magic[sq].attacks = bishop_pool + bishop_offset;
        bishop_offset           += (size_t)1 << popcount(bishop_magic[sq].mask);
        fill_attacks(&bishop_magic[sq], (Square)sq, 0);
    }
}
