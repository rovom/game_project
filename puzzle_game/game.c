#include "game.h"
#include "assets.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int rect_contains(const SDL_Rect *r, int x, int y)
{
    return (x >= r->x && x < r->x + r->w &&
            y >= r->y && y < r->y + r->h);
}

int rect_overlap_pct(const SDL_Rect *a, const SDL_Rect *b)
{
    int ix1 = (a->x > b->x) ? a->x : b->x;
    int iy1 = (a->y > b->y) ? a->y : b->y;
    int ix2 = (a->x+a->w < b->x+b->w) ? a->x+a->w : b->x+b->w;
    int iy2 = (a->y+a->h < b->y+b->h) ? a->y+a->h : b->y+b->h;
    if (ix2 <= ix1 || iy2 <= iy1) return 0;
    int inter = (ix2-ix1) * (iy2-iy1);
    int area_a = a->w * a->h;
    if (area_a == 0) return 0;
    return inter * 100 / area_a;
}

static void shuffle3(int arr[3])
{
    for (int i = 2; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = arr[i]; arr[i] = arr[j]; arr[j] = tmp;
    }
}

static SDL_Texture *load_random_bg(SDL_Renderer *renderer)
{
    const char *paths[] = BG_IMAGE_PATHS;
    int idx = rand() % BG_IMAGE_COUNT;
    SDL_Surface *surf = IMG_Load(paths[idx]);
    if (!surf) {
        fprintf(stderr, "IMG_Load failed for bg: %s\n", IMG_GetError());
        return NULL;
    }
    SDL_Texture *tex = SDL_CreateTexture(renderer,
                        SDL_PIXELFORMAT_RGBA8888,
                        SDL_TEXTUREACCESS_TARGET,
                        WINDOW_W, WINDOW_H);
    if (!tex) {
        SDL_FreeSurface(surf);
        return NULL;
    }
    SDL_Texture *src_tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);
    if (!src_tex) {
        SDL_DestroyTexture(tex);
        return NULL;
    }

    SDL_SetRenderTarget(renderer, tex);
    SDL_Rect dst = {0, 0, WINDOW_W, WINDOW_H};
    SDL_RenderCopy(renderer, src_tex, NULL, &dst);
    SDL_SetRenderTarget(renderer, NULL);
    SDL_DestroyTexture(src_tex);
    return tex;
}

static SDL_Texture *build_puzzle_tex(SDL_Renderer *renderer, SDL_Texture *bg,
                                     int hole_x, int hole_y)
{
    SDL_Texture *tex = SDL_CreateTexture(renderer,
                        SDL_PIXELFORMAT_RGBA8888,
                        SDL_TEXTUREACCESS_TARGET,
                        WINDOW_W, WINDOW_H);
    if (!tex) return NULL;

    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(renderer, tex);
    SDL_RenderCopy(renderer, bg, NULL, NULL);

    SDL_Rect hole = {hole_x, hole_y, MISSING_W, MISSING_H};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer, 10, 10, 20, 255);
    SDL_RenderFillRect(renderer, &hole);

    SDL_SetRenderDrawColor(renderer, 0, 220, 255, 255);
    int dash = 10, gap = 6;
    for (int x = hole.x; x < hole.x + hole.w; x += dash + gap) {
        int ex = x + dash; if (ex > hole.x + hole.w) ex = hole.x + hole.w;
        SDL_RenderDrawLine(renderer, x, hole.y, ex, hole.y);
        SDL_RenderDrawLine(renderer, x, hole.y + hole.h - 1, ex, hole.y + hole.h - 1);
    }
    for (int y = hole.y; y < hole.y + hole.h; y += dash + gap) {
        int ey = y + dash; if (ey > hole.y + hole.h) ey = hole.y + hole.h;
        SDL_RenderDrawLine(renderer, hole.x,              y, hole.x,              ey);
        SDL_RenderDrawLine(renderer, hole.x + hole.w - 1, y, hole.x + hole.w - 1, ey);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderTarget(renderer, NULL);
    return tex;
}

static SDL_Texture *crop_piece(SDL_Renderer *renderer, SDL_Texture *bg,
                                int src_x, int src_y)
{
    SDL_Texture *tex = SDL_CreateTexture(renderer,
                        SDL_PIXELFORMAT_RGBA8888,
                        SDL_TEXTUREACCESS_TARGET,
                        MISSING_W, MISSING_H);
    if (!tex) return NULL;

    SDL_SetRenderTarget(renderer, tex);
    SDL_Rect src = {src_x, src_y, MISSING_W, MISSING_H};
    SDL_Rect dst = {0, 0, MISSING_W, MISSING_H};
    SDL_RenderCopy(renderer, bg, &src, &dst);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 180);
    SDL_RenderDrawRect(renderer, &dst);

    SDL_SetRenderTarget(renderer, NULL);
    return tex;
}

static void set_feedback(GameContext *ctx, const char *msg, Uint8 r, Uint8 g, Uint8 b)
{
    if (ctx->tex_feedback) {
        SDL_DestroyTexture(ctx->tex_feedback);
        ctx->tex_feedback = NULL;
    }
    if (!ctx->font_large) return;

    SDL_Color col = {r, g, b, 255};
    SDL_Surface *surf = TTF_RenderText_Blended(ctx->font_large, msg, col);
    if (!surf) return;

    ctx->tex_feedback = SDL_CreateTextureFromSurface(ctx->renderer, surf);
    ctx->feedback_rect.w = surf->w;
    ctx->feedback_rect.h = surf->h;
    ctx->feedback_rect.x = (WINDOW_W - surf->w) / 2;
    ctx->feedback_rect.y = (WINDOW_H / 2) - surf->h / 2 - 30;
    SDL_FreeSurface(surf);
    ctx->feedback_anim_start = SDL_GetTicks();
    ctx->feedback_scale      = 0.0f;
}

static int rand_range(int min, int max)
{
    return min + rand() % (max - min + 1);
}

static void pick_wrong_positions(int cx, int cy, int *w1x, int *w1y, int *w2x, int *w2y)
{
    int candidates[6][2] = {
        {40,  40},
        {440, 40},
        {750, 40},
        {40,  280},
        {750, 280},
        {440, 280}
    };

    int used[6] = {0};
    int chosen[2] = {-1, -1};
    int count = 0;

    for (int attempt = 0; attempt < 100 && count < 2; attempt++) {
        int ci = rand() % 6;
        if (used[ci]) continue;
        int px = candidates[ci][0];
        int py = candidates[ci][1];
        if (abs(px - cx) < MISSING_W + 20 && abs(py - cy) < MISSING_H + 20) continue;
        if (count == 1 &&
            abs(px - candidates[chosen[0]][0]) < MISSING_W + 20 &&
            abs(py - candidates[chosen[0]][1]) < MISSING_H + 20) continue;
        used[ci] = 1;
        chosen[count++] = ci;
    }

    if (count < 1) { chosen[0] = 0; }
    if (count < 2) { chosen[1] = (chosen[0] == 0) ? 1 : 0; }

    *w1x = candidates[chosen[0]][0];
    *w1y = candidates[chosen[0]][1];
    *w2x = candidates[chosen[1]][0];
    *w2y = candidates[chosen[1]][1];
}

static int build_round(GameContext *ctx)
{
    if (ctx->tex_bg)     { SDL_DestroyTexture(ctx->tex_bg);     ctx->tex_bg     = NULL; }
    if (ctx->tex_puzzle) { SDL_DestroyTexture(ctx->tex_puzzle); ctx->tex_puzzle = NULL; }
    for (int i = 0; i < 3; i++) {
        if (ctx->pieces[i].texture) {
            SDL_DestroyTexture(ctx->pieces[i].texture);
            ctx->pieces[i].texture = NULL;
        }
    }

    ctx->missing_x = rand_range(MISSING_MIN_X, MISSING_MAX_X);
    ctx->missing_y = rand_range(MISSING_MIN_Y, MISSING_MAX_Y);

    ctx->tex_bg = load_random_bg(ctx->renderer);
    if (!ctx->tex_bg) return 0;

    ctx->tex_puzzle = build_puzzle_tex(ctx->renderer, ctx->tex_bg,
                                       ctx->missing_x, ctx->missing_y);
    if (!ctx->tex_puzzle) return 0;

    int w1x, w1y, w2x, w2y;
    pick_wrong_positions(ctx->missing_x, ctx->missing_y, &w1x, &w1y, &w2x, &w2y);

    SDL_Texture *pc  = crop_piece(ctx->renderer, ctx->tex_bg, ctx->missing_x, ctx->missing_y);
    SDL_Texture *pw1 = crop_piece(ctx->renderer, ctx->tex_bg, w1x, w1y);
    SDL_Texture *pw2 = crop_piece(ctx->renderer, ctx->tex_bg, w2x, w2y);
    if (!pc || !pw1 || !pw2) {
        fprintf(stderr, "Failed to crop pieces\n");
        if (pc)  SDL_DestroyTexture(pc);
        if (pw1) SDL_DestroyTexture(pw1);
        if (pw2) SDL_DestroyTexture(pw2);
        return 0;
    }

    SDL_Texture *textures[3] = {pc,  pw1, pw2};
    int          corrects[3] = {1,   0,   0  };
    int          order[3]    = {0,   1,   2  };
    shuffle3(order);

    int total_w      = 3 * TRAY_PIECE_W + 2 * TRAY_GAP;
    int tray_x_start = (WINDOW_W - total_w) / 2;

    for (int i = 0; i < 3; i++) {
        int s = order[i];
        ctx->pieces[i].texture = textures[s];
        ctx->pieces[i].correct = corrects[s];
        ctx->pieces[i].placed  = 0;

        ctx->pieces[i].tray_rect.x = tray_x_start + i * (TRAY_PIECE_W + TRAY_GAP);
        ctx->pieces[i].tray_rect.y = TRAY_Y;
        ctx->pieces[i].tray_rect.w = TRAY_PIECE_W;
        ctx->pieces[i].tray_rect.h = TRAY_PIECE_H;
        ctx->pieces[i].drag_rect   = ctx->pieces[i].tray_rect;

        if (corrects[s]) ctx->correct_index = i;
    }

    ctx->target_rect.x = ctx->missing_x;
    ctx->target_rect.y = ctx->missing_y;
    ctx->target_rect.w = MISSING_W;
    ctx->target_rect.h = MISSING_H;

    ctx->dragging       = -1;
    ctx->start_ticks    = SDL_GetTicks();
    ctx->timer_fraction = 1.0f;
    ctx->state          = STATE_PLAYING;

    return 1;
}

static int game_init_common(GameContext *ctx, SDL_Renderer *renderer, int embedded)
{
    memset(ctx, 0, sizeof(*ctx));
    srand((unsigned)time(NULL));

    ctx->present_on_render = embedded ? 0 : 1;

    if (!embedded) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
            fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
            return 0;
        }
        ctx->owns_sdl = 1;

        ctx->window = SDL_CreateWindow(WINDOW_TITLE,
                        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                        WINDOW_W, WINDOW_H, SDL_WINDOW_SHOWN);
        if (!ctx->window) {
            fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
            return 0;
        }

        ctx->renderer = SDL_CreateRenderer(ctx->window, -1,
                            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!ctx->renderer) {
            fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
            return 0;
        }
        ctx->owns_window_renderer = 1;
    } else {
        ctx->renderer = renderer;
        if (!ctx->renderer) return 0;
    }

    if (!(IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) & (IMG_INIT_PNG | IMG_INIT_JPG))) {
        fprintf(stderr, "IMG_Init: %s\n", IMG_GetError());
        return 0;
    }
    ctx->owns_img = !embedded;

    if (!TTF_WasInit()) {
        if (TTF_Init() < 0) {
            fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
            return 0;
        }
        ctx->owns_ttf = 1;
    }

#ifdef USE_MIXER
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        fprintf(stderr, "Mix_OpenAudio: %s\n", Mix_GetError());
    } else {
        ctx->sfx_success = Mix_LoadWAV(SFX_SUCCESS_PATH);
        ctx->sfx_fail    = Mix_LoadWAV(SFX_FAIL_PATH);
        ctx->sfx_pick    = Mix_LoadWAV(SFX_PICK_PATH);
    }
#endif

    ctx->font_large = TTF_OpenFont(FONT_PATH, 42);
    ctx->font_small = TTF_OpenFont(FONT_PATH, 22);
    if (!ctx->font_large || !ctx->font_small) {
        ctx->font_large = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf", 42);
        ctx->font_small = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 22);
    }
    if (!ctx->font_large || !ctx->font_small) {
        fprintf(stderr, "TTF_OpenFont: %s\n", TTF_GetError());
        return 0;
    }

    return build_round(ctx);
}

int game_init(GameContext *ctx)
{
    return game_init_common(ctx, NULL, 0);
}

int game_init_embedded(GameContext *ctx, SDL_Renderer *renderer)
{
    return game_init_common(ctx, renderer, 1);
}

void game_destroy(GameContext *ctx)
{
    if (!ctx) return;

    for (int i = 0; i < 3; i++)
        if (ctx->pieces[i].texture) SDL_DestroyTexture(ctx->pieces[i].texture);

    if (ctx->tex_puzzle)   SDL_DestroyTexture(ctx->tex_puzzle);
    if (ctx->tex_bg)       SDL_DestroyTexture(ctx->tex_bg);
    if (ctx->tex_feedback) SDL_DestroyTexture(ctx->tex_feedback);
    if (ctx->font_large)   TTF_CloseFont(ctx->font_large);
    if (ctx->font_small)   TTF_CloseFont(ctx->font_small);

#ifdef USE_MIXER
    if (ctx->sfx_success) Mix_FreeChunk(ctx->sfx_success);
    if (ctx->sfx_fail)    Mix_FreeChunk(ctx->sfx_fail);
    if (ctx->sfx_pick)    Mix_FreeChunk(ctx->sfx_pick);
    if (ctx->owns_audio) Mix_CloseAudio();
#endif

    if (ctx->owns_window_renderer && ctx->renderer) SDL_DestroyRenderer(ctx->renderer);
    if (ctx->owns_window_renderer && ctx->window)   SDL_DestroyWindow(ctx->window);

    if (ctx->owns_ttf) TTF_Quit();
    if (ctx->owns_img) IMG_Quit();
    if (ctx->owns_sdl) SDL_Quit();
}

void game_restart(GameContext *ctx)
{
    if (ctx->tex_feedback) {
        SDL_DestroyTexture(ctx->tex_feedback);
        ctx->tex_feedback = NULL;
    }
    ctx->dragging = -1;
    build_round(ctx);
}

void game_handle_event(GameContext *ctx, const SDL_Event *e)
{
    if (e->type == SDL_KEYDOWN) {
        if (e->key.keysym.sym == SDLK_ESCAPE) {
            ctx->state = STATE_FAIL;
            set_feedback(ctx, "PUZZLE FAILED", 255, 80, 80);
            return;
        }
        if (e->key.keysym.sym == SDLK_r) game_restart(ctx);
        return;
    }

    if (ctx->state != STATE_PLAYING) return;

    if (e->type == SDL_MOUSEBUTTONDOWN && e->button.button == SDL_BUTTON_LEFT) {
        int mx = e->button.x, my = e->button.y;
        for (int i = 0; i < 3; i++) {
            if (!ctx->pieces[i].placed &&
                rect_contains(&ctx->pieces[i].drag_rect, mx, my))
            {
                ctx->dragging      = i;
                ctx->drag_offset_x = mx - ctx->pieces[i].drag_rect.x;
                ctx->drag_offset_y = my - ctx->pieces[i].drag_rect.y;
#ifdef USE_MIXER
                if (ctx->sfx_pick) Mix_PlayChannel(-1, ctx->sfx_pick, 0);
#endif
                break;
            }
        }
    }

    if (e->type == SDL_MOUSEMOTION && ctx->dragging >= 0) {
        ctx->pieces[ctx->dragging].drag_rect.x = e->motion.x - ctx->drag_offset_x;
        ctx->pieces[ctx->dragging].drag_rect.y = e->motion.y - ctx->drag_offset_y;
    }

    if (e->type == SDL_MOUSEBUTTONUP && e->button.button == SDL_BUTTON_LEFT
        && ctx->dragging >= 0)
    {
        int idx = ctx->dragging;
        ctx->dragging = -1;

        int pct = rect_overlap_pct(&ctx->target_rect, &ctx->pieces[idx].drag_rect);
        if (pct >= 70) {
            ctx->pieces[idx].drag_rect.x = ctx->target_rect.x
                + (ctx->target_rect.w - ctx->pieces[idx].drag_rect.w) / 2;
            ctx->pieces[idx].drag_rect.y = ctx->target_rect.y
                + (ctx->target_rect.h - ctx->pieces[idx].drag_rect.h) / 2;

            if (ctx->pieces[idx].correct) {
                ctx->pieces[idx].placed = 1;
                ctx->state = STATE_SUCCESS;
                set_feedback(ctx, "PUZZLE SOLVED!", 0, 255, 160);
#ifdef USE_MIXER
                if (ctx->sfx_success) Mix_PlayChannel(-1, ctx->sfx_success, 0);
#endif
            } else {
                ctx->state = STATE_FAIL;
                set_feedback(ctx, "WRONG PIECE -- PRESS R", 255, 80, 80);
#ifdef USE_MIXER
                if (ctx->sfx_fail) Mix_PlayChannel(-1, ctx->sfx_fail, 0);
#endif
            }
        } else {
            ctx->pieces[idx].drag_rect = ctx->pieces[idx].tray_rect;
        }
    }
}

void game_update(GameContext *ctx)
{
    if (ctx->state != STATE_PLAYING) return;

    Uint32 elapsed = SDL_GetTicks() - ctx->start_ticks;
    ctx->timer_fraction = 1.0f - ((float)elapsed / (TIMER_SECONDS * 1000.0f));
    if (ctx->timer_fraction < 0.0f) {
        ctx->timer_fraction = 0.0f;
        ctx->state = STATE_TIMEOUT;
        set_feedback(ctx, "TIME OUT -- PRESS R", 255, 200, 0);
    }
}

void game_render(GameContext *ctx)
{
    SDL_Renderer *r = ctx->renderer;

    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    SDL_RenderClear(r);

    SDL_RenderCopy(r, ctx->tex_puzzle, NULL, NULL);

    SDL_Rect tray_bg = {0, TRAY_Y - 16, WINDOW_W, WINDOW_H - TRAY_Y + 16};
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 10, 14, 35, 220);
    SDL_RenderFillRect(r, &tray_bg);
    SDL_SetRenderDrawColor(r, 0, 180, 255, 200);
    SDL_RenderDrawLine(r, 0, TRAY_Y - 16, WINDOW_W, TRAY_Y - 16);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    if (ctx->font_small && ctx->state == STATE_PLAYING) {
        SDL_Color col = {0, 180, 255, 255};
        SDL_Surface *s = TTF_RenderText_Blended(ctx->font_small, "SELECT & DRAG THE MISSING PIECE", col);
        if (s) {
            SDL_Texture *t = SDL_CreateTextureFromSurface(r, s);
            SDL_Rect dst = {(WINDOW_W - s->w) / 2, TRAY_Y - 14, s->w, s->h};
            SDL_FreeSurface(s);
            SDL_RenderCopy(r, t, NULL, &dst);
            SDL_DestroyTexture(t);
        }
    }

    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < 3; i++) {
            if (ctx->pieces[i].placed) continue;
            int is_dragged = (ctx->dragging == i);
            if ((pass == 0 && is_dragged) || (pass == 1 && !is_dragged)) continue;

            if (is_dragged) {
                SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(r, 0, 0, 0, 100);
                SDL_Rect shadow = ctx->pieces[i].drag_rect;
                shadow.x += 6; shadow.y += 6;
                SDL_RenderFillRect(r, &shadow);
                SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
            }

            SDL_RenderCopy(r, ctx->pieces[i].texture, NULL, &ctx->pieces[i].drag_rect);

            if (is_dragged) {
                int pct = rect_overlap_pct(&ctx->target_rect, &ctx->pieces[i].drag_rect);
                if (pct > 30) {
                    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(r, 0, 255, 180, 200);
                    SDL_Rect border = ctx->pieces[i].drag_rect;
                    for (int t = 0; t < 3; t++) {
                        border.x--; border.y--; border.w += 2; border.h += 2;
                        SDL_RenderDrawRect(r, &border);
                    }
                    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
                }
            }
        }
    }

    for (int i = 0; i < 3; i++) {
        if (ctx->pieces[i].placed) {
            SDL_RenderCopy(r, ctx->pieces[i].texture, NULL, &ctx->target_rect);
        }
    }

    {
        SDL_Rect bar_bg = {TIMER_BAR_X, TIMER_BAR_Y, TIMER_BAR_W, TIMER_BAR_H};
        SDL_SetRenderDrawColor(r, 30, 30, 60, 255);
        SDL_RenderFillRect(r, &bar_bg);

        float f = ctx->timer_fraction;
        Uint8 br  = (f > 0.5f) ? (Uint8)((1.0f - f) * 2 * 255) : 255;
        Uint8 bg2 = (f > 0.5f) ? 255 : (Uint8)(f * 2 * 255);
        int bar_w = (int)(TIMER_BAR_W * f);
        if (bar_w < 0) bar_w = 0;
        SDL_Rect bar_fill = {TIMER_BAR_X, TIMER_BAR_Y, bar_w, TIMER_BAR_H};
        SDL_SetRenderDrawColor(r, br, bg2, 30, 255);
        SDL_RenderFillRect(r, &bar_fill);
        SDL_SetRenderDrawColor(r, 80, 80, 120, 255);
        SDL_RenderDrawRect(r, &bar_bg);
    }

    if (ctx->tex_feedback && ctx->state != STATE_PLAYING) {
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, 0, 0, 0, 160);
        SDL_Rect overlay = {0, 0, WINDOW_W, WINDOW_H};
        SDL_RenderFillRect(r, &overlay);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

        {
            Uint32 elapsed_ms = SDL_GetTicks() - ctx->feedback_anim_start;
            float  scale;
            if (elapsed_ms < 300) {
                float t = (float)elapsed_ms / 300.0f;
                float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
                scale = ease * 1.15f;
            } else if (elapsed_ms < 450) {
                float t = (float)(elapsed_ms - 300) / 150.0f;
                scale = 1.15f - t * 0.15f;
            } else {
                scale = 1.0f;
            }

            int sw = (int)(ctx->feedback_rect.w * scale);
            int sh = (int)(ctx->feedback_rect.h * scale);
            SDL_Rect dst = {
                (WINDOW_W - sw) / 2,
                ctx->feedback_rect.y + (ctx->feedback_rect.h - sh) / 2,
                sw, sh
            };
            SDL_RenderCopy(r, ctx->tex_feedback, NULL, &dst);
        }

        if (ctx->font_small) {
            SDL_Color sub_col = {180, 180, 220, 255};
            SDL_Surface *ss = TTF_RenderText_Blended(ctx->font_small,
                                "Press R to restart   |   ESC to quit", sub_col);
            if (ss) {
                SDL_Texture *st = SDL_CreateTextureFromSurface(r, ss);
                SDL_Rect sdst = {
                    (WINDOW_W - ss->w) / 2,
                    ctx->feedback_rect.y + ctx->feedback_rect.h + 20,
                    ss->w, ss->h
                };
                SDL_FreeSurface(ss);
                SDL_RenderCopy(r, st, NULL, &sdst);
                SDL_DestroyTexture(st);
            }
        }
    }

    if (ctx->present_on_render) SDL_RenderPresent(r);
}

int run_embedded_puzzle(SDL_Renderer *renderer)
{
    GameContext ctx;
    SDL_Event event;
    Uint32 result_started = 0;
    int running = 1;
    int result = 0;

    if (!game_init_embedded(&ctx, renderer)) {
        game_destroy(&ctx);
        return 0;
    }

    SDL_RenderSetLogicalSize(renderer, WINDOW_W, WINDOW_H);
    while (running) {
        game_render(&ctx);
        SDL_RenderPresent(renderer);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
                break;
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE &&
                ctx.state != STATE_PLAYING) {
                running = 0;
                break;
            }
            game_handle_event(&ctx, &event);
        }

        game_update(&ctx);

        if (ctx.state != STATE_PLAYING) {
            if (result_started == 0) result_started = SDL_GetTicks();
            if (SDL_GetTicks() - result_started >= 1200) {
                result = (ctx.state == STATE_SUCCESS);
                running = 0;
            }
        }

        SDL_Delay(16);
    }

    game_destroy(&ctx);
    return result;
}
