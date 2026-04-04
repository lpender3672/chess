#include "pgn.h"
#include "position.h"
#include "movegen.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define PGN_BUF   65536
#define LINE_MAX  65536   /* Lichess movetext is often one long line */
#define MT_MAX   131072

/* ── Buffered character reader ─────────────────────────────────────────── */
static int pgn_getc(PgnParser *p) {
    if (p->buf_pos >= p->buf_len) {
        p->buf_len = fread(p->buf, 1, p->buf_size, p->file);
        p->buf_pos = 0;
        if (!p->buf_len) return EOF;
    }
    return (unsigned char)p->buf[p->buf_pos++];
}

/* Read one line (strips \r\n). Returns 0 on EOF. */
static int read_line(PgnParser *p, char *buf, int size) {
    int len = 0, c;
    while ((c = pgn_getc(p)) != EOF) {
        if (c == '\n') break;
        if (c == '\r') continue;
        if (len < size - 1) buf[len++] = (char)c;
    }
    buf[len] = '\0';
    return (c != EOF || len > 0);
}

/* ── Tag parsing ───────────────────────────────────────────────────────── */
static void parse_tag(PgnGame *g, const char *line) {
    /* [Key "Value"] */
    const char *p = line + 1;
    while (*p == ' ') p++;

    char key[64] = "";
    int ki = 0;
    while (*p && *p != ' ' && *p != '"' && ki < 63) key[ki++] = *p++;
    key[ki] = '\0';

    while (*p && *p != '"') p++;
    if (*p == '"') p++;

    char val[PGN_TAG_LEN] = "";
    int vi = 0;
    while (*p && *p != '"' && vi < PGN_TAG_LEN - 1) val[vi++] = *p++;
    val[vi] = '\0';

    if      (!strcmp(key, "Event"))  snprintf(g->event,  sizeof(g->event),  "%s", val);
    else if (!strcmp(key, "White"))  snprintf(g->white,  sizeof(g->white),  "%s", val);
    else if (!strcmp(key, "Black"))  snprintf(g->black,  sizeof(g->black),  "%s", val);
    else if (!strcmp(key, "Result")) snprintf(g->result, sizeof(g->result), "%s", val);
    else if (!strcmp(key, "FEN"))    snprintf(g->fen,    sizeof(g->fen),    "%s", val);
}

/* ── Movetext tokeniser ────────────────────────────────────────────────── */
static int is_result_tok(const char *s) {
    return !strcmp(s, "1-0") || !strcmp(s, "0-1") ||
           !strcmp(s, "1/2-1/2") || !strcmp(s, "*");
}

static int is_move_number_tok(const char *s) {
    const char *p = s;
    while (*p && isdigit((unsigned char)*p)) p++;
    if (p == s) return 0;       /* no leading digits */
    if (*p != '.') return 0;    /* must end in dots */
    while (*p == '.') p++;
    return *p == '\0';
}

/* Advance *ptr past whitespace, skip comments/variations/NAGs inline,
 * then return the next whitespace-delimited token (NUL-terminated in-place).
 * Returns NULL at end of string. */
static char *next_token(char **ptr) {
    char *p = *ptr;

retry:
    while (*p && isspace((unsigned char)*p)) p++;
    if (!*p) { *ptr = p; return NULL; }

    /* comment { ... } */
    if (*p == '{') {
        while (*p && *p != '}') p++;
        if (*p) p++;
        goto retry;
    }
    /* variation ( ... ) with nesting */
    if (*p == '(') {
        int depth = 1;
        p++;
        while (*p && depth > 0) {
            if      (*p == '(') depth++;
            else if (*p == ')') depth--;
            else if (*p == '{') { while (*p && *p != '}') p++; }
            p++;
        }
        goto retry;
    }
    /* NAG $N */
    if (*p == '$') {
        while (*p && !isspace((unsigned char)*p)) p++;
        goto retry;
    }

    char *start = p;
    while (*p && !isspace((unsigned char)*p) &&
           *p != '{' && *p != '(' && *p != ')') p++;
    if (*p) *p++ = '\0';
    *ptr = p;
    return start;
}

/* ── Open / Close ──────────────────────────────────────────────────────── */
int pgn_open(PgnParser *p, const char *path) {
    memset(p, 0, sizeof(*p));
    p->file = fopen(path, "r");
    if (!p->file) return 0;
    p->buf = malloc(PGN_BUF);
    if (!p->buf) { fclose(p->file); return 0; }
    p->buf_size = PGN_BUF;
    return 1;
}

void pgn_close(PgnParser *p) {
    if (p->file) fclose(p->file);
    free(p->buf);
}

/* ── Read next game ────────────────────────────────────────────────────── */
int pgn_next_game(PgnParser *p, PgnGame *g) {
    memset(g, 0, sizeof(*g));

    static char line[LINE_MAX];
    static char mt[MT_MAX];

    /* ── 1. Read header tags, skipping any leading blank lines ── */
    int found_header = 0;
    while (read_line(p, line, LINE_MAX)) {
        if (line[0] == '[') {
            found_header = 1;
            parse_tag(g, line);
        } else if (found_header) {
            /* first non-tag line after headers — may be blank or movetext */
            break;
        }
        /* blank lines before first '[' are skipped silently */
    }
    if (!found_header) return 0;  /* true EOF */

    /* ── 2. Accumulate movetext lines ── */
    int mt_len = 0;
    mt[0] = '\0';

    /* `line` already holds the first post-header line from the loop above */
    for (;;) {
        int len = (int)strlen(line);
        if (len == 0) {
            if (mt_len > 0) break;  /* blank line terminates movetext */
        } else if (mt_len + len + 2 < MT_MAX) {
            if (mt_len > 0) mt[mt_len++] = ' ';
            memcpy(mt + mt_len, line, len + 1);
            mt_len += len;
        }
        if (!read_line(p, line, LINE_MAX)) break;
    }

    /* ── 3. Parse movetext tokens into moves ── */
    Position pos;
    pos_from_fen(&pos, g->fen[0] ? g->fen : FEN_STARTPOS);

    char *ptr = mt;
    char *tok;
    while ((tok = next_token(&ptr)) != NULL) {
        if (is_result_tok(tok))    break;
        if (is_move_number_tok(tok)) continue;

        /* handle combined tokens like "1.e4" (no space after dot) */
        char *move_part = tok;
        {
            char *t = tok;
            while (*t && isdigit((unsigned char)*t)) t++;
            while (*t == '.') t++;
            if (*t != '\0') move_part = t;
            else continue;
        }

        if (g->move_count >= PGN_MAX_MOVES) {
            p->games_failed++;
            break;
        }

        Move m = move_from_san(&pos, move_part);
        if (m == MOVE_NONE) {
            fprintf(stderr, "parse error: game %ld, token '%s'\n",
                    p->games_parsed + 1, move_part);
            p->games_failed++;
            /* drain remaining tokens for this game */
            while ((tok = next_token(&ptr)) != NULL && !is_result_tok(tok));
            break;
        }

        g->moves[g->move_count++] = m;
        UndoInfo undo;
        pos_make_move(&pos, m, &undo);
    }

    p->games_parsed++;
    return 1;
}
