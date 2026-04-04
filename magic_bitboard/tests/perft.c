/*
 * perft.c — move generation correctness via node counting.
 *
 * Hardcoded cases are from the Chess Programming Wiki:
 * https://www.chessprogramming.org/Perft_Results
 *
 * EPD suite is loaded from Chess-EPDs/perft.epd (git submodule).
 * Format per line:  <FEN>; D1 <n>; D2 <n>; ...
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bitboard.h"
#include "magic.h"
#include "position.h"
#include "movegen.h"

static long perft(Position *pos, int depth) {
    if (depth == 0) return 1;

    MoveList list;
    movegen_generate(pos, &list, GEN_ALL);

    /* bulk-count at depth 1 — skip make/unmake overhead */
    if (depth == 1) {
        int legal = 0;
        for (int i = 0; i < list.count; i++)
            if (movegen_is_legal(pos, list.moves[i])) legal++;
        return legal;
    }

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

/* Print each move with its node count — used to spot rogue moves.
 * Usage: perft.exe --divide "<fen>" */
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

/* ── EPD perft suite ───────────────────────────────────────────────────── */

/*
 * Parse one EPD perft line:
 *   <FEN tokens (4-6 fields)>; D1 <n>; D2 <n>; ...
 *
 * Runs the deepest depth present that is <= max_depth.
 * Updates *pass / *fail counters.
 */
static void run_epd_line(char *line, int max_depth, int *pass, int *fail) {
    /* strip trailing newline/whitespace */
    int len = (int)strlen(line);
    while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r' || line[len-1] == ' '))
        line[--len] = '\0';
    if (len == 0) return;

    /* split on first ';' to get FEN */
    char *semi = strchr(line, ';');
    if (!semi) return;  /* no depth annotations — skip */
    *semi = '\0';
    char fen[128];
    snprintf(fen, sizeof(fen), "%s", line);
    /* trim trailing space from fen */
    int flen = (int)strlen(fen);
    while (flen > 0 && fen[flen-1] == ' ') fen[--flen] = '\0';

    /* parse depth annotations: "; D<n> <count>" — pick deepest within limit */
    char *p = semi + 1;
    int  best_depth    = 0;
    long best_expected = 0;

    while (p && *p) {
        while (*p == ' ' || *p == ';') p++;
        if (!*p) break;
        int d; long n;
        if (sscanf(p, "D%d %ld", &d, &n) == 2) {
            if (d <= max_depth && d > best_depth) {
                best_depth    = d;
                best_expected = n;
            }
        }
        p = strchr(p, ';');
    }

    if (best_depth == 0) return;  /* no depth within limit — skip */

    Position pos;
    pos_from_fen(&pos, fen);
    long result = perft(&pos, best_depth);
    int ok = (result == best_expected);
    printf("[%s] D%d  got=%-10ld  expected=%-10ld  %s\n",
           fen, best_depth, result, best_expected, ok ? "PASS" : "FAIL");
    if (ok) (*pass)++; else (*fail)++;
}

static void run_epd_file(const char *path, int max_depth, int *pass, int *fail) {
    FILE *f = fopen(path, "r");
    if (!f) {
        printf("(could not open %s — skipping EPD suite)\n", path);
        return;
    }
    char line[512];
    while (fgets(line, sizeof(line), f))
        run_epd_line(line, max_depth, pass, fail);
    fclose(f);
}

/* ── main ──────────────────────────────────────────────────────────────── */
int main(int argc, char **argv) {
    bb_init();
    magic_init();

    /* optional: --divide <fen> */
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "--divide") == 0) {
            Position pos;
            pos_from_fen(&pos, argv[i+1]);
            printf("=== divide ===\n");
            divide(&pos);
            return 0;
        }
    }

    /* max depth for EPD suite (pass as first arg, default 4) */
    int epd_max_depth = 4;
    if (argc >= 2 && argv[1][0] != '-') epd_max_depth = atoi(argv[1]);

    /* ── hardcoded CPW positions ── */
    printf("=== CPW positions ===\n");
    int pass = 0, fail = 0;
    for (size_t i = 0; i < sizeof(CASES) / sizeof(*CASES); i++) {
        Position pos;
        pos_from_fen(&pos, CASES[i].fen);
        long result = perft(&pos, CASES[i].depth);
        int ok = (result == CASES[i].expected);
        printf("[%s] D%d  got=%-10ld  expected=%-10ld  %s\n",
               CASES[i].fen, CASES[i].depth,
               result, CASES[i].expected, ok ? "PASS" : "FAIL");
        if (ok) pass++; else fail++;
    }
    printf("\n%d passed, %d failed\n\n", pass, fail);

    /* ── EPD suite ── */
    printf("=== Chess-EPDs/perft.epd (max depth %d) ===\n", epd_max_depth);
    int epass = 0, efail = 0;

    /* path relative to where the binary is run — try a few common locations */
    const char *epd_paths[] = {
        "../../Chess-EPDs/perft.epd",
        "../Chess-EPDs/perft.epd",
        "Chess-EPDs/perft.epd",
        NULL
    };
    int loaded = 0;
    for (int i = 0; epd_paths[i]; i++) {
        FILE *probe = fopen(epd_paths[i], "r");
        if (probe) { fclose(probe); run_epd_file(epd_paths[i], epd_max_depth, &epass, &efail); loaded = 1; break; }
    }
    if (!loaded) {
        if (argc >= 3) run_epd_file(argv[2], epd_max_depth, &epass, &efail);
        else printf("(EPD file not found — pass path as second argument)\n");
    }
    printf("\n%d passed, %d failed\n", epass, efail);

    return (fail + efail) > 0 ? 1 : 0;
}
