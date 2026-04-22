#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* ── Constants ───────────────────────────────────────── */
#define SCREEN_W       800
#define SCREEN_H       600
#define MAX_Q          60
#define MAX_STR        256
#define TIME_LIMIT     15000   /* ms per question          */
#define FONT_PATH      "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
#define QUESTIONS_FILE "questions.txt"

/* ── States ──────────────────────────────────────────── */
typedef enum { S_MENU, S_PLAYING, S_RESULT, S_GAMEOVER } State;

/* ── One question (3 choices) ────────────────────────── */
typedef struct {
    char q[MAX_STR];         /* question text              */
    char c[3][MAX_STR];      /* 3 choices                  */
    int  ans;                /* correct index (0,1,2)      */
    int  seen;               /* already used this round?   */
} Question;

/* ── Whole game context ──────────────────────────────── */
typedef struct {
    /* data */
    Question   bank[MAX_Q];
    int        total;        /* questions loaded           */

    /* session */
    State      state;
    int        cur;          /* index of current question  */
    int        asked;        /* questions asked so far     */
    int        score;
    int        lives;
    int        last_ok;      /* 1=correct 0=wrong/timeout  */

    /* timer */
    Uint32     t_start;

    /* rotozoom */
    float      rz_scale;
    float      rz_angle;

    /* SDL */
    SDL_Window   *win;
    SDL_Renderer *ren;
    TTF_Font     *fnt_big;
    TTF_Font     *fnt_med;
    TTF_Font     *fnt_sml;
} Game;

/* ── Prototypes (all in game.c) ──────────────────────── */
void game_init   (Game *g);
void game_load   (Game *g);
void game_start  (Game *g);
void game_pick   (Game *g);
void game_answer (Game *g, int choice);
void game_tick   (Game *g);

void draw_menu    (Game *g);
void draw_playing (Game *g);
void draw_result  (Game *g);
void draw_gameover(Game *g);

void ev_menu    (Game *g, SDL_Event *e);
void ev_playing (Game *g, SDL_Event *e);
void ev_result  (Game *g, SDL_Event *e);
void ev_gameover(Game *g, SDL_Event *e);

/* helpers */
void txt  (Game *g, TTF_Font *f, const char *s, int x, int y, SDL_Color c);
void txtc (Game *g, TTF_Font *f, const char *s, int cx, int y, SDL_Color c);
void box  (Game *g, int x, int y, int w, int h, SDL_Color fill, SDL_Color border);
void rotozoom_txt(Game *g, const char *s, SDL_Color c);
void timer_bar   (Game *g);

#endif
