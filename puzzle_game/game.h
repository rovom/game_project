#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#ifdef USE_MIXER
#  include <SDL2/SDL_mixer.h>
#endif

#include "assets.h"

typedef enum {
    STATE_PLAYING = 0,
    STATE_SUCCESS,
    STATE_FAIL,
    STATE_TIMEOUT
} GameState;

typedef struct {
    SDL_Texture *texture;
    SDL_Rect     tray_rect;
    SDL_Rect     drag_rect;
    int          correct;
    int          placed;
} Piece;

typedef struct {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    int           owns_sdl;
    int           owns_img;
    int           owns_ttf;
    int           owns_audio;
    int           owns_window_renderer;
    int           present_on_render;

    SDL_Texture  *tex_bg;
    SDL_Texture  *tex_puzzle;

    TTF_Font     *font_large;
    TTF_Font     *font_small;

    Piece         pieces[3];
    int           correct_index;

    int           dragging;
    int           drag_offset_x;
    int           drag_offset_y;

    SDL_Rect      target_rect;

    int           missing_x;
    int           missing_y;

    Uint32        start_ticks;
    float         timer_fraction;

    GameState     state;

    SDL_Texture  *tex_feedback;
    SDL_Rect      feedback_rect;
    Uint32        feedback_anim_start; 
    float         feedback_scale;     

#ifdef USE_MIXER
    Mix_Chunk    *sfx_success;
    Mix_Chunk    *sfx_fail;
    Mix_Chunk    *sfx_pick;
#endif

} GameContext;

int  game_init   (GameContext *ctx);
int  game_init_embedded(GameContext *ctx, SDL_Renderer *renderer);
void game_destroy(GameContext *ctx);

void game_handle_event (GameContext *ctx, const SDL_Event *e);
void game_update       (GameContext *ctx);
void game_render       (GameContext *ctx);
int  run_embedded_puzzle(SDL_Renderer *renderer);

int  rect_contains    (const SDL_Rect *r, int x, int y);
int  rect_overlap_pct (const SDL_Rect *a, const SDL_Rect *b);

void game_restart(GameContext *ctx);

#endif
