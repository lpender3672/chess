#include "bitboard.h"

Bitboard knight_attacks[64];
Bitboard king_attacks[64];
Bitboard pawn_attacks[2][64];

const Bitboard RANK_BB[8] = {
    0xFFULL, 0xFF00ULL, 0xFF0000ULL, 0xFF000000ULL,
    0xFF00000000ULL, 0xFF0000000000ULL, 0xFF000000000000ULL, 0xFF00000000000000ULL
};

const Bitboard FILE_BB[8] = {
    0x0101010101010101ULL, 0x0202020202020202ULL,
    0x0404040404040404ULL, 0x0808080808080808ULL,
    0x1010101010101010ULL, 0x2020202020202020ULL,
    0x4040404040404040ULL, 0x8080808080808080ULL
};

#ifndef USE_POPCNT
int popcount(Bitboard b) {
    int c = 0;
    for (; b; b &= b - 1) c++;
    return c;
}
#endif

static Bitboard shift_safe(Bitboard b, int delta) {
    if (delta > 0) return b << delta;
    return b >> -delta;
}

void bb_init(void) {
    for (int sq = 0; sq < 64; sq++) {
        Bitboard b = sq_bb((Square)sq);
        int r = sq / 8, f = sq % 8;

        /* Knight */
        Bitboard kn = 0;
        if (r > 1 && f > 0) kn |= b >> 17;
        if (r > 1 && f < 7) kn |= b >> 15;
        if (r > 0 && f > 1) kn |= b >> 10;
        if (r > 0 && f < 6) kn |= b >>  6;
        if (r < 7 && f > 1) kn |= b <<  6;
        if (r < 7 && f < 6) kn |= b << 10;
        if (r < 6 && f > 0) kn |= b << 15;
        if (r < 6 && f < 7) kn |= b << 17;
        knight_attacks[sq] = kn;

        /* King */
        Bitboard kg = 0;
        if (f > 0) { kg |= b >> 1; if (r > 0) kg |= b >> 9; if (r < 7) kg |= b << 7; }
        if (f < 7) { kg |= b << 1; if (r > 0) kg |= b >> 7; if (r < 7) kg |= b << 9; }
        if (r > 0)  kg |= b >> 8;
        if (r < 7)  kg |= b << 8;
        king_attacks[sq] = kg;

        /* Pawns */
        pawn_attacks[WHITE][sq] = ((b & ~FILE_BB[7]) << 9) | ((b & ~FILE_BB[0]) << 7);
        pawn_attacks[BLACK][sq] = ((b & ~FILE_BB[7]) >> 7) | ((b & ~FILE_BB[0]) >> 9);
    }
    (void)shift_safe; /* suppress unused warning */
}
