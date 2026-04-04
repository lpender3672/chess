/*
 * perft.c — move generation correctness via node counting.
 *
 * These positions and expected counts are from the Chess Programming Wiki:
 * https://www.chessprogramming.org/Perft_Results
 *
 * Run before the full PGN validation as a fast sanity check.
 */
#include <stdio.h>
#include <string.h>
#include "bitboard.h"
#include "magic.h"
#include "position.h"
#include "movegen.h"

static long perft(Position *pos, int depth) {
    if (depth == 0) return 1;

    MoveList list;
    movegen_generate(pos, &list, GEN_ALL);

    long nodes = 0;
    for (int i = 0; i < list.count; i++) {
        if (!movegen_is_legal(pos, list.moves[i])) continue;
        UndoInfo undo;
        pos_make_move(pos, list.moves[i], &undo);
        nodes += perft(pos, depth - 1);
        pos_unmake_move(pos, &undo);
    }
    return nodes;
}

typedef struct { const char *fen; int depth; long expected; } PerftCase;

static const PerftCase CASES[] = {
    /* Position 1: startpos */
    { FEN_STARTPOS,                                                        1,        20 },
    { FEN_STARTPOS,                                                        2,       400 },
    { FEN_STARTPOS,                                                        3,      8902 },
    { FEN_STARTPOS,                                                        4,    197281 },
    { FEN_STARTPOS,                                                        5,   4865609 },
    /* Position 2: Kiwipete */
    { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", 1,        48 },
    { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", 2,      2039 },
    { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", 3,     97862 },
    { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", 4,   4085603 },
    /* Position 3 */
    { "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -",                            1,        14 },
    { "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -",                            2,       191 },
    { "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -",                            3,      2812 },
    { "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -",                            4,     43238 },
    { "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -",                            5,    674624 },
};

/* Print each move with its node count at depth 1 — used to spot rogue moves */
static void divide(Position *pos) {
    MoveList list;
    movegen_generate(pos, &list, GEN_ALL);
    printf("pseudo-legal count: %d\n", list.count);
    for (int i = 0; i < list.count; i++) {
        if (!movegen_is_legal(pos, list.moves[i])) continue;
        char uci[6];
        move_to_uci(list.moves[i], uci);
        UndoInfo undo;
        pos_make_move(pos, list.moves[i], &undo);
        long n = perft(pos, 0);
        pos_unmake_move(pos, &undo);
        printf("  %s: %ld\n", uci, n);
    }
}

int main(void) {
    bb_init();
    magic_init();

    printf("=== divide kiwipete depth 1 ===\n");
    { Position pos; pos_from_fen(&pos, "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"); divide(&pos); }
    printf("\n");

    int pass = 0, fail = 0;
    for (size_t i = 0; i < sizeof(CASES) / sizeof(*CASES); i++) {
        Position pos;
        pos_from_fen(&pos, CASES[i].fen);
        long result = perft(&pos, CASES[i].depth);
        int ok = (result == CASES[i].expected);
        printf("[%s] depth=%d  got=%-10ld  expected=%-10ld  %s\n",
               CASES[i].fen, CASES[i].depth,
               result, CASES[i].expected, ok ? "PASS" : "FAIL");
        if (ok) pass++; else fail++;
    }
    printf("\n%d passed, %d failed\n", pass, fail);
    return fail > 0 ? 1 : 0;
}
