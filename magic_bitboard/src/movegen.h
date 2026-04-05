#pragma once
#include "types.h"
#include "position.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GEN_ALL,       /* pseudo-legal + will filter check */
    GEN_CAPTURES,  /* captures and promotions only */
} GenType;

/* Fills list with pseudo-legal moves. Returns number of moves generated. */
int  movegen_generate(const Position *pos, MoveList *list, GenType type);

/* Returns 1 if move is legal in position (handles check filtering). */
int  movegen_is_legal(Position *pos, Move m);

/* Converts a move to algebraic notation (e.g. "e2e4", "e7e8q") */
void move_to_uci(Move m, char *buf);

/* Parses UCI move string into a Move, validated against position. */
Move move_from_uci(const Position *pos, const char *uci);

/* Parses SAN (Standard Algebraic Notation) move string. */
Move move_from_san(Position *pos, const char *san);

#ifdef __cplusplus
}
#endif
