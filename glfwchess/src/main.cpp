#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "bitboard.h"
#include "magic.h"
#include "movegen.h"
#include "position.h"

// ── Constants ────────────────────────────────────────────────────────────────

// pieces.png: 2 rows (white=0, black=1) x 6 cols (K,Q,B,N,R,P left-to-right)
static const int kSpriteCol[6] = { 5, 3, 2, 4, 1, 0 }; // indexed by PieceType

static const int   kBarH         = 100;
static const float kPickSize     = 90.0f;
static const float kPickGap      = 8.0f;
static const float kDefaultTime  = 180.0f;  // seconds per player

// Promotion picker order (left-to-right)
static const PieceType kPromoOptions[4] = { QUEEN, ROOK, BISHOP, KNIGHT };

// promo flags & 0x03: 0=KNIGHT,1=BISHOP,2=ROOK,3=QUEEN  (KNIGHT + offset)
static PieceType promo_piece_from_flags(uint32_t flags) {
    return (PieceType)(KNIGHT + (flags & 0x03));
}

// ── Font ─────────────────────────────────────────────────────────────────────

static const int kAtlasW = 512, kAtlasH = 512;

struct Font {
    GLuint          tex    = 0;
    stbtt_bakedchar chars[96];   // ASCII 32–127
    float           size   = 0;  // baked pixel height
    float           ascent = 0;  // px above baseline
    float           descent= 0;  // px below baseline (positive value)
};

static bool font_init(Font &font, const char *path, float size_px) {
    FILE *f = std::fopen(path, "rb");
    if (!f) return false;
    std::fseek(f, 0, SEEK_END);
    long file_size = std::ftell(f);
    std::rewind(f);
    std::vector<uint8_t> ttf(file_size);
    std::fread(ttf.data(), 1, file_size, f);
    std::fclose(f);

    // Bake ASCII glyphs into an alpha-only atlas
    std::vector<uint8_t> bitmap(kAtlasW * kAtlasH);
    stbtt_BakeFontBitmap(ttf.data(), 0, size_px,
                          bitmap.data(), kAtlasW, kAtlasH,
                          32, 96, font.chars);

    // Get vertical metrics for proper centering
    stbtt_fontinfo info;
    stbtt_InitFont(&info, ttf.data(), 0);
    int ia, id, ilg;
    stbtt_GetFontVMetrics(&info, &ia, &id, &ilg);
    float scale  = stbtt_ScaleForPixelHeight(&info, size_px);
    font.ascent  =  ia * scale;
    font.descent = -id * scale;   // stb descent is negative, flip to positive
    font.size    = size_px;

    glGenTextures(1, &font.tex);
    glBindTexture(GL_TEXTURE_2D, font.tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // GL_ALPHA: glyph coverage in alpha channel, colour set via glColor
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA,
                  kAtlasW, kAtlasH, 0,
                  GL_ALPHA, GL_UNSIGNED_BYTE, bitmap.data());
    return true;
}

// Measure text width without drawing.
static float text_width(const Font &font, const char *text) {
    float x = 0, y = 0;
    while (*text) {
        if (*text >= 32 && *text < 128) {
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(font.chars, kAtlasW, kAtlasH,
                                *text - 32, &x, &y, &q, 0);
        }
        ++text;
    }
    return x;
}

// Draw text with baseline at (x, y). r/g/b in [0,1].
static void draw_text(const Font &font, float x, float y,
                       const char *text, float r, float g, float b) {
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, font.tex);
    glColor4f(r, g, b, 1.0f);

    glBegin(GL_QUADS);
    float cx = x, cy = y;
    while (*text) {
        if (*text >= 32 && *text < 128) {
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(font.chars, kAtlasW, kAtlasH,
                                *text - 32, &cx, &cy, &q, 0);
            glTexCoord2f(q.s0, q.t0); glVertex2f(q.x0, q.y0);
            glTexCoord2f(q.s1, q.t0); glVertex2f(q.x1, q.y0);
            glTexCoord2f(q.s1, q.t1); glVertex2f(q.x1, q.y1);
            glTexCoord2f(q.s0, q.t1); glVertex2f(q.x0, q.y1);
        }
        ++text;
    }
    glEnd();

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

// Compute baseline y so the text is centred vertically inside a bar.
static float bar_baseline(const Font &font, float bar_y, float bar_h) {
    float text_h = font.ascent + font.descent;
    return bar_y + (bar_h + text_h) * 0.5f - font.descent;
}

// ── GL helpers ────────────────────────────────────────────────────────────────

static GLuint load_texture(const char *path) {
    int w, h, n;
    stbi_set_flip_vertically_on_load(0);
    unsigned char *data = stbi_load(path, &w, &h, &n, 4);
    if (!data) {
        std::fprintf(stderr, "Failed to load texture: %s\n", path);
        return 0;
    }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    return tex;
}

static void draw_quad(float x, float y, float w, float h,
                       float u0, float v0, float u1, float v1) {
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0); glVertex2f(x,     y);
    glTexCoord2f(u1, v0); glVertex2f(x + w, y);
    glTexCoord2f(u1, v1); glVertex2f(x + w, y + h);
    glTexCoord2f(u0, v1); glVertex2f(x,     y + h);
    glEnd();
}

static void fill_rect(float x, float y, float w, float h) {
    glBegin(GL_QUADS);
    glVertex2f(x,     y);     glVertex2f(x + w, y);
    glVertex2f(x + w, y + h); glVertex2f(x,     y + h);
    glEnd();
}

// ── App state ─────────────────────────────────────────────────────────────────

enum class GameState  { Idle, PieceSelected, AwaitingPromotion };
enum class GameResult { Ongoing, WhiteWins, BlackWins, Stalemate };

struct Capture { Color color; PieceType pt; };

struct AppState {
    Position position;
    MoveList moves{};
    int      move_count  = 0;

    GameState state       = GameState::Idle;
    int       selected_sq = -1;
    Square    promo_from  = SQ_NONE;
    Square    promo_to    = SQ_NONE;

    std::vector<Capture> capture_log;

    // Clock
    float  time_left[2]   = { kDefaultTime, kDefaultTime };
    double last_tick      = 0.0;
    bool   clock_running  = false;

    GameResult result     = GameResult::Ongoing;

    bool   pending_click  = false;
    double click_x = 0, click_y = 0;
};

// ── Input ─────────────────────────────────────────────────────────────────────

static void mouse_button_cb(GLFWwindow *win, int button, int action, int /*mods*/) {
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return;
    auto *s = static_cast<AppState *>(glfwGetWindowUserPointer(win));
    glfwGetCursorPos(win, &s->click_x, &s->click_y);
    s->pending_click = true;
}

static void picker_rect(int win_w, int win_h, float &ox, float &oy) {
    float total_w = 4 * kPickSize + 3 * kPickGap;
    ox = (win_w - total_w) * 0.5f;
    oy = (win_h - kPickSize) * 0.5f;
}

static void commit_move(AppState &app, Move m) {
    Color    opponent = (Color)(app.position.side ^ 1);
    UndoInfo undo;
    pos_make_move(&app.position, m, &undo);
    if (undo.captured != PIECE_NB)
        app.capture_log.push_back({opponent, undo.captured});
    app.move_count = movegen_generate(&app.position, &app.moves, GEN_ALL);
    app.state       = GameState::Idle;
    app.selected_sq = -1;

    if (!app.clock_running) {
        app.clock_running = true;
        app.last_tick     = glfwGetTime();
    }

    // move_count is pseudo-legal; count truly legal moves for end-condition detection
    int legal_count = 0;
    for (int i = 0; i < app.move_count; i++)
        if (movegen_is_legal(&app.position, app.moves.moves[i]))
            legal_count++;

    if (legal_count == 0) {
        bool in_check = pos_in_check(&app.position, app.position.side);
        app.result        = in_check
                            ? ((app.position.side == WHITE) ? GameResult::BlackWins : GameResult::WhiteWins)
                            : GameResult::Stalemate;
        app.clock_running = false;
    }
}

static void handle_click(AppState &app, double mx, double my,
                          int board_px, int board_y, int win_w, int win_h) {
    if (app.result != GameResult::Ongoing) return;

    // ── Promotion picker ──────────────────────────────────────────────────
    if (app.state == GameState::AwaitingPromotion) {
        float ox, oy;
        picker_rect(win_w, win_h, ox, oy);
        for (int i = 0; i < 4; i++) {
            float px = ox + i * (kPickSize + kPickGap);
            if (mx < px || mx >= px + kPickSize || my < oy || my >= oy + kPickSize)
                continue;
            PieceType chosen = kPromoOptions[i];
            for (int j = 0; j < app.move_count; j++) {
                Move m = app.moves.moves[j];
                if (MOVE_FROM(m) != app.promo_from)          continue;
                if (MOVE_TO(m)   != app.promo_to)            continue;
                if (!(MOVE_FLAGS(m) & 0x20))                 continue;
                if (promo_piece_from_flags(MOVE_FLAGS(m)) != chosen) continue;
                if (!movegen_is_legal(&app.position, m))     continue;
                commit_move(app, m);
                return;
            }
        }
        app.state       = GameState::Idle;
        app.selected_sq = -1;
        return;
    }

    // ── Board click ───────────────────────────────────────────────────────
    double bmy = my - board_y;
    if (bmy < 0 || bmy >= board_px || mx < 0 || mx >= board_px) {
        app.state       = GameState::Idle;
        app.selected_sq = -1;
        return;
    }

    float  sq_size = board_px / 8.0f;
    int    file    = (int)(mx / sq_size);
    int    rank    = 7 - (int)(bmy / sq_size);
    Square clicked = (Square)(rank * 8 + file);

    auto is_own_piece = [&](Square s) -> bool {
        if (pos_piece_on(&app.position, s) == PIECE_NB) return false;
        return !!(app.position.by_color[app.position.side] & (1ULL << s));
    };

    if (app.state == GameState::Idle) {
        if (is_own_piece(clicked)) {
            app.selected_sq = (int)clicked;
            app.state       = GameState::PieceSelected;
        }
        return;
    }

    // ── PieceSelected ─────────────────────────────────────────────────────
    bool has_promo  = false;
    Move best_quiet = MOVE_NONE;
    for (int i = 0; i < app.move_count; i++) {
        Move m = app.moves.moves[i];
        if (MOVE_FROM(m) != (Square)app.selected_sq) continue;
        if (MOVE_TO(m)   != clicked)                 continue;
        if (!movegen_is_legal(&app.position, m))     continue;
        if (MOVE_FLAGS(m) & 0x20)
            has_promo = true;
        else if (best_quiet == MOVE_NONE)
            best_quiet = m;
    }

    if (has_promo) {
        app.promo_from = (Square)app.selected_sq;
        app.promo_to   = clicked;
        app.state      = GameState::AwaitingPromotion;
        return;
    }
    if (best_quiet != MOVE_NONE) {
        commit_move(app, best_quiet);
        return;
    }

    app.state       = GameState::Idle;
    app.selected_sq = -1;
    if (is_own_piece(clicked)) {
        app.selected_sq = (int)clicked;
        app.state       = GameState::PieceSelected;
    }
}

// ── Rendering helpers ─────────────────────────────────────────────────────────

static void draw_bar(float x, float y, float w, float h, float r, float g, float b) {
    glColor4f(r, g, b, 1.0f);
    fill_rect(x, y, w, h);
}

static void draw_captured_pieces(const std::vector<Capture> &log, Color captured_color,
                                  float bar_x, float bar_y, float bar_w, float bar_h,
                                  GLuint pieces_tex) {
    static const PieceType kOrder[5] = { PAWN, KNIGHT, BISHOP, ROOK, QUEEN };

    int counts[5] = {};
    for (const Capture &c : log) {
        if (c.color != captured_color) continue;
        for (int i = 0; i < 5; i++)
            if (c.pt == kOrder[i]) { counts[i]++; break; }
    }

    float piece_size = bar_h * 0.78f;
    float pad_y      = (bar_h - piece_size) * 0.5f;
    float step       = piece_size * 0.38f;
    float gap        = 10.0f;

    float total_w = 0.0f;
    bool  first   = true;
    for (int i = 0; i < 5; i++) {
        if (counts[i] <= 0) continue;
        if (!first) total_w += gap;
        total_w += (counts[i] - 1) * step + piece_size;
        first = false;
    }
    if (total_w == 0.0f) return;

    float x          = bar_x + bar_w - 10.0f - total_w;
    float y          = bar_y + pad_y;
    int   sprite_row = (captured_color == WHITE) ? 0 : 1;

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, pieces_tex);
    glColor4f(1, 1, 1, 1);

    first = true;
    for (int i = 0; i < 5; i++) {
        if (counts[i] <= 0) continue;
        if (!first) x += gap;
        int   sc = kSpriteCol[kOrder[i]];
        float u0 = sc / 6.0f,         u1 = (sc + 1) / 6.0f;
        float v0 = sprite_row / 2.0f, v1 = (sprite_row + 1) / 2.0f;
        for (int j = 0; j < counts[i]; j++)
            draw_quad(x + j * step, y, piece_size, piece_size, u0, v0, u1, v1);
        x    += (counts[i] - 1) * step + piece_size;
        first = false;
    }

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

static void draw_clock(const Font &font, float time_sec,
                        float bar_x, float bar_y, float bar_w, float bar_h,
                        float bg_r, float bg_g, float bg_b) {
    char buf[16];
    int  t = (int)std::ceil(time_sec);
    std::snprintf(buf, sizeof(buf), "%d:%02d", t / 60, t % 60);

    float x = bar_x + 18.0f;
    float y = bar_baseline(font, bar_y, bar_h);

    // Perceived luminance — use black text on bright backgrounds, white on dark
    float lum = 0.299f * bg_r + 0.587f * bg_g + 0.114f * bg_b;
    float fg  = lum > 0.45f ? 0.05f : 0.95f;
    draw_text(font, x, y, buf, fg, fg, fg);
}

static void draw_promotion_picker(const Position &pos, int win_w, int win_h,
                                   GLuint pieces_tex) {
    float ox, oy;
    picker_rect(win_w, win_h, ox, oy);
    float total_w = 4 * kPickSize + 3 * kPickGap;

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.55f);
    fill_rect(0, 0, (float)win_w, (float)win_h);

    float pad = 12.0f;
    glColor4f(0.22f, 0.22f, 0.22f, 1.0f);
    fill_rect(ox - pad, oy - pad, total_w + pad * 2.0f, kPickSize + pad * 2.0f);

    int sprite_row = (pos.side == WHITE) ? 0 : 1;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, pieces_tex);
    glColor4f(1, 1, 1, 1);
    for (int i = 0; i < 4; i++) {
        int   sc = kSpriteCol[kPromoOptions[i]];
        float u0 = sc / 6.0f,         u1 = (sc + 1) / 6.0f;
        float v0 = sprite_row / 2.0f, v1 = (sprite_row + 1) / 2.0f;
        draw_quad(ox + i * (kPickSize + kPickGap), oy, kPickSize, kPickSize, u0, v0, u1, v1);
    }

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "Failed to initialize GLFW\n");
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    GLFWwindow *window = glfwCreateWindow(720, 720 + 2 * kBarH, "glfwchess", nullptr, nullptr);
    glfwSetWindowAspectRatio(window, 720, 720 + 2 * kBarH);
    if (!window) {
        std::fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    bb_init();
    magic_init();

    AppState app{};
    pos_from_fen(&app.position, FEN_STARTPOS);
    app.move_count = movegen_generate(&app.position, &app.moves, GEN_ALL);

    glfwSetWindowUserPointer(window, &app);
    glfwSetMouseButtonCallback(window, mouse_button_cb);

    GLuint board_tex  = load_texture("board.png");
    GLuint pieces_tex = load_texture("pieces.png");

    // Load font — try the candidates copied to the build dir
    Font font;
    bool font_ok = false;
    for (const char *name : { "consola.ttf", "cour.ttf", "arial.ttf" }) {
        if (font_init(font, name, 52.0f)) { font_ok = true; break; }
    }
    if (!font_ok)
        std::fprintf(stderr, "Warning: no font loaded, clocks will not display\n");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // ── Tick clock ───────────────────────────────────────────────────
        double now = glfwGetTime();
        if (app.clock_running) {
            float dt = (float)(now - app.last_tick);
            float &t = app.time_left[app.position.side];
            t = std::max(0.0f, t - dt);
            if (t == 0.0f) {
                app.result        = (app.position.side == WHITE) ? GameResult::BlackWins
                                                                  : GameResult::WhiteWins;
                app.clock_running = false;
            }
        }
        app.last_tick = now;

        // ── Process click ─────────────────────────────────────────────────
        if (app.pending_click) {
            app.pending_click = false;
            int w, h;
            glfwGetWindowSize(window, &w, &h);
            int board_px = std::min(w, h - 2 * kBarH);
            handle_click(app, app.click_x, app.click_y, board_px, kBarH, w, h);
        }

        // ── Geometry ──────────────────────────────────────────────────────
        int win_w, win_h;
        glfwGetWindowSize(window, &win_w, &win_h);
        int   board_px = std::min(win_w, win_h - 2 * kBarH);
        float sq       = board_px / 8.0f;
        float board_y  = (float)kBarH;

        int fb_w, fb_h;
        glfwGetFramebufferSize(window, &fb_w, &fb_h);
        glViewport(0, 0, fb_w, fb_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, win_w, win_h, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glDisable(GL_TEXTURE_2D);

        // ── Bars ──────────────────────────────────────────────────────────
        float black_bar_y = 0.0f;
        float white_bar_y = board_y + board_px;

        // Determine bar colours based on game result
        auto bar_rgb = [&](Color side) -> std::tuple<float,float,float> {
            switch (app.result) {
                case GameResult::WhiteWins:
                    return side == WHITE ? std::make_tuple(0.10f, 0.52f, 0.14f)  // green
                                        : std::make_tuple(0.55f, 0.07f, 0.07f); // red
                case GameResult::BlackWins:
                    return side == BLACK ? std::make_tuple(0.10f, 0.52f, 0.14f)
                                        : std::make_tuple(0.55f, 0.07f, 0.07f);
                case GameResult::Stalemate:
                    return std::make_tuple(0.45f, 0.40f, 0.05f); // dim gold
                default:
                    return side == WHITE ? std::make_tuple(0.92f, 0.92f, 0.92f)
                                        : std::make_tuple(0.10f, 0.10f, 0.10f);
            }
        };

        auto [br, bg, bb] = bar_rgb(BLACK);
        auto [wr, wg, wb] = bar_rgb(WHITE);
        draw_bar(0, black_bar_y, (float)win_w, (float)kBarH, br, bg, bb);
        draw_bar(0, white_bar_y, (float)win_w, (float)kBarH, wr, wg, wb);

        if (font_ok) {
            draw_clock(font, app.time_left[BLACK], 0, black_bar_y, (float)win_w, (float)kBarH, br, bg, bb);
            draw_clock(font, app.time_left[WHITE], 0, white_bar_y, (float)win_w, (float)kBarH, wr, wg, wb);
        }

        draw_captured_pieces(app.capture_log, WHITE, 0, black_bar_y, (float)win_w, (float)kBarH, pieces_tex);
        draw_captured_pieces(app.capture_log, BLACK, 0, white_bar_y, (float)win_w, (float)kBarH, pieces_tex);

        // ── Board ─────────────────────────────────────────────────────────
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, board_tex);
        glColor4f(1, 1, 1, 1);
        draw_quad(0, board_y, (float)board_px, (float)board_px, 0, 0, 1, 1);

        // ── Highlights ────────────────────────────────────────────────────
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        if (app.selected_sq >= 0) {
            int sf = app.selected_sq % 8, sr = app.selected_sq / 8;
            glColor4f(0.9f, 0.9f, 0.1f, 0.5f);
            fill_rect(sf * sq, board_y + (7 - sr) * sq, sq, sq);

            glColor4f(0.2f, 0.85f, 0.2f, 0.4f);
            for (int i = 0; i < app.move_count; i++) {
                Move m = app.moves.moves[i];
                if (MOVE_FROM(m) != (Square)app.selected_sq) continue;
                if (!movegen_is_legal(&app.position, m))     continue;
                Square to = MOVE_TO(m);
                fill_rect((to % 8) * sq, board_y + (7 - to / 8) * sq, sq, sq);
            }
        }

        glDisable(GL_BLEND);

        // ── Pieces ────────────────────────────────────────────────────────
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindTexture(GL_TEXTURE_2D, pieces_tex);

        for (int rank = 0; rank < 8; rank++) {
            for (int file = 0; file < 8; file++) {
                Square    s  = (Square)(rank * 8 + file);
                PieceType pt = pos_piece_on(&app.position, s);
                if (pt == PIECE_NB) continue;
                Color col = (app.position.by_color[WHITE] & (1ULL << s)) ? WHITE : BLACK;
                int   sc  = kSpriteCol[pt], sr = (col == WHITE) ? 0 : 1;
                glColor4f(1, 1, 1, 1);
                draw_quad(file * sq, board_y + (7 - rank) * sq, sq, sq,
                           sc / 6.0f, sr / 2.0f, (sc + 1) / 6.0f, (sr + 1) / 2.0f);
            }
        }

        glDisable(GL_BLEND);
        glDisable(GL_TEXTURE_2D);

        // ── Promotion overlay (topmost) ───────────────────────────────────
        if (app.state == GameState::AwaitingPromotion)
            draw_promotion_picker(app.position, win_w, win_h, pieces_tex);

        glfwSwapBuffers(window);
    }

    glDeleteTextures(1, &board_tex);
    glDeleteTextures(1, &pieces_tex);
    if (font_ok) glDeleteTextures(1, &font.tex);
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
