#pragma once
#include "position.h"
#include "movegen.h"
#include <stdio.h>

/* ── PGN game record ───────────────────────────────────────────────────── */
#define PGN_MAX_MOVES  512
#define PGN_TAG_LEN    128

typedef struct {
    char  event[PGN_TAG_LEN];
    char  white[PGN_TAG_LEN];
    char  black[PGN_TAG_LEN];
    char  result[8];           /* "1-0", "0-1", "1/2-1/2", "*" */
    char  fen[128];            /* starting FEN, or empty for startpos */
    Move  moves[PGN_MAX_MOVES];
    int   move_count;
} PgnGame;

/* ── Parser ────────────────────────────────────────────────────────────── */
typedef struct {
    FILE  *file;
    char  *buf;
    size_t buf_size;
    size_t buf_pos;
    size_t buf_len;
    long   games_parsed;
    long   games_failed;
} PgnParser;

int  pgn_open(PgnParser *p, const char *path);
void pgn_close(PgnParser *p);

/* Reads the next game into g. Returns 1 on success, 0 on EOF, -1 on error. */
int  pgn_next_game(PgnParser *p, PgnGame *g);
