/*
 * validate.c — replay every move in a Lichess PGN database and verify that
 *              each played move is present in our generated legal move list.
 *
 * Usage:  validate <path/to/database.pgn>
 *
 * Database: https://database.lichess.org/standard/lichess_db_standard_rated_2013-01.pgn.zst
 * Decompress with zstd before use:
 *   zstd -d lichess_db_standard_rated_2013-01.pgn.zst
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bitboard.h"
#include "magic.h"
#include "position.h"
#include "movegen.h"
#include "pgn.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <database.pgn>\n", argv[0]);
        return 1;
    }

    bb_init();
    magic_init();

    PgnParser parser;
    if (!pgn_open(&parser, argv[1])) {
        fprintf(stderr, "error: cannot open %s\n", argv[1]);
        return 1;
    }

    PgnGame    game;
    Position   pos;
    MoveList   legal;
    long       games_ok     = 0;
    long       games_fail   = 0;
    long       moves_total  = 0;

    while (pgn_next_game(&parser, &game) == 1) {
        const char *fen = game.fen[0] ? game.fen : FEN_STARTPOS;
        pos_from_fen(&pos, fen);

        int game_ok = 1;
        for (int i = 0; i < game.move_count; i++) {
            Move played = game.moves[i];

            movegen_generate(&pos, &legal, GEN_ALL);

            /* Check played move is in legal list */
            int found = 0;
            for (int j = 0; j < legal.count; j++) {
                if (legal.moves[j] == played && movegen_is_legal(&pos, legal.moves[j])) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                char uci[6];
                move_to_uci(played, uci);
                fprintf(stderr, "FAIL game %ld move %d: %s not legal\n",
                        parser.games_parsed, i + 1, uci);
                game_ok = 0;
                break;
            }

            UndoInfo undo;
            pos_make_move(&pos, played, &undo);
            moves_total++;
        }

        if (game_ok) games_ok++; else games_fail++;

        if ((games_ok + games_fail) % 10000 == 0)
            printf("\rgames: %ld ok  %ld fail  moves: %ld",
                   games_ok, games_fail, moves_total);
    }

    printf("\n\nDone.\n");
    printf("Games OK:   %ld\n", games_ok);
    printf("Games fail: %ld\n", games_fail);
    printf("Moves:      %ld\n", moves_total);

    pgn_close(&parser);
    return games_fail > 0 ? 1 : 0;
}
