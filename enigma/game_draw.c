#include "game.h"

/* ════════════════════════════════════════════════════════
   COLORS
═════════════════════════════════════════════════════════ */
static SDL_Color WHITE  = {240,240,240,255};
static SDL_Color GREY   = {120,130,160,255};
static SDL_Color BLACK  = { 10, 12, 20,255};
static SDL_Color ACCENT = { 80,210,160,255};
static SDL_Color RED    = {220, 60, 60,255};
static SDL_Color GREEN  = { 60,200,100,255};
static SDL_Color YELLOW = {240,200, 50,255};
static SDL_Color DARK   = { 22, 28, 46,255};

/* ════════════════════════════════════════════════════════
   HELPERS
═════════════════════════════════════════════════════════ */

/* draw text at (x,y) */
void txt(Game *g, TTF_Font *f, const char *s, int x, int y, SDL_Color c) {
    if (!f || !s || !s[0]) return;
    SDL_Surface *sur = TTF_RenderUTF8_Blended(f, s, c);
    if (!sur) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(g->ren, sur);
    SDL_Rect dst = {x, y, sur->w, sur->h};
    SDL_FreeSurface(sur);
    if (tex) { SDL_RenderCopy(g->ren, tex, NULL, &dst); SDL_DestroyTexture(tex); }
}

/* draw text centered on cx */
void txtc(Game *g, TTF_Font *f, const char *s, int cx, int y, SDL_Color c) {
    if (!f || !s || !s[0]) return;
    SDL_Surface *sur = TTF_RenderUTF8_Blended(f, s, c);
    if (!sur) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(g->ren, sur);
    SDL_Rect dst = {cx - sur->w/2, y, sur->w, sur->h};
    SDL_FreeSurface(sur);
    if (tex) { SDL_RenderCopy(g->ren, tex, NULL, &dst); SDL_DestroyTexture(tex); }
}

/* filled box with border */
void box(Game *g, int x, int y, int w, int h, SDL_Color fill, SDL_Color border) {
    SDL_Rect r = {x, y, w, h};
    SDL_SetRenderDrawColor(g->ren, fill.r, fill.g, fill.b, fill.a);
    SDL_RenderFillRect(g->ren, &r);
    SDL_SetRenderDrawColor(g->ren, border.r, border.g, border.b, border.a);
    SDL_RenderDrawRect(g->ren, &r);
}

/* rotozoom: renders text scaled+rotated at screen center */
void rotozoom_txt(Game *g, const char *s, SDL_Color c) {
    if (!g->fnt_big) return;
    SDL_Surface *sur = TTF_RenderUTF8_Blended(g->fnt_big, s, c);
    if (!sur) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(g->ren, sur);
    int tw = sur->w, th = sur->h;
    SDL_FreeSurface(sur);
    if (!tex) return;
    int dw = (int)(tw * g->rz_scale), dh = (int)(th * g->rz_scale);
    SDL_Rect dst = {SCREEN_W/2 - dw/2, SCREEN_H/2 - dh/2, dw, dh};
    SDL_RenderCopyEx(g->ren, tex, NULL, &dst, g->rz_angle, NULL, SDL_FLIP_NONE);
    SDL_DestroyTexture(tex);
}

/* animated timer bar (no numbers) */
void timer_bar(Game *g) {
    float ratio = 1.0f - (float)(SDL_GetTicks() - g->t_start) / TIME_LIMIT;
    if (ratio < 0) ratio = 0;
    int X=60, Y=SCREEN_H-30, W=SCREEN_W-120, H=16;
    /* track */
    SDL_SetRenderDrawColor(g->ren, 30,35,55,255);
    SDL_Rect tr = {X,Y,W,H}; SDL_RenderFillRect(g->ren,&tr);
    /* fill: green -> yellow -> red */
    int fw = (int)(W * ratio);
    Uint8 r = (ratio > 0.5f) ? (Uint8)((1-ratio)*2*255) : 220;
    Uint8 gr= (ratio > 0.5f) ? 200 : (Uint8)(ratio*2*180);
    SDL_SetRenderDrawColor(g->ren, r, gr, 30, 255);
    SDL_Rect fi = {X,Y,fw,H}; if(fw>0) SDL_RenderFillRect(g->ren,&fi);
    /* border */
    SDL_SetRenderDrawColor(g->ren, 60,70,110,255);
    SDL_RenderDrawRect(g->ren, &tr);
}

/* background: simple dot grid */
static void bg(Game *g) {
    SDL_SetRenderDrawColor(g->ren, BLACK.r, BLACK.g, BLACK.b, 255);
    SDL_RenderClear(g->ren);
    SDL_SetRenderDrawColor(g->ren, 30, 35, 55, 255);
    for (int x=0; x<SCREEN_W; x+=40)
        for (int y=0; y<SCREEN_H; y+=40)
            SDL_RenderDrawPoint(g->ren, x, y);
}

/* ════════════════════════════════════════════════════════
   DRAW: MENU
═════════════════════════════════════════════════════════ */
void draw_menu(Game *g) {
    bg(g);
    /* title */
    box(g, 200,130, 400,80, DARK, ACCENT);
    txtc(g, g->fnt_big, "ENIGMA", SCREEN_W/2, 148, ACCENT);
    /* subtitle */
    txtc(g, g->fnt_sml, "A quiz adventure", SCREEN_W/2, 230, GREY);

    /* PLAY button */
    box(g, 300,290, 200,55, DARK, ACCENT);
    txtc(g, g->fnt_med, "PLAY  [ENTER]", SCREEN_W/2, 304, WHITE);

    /* QUIT button */
    box(g, 300,365, 200,55, DARK, RED);
    txtc(g, g->fnt_med, "QUIT  [ESC]", SCREEN_W/2, 379, RED);

    /* info */
    char buf[64];
    snprintf(buf,sizeof(buf),"%d questions loaded", g->total);
    txtc(g, g->fnt_sml, buf, SCREEN_W/2, 450, GREY);
}

/* ════════════════════════════════════════════════════════
   DRAW: PLAYING
═════════════════════════════════════════════════════════ */
void draw_playing(Game *g) {
    bg(g);

    if (g->cur < 0) return;
    Question *q = &g->bank[g->cur];

    /* HUD bar */
    box(g, 0,0, SCREEN_W,46, DARK, DARK);
    char buf[64];
    snprintf(buf,sizeof(buf),"Score: %d", g->score);
    txt(g, g->fnt_sml, buf, 12, 12, ACCENT);
    snprintf(buf,sizeof(buf),"Q %d/10", g->asked+1);
    txtc(g, g->fnt_sml, buf, SCREEN_W/2, 14, WHITE);
    /* lives */
    for (int i=0; i<3; i++) {
        SDL_Color hc = (i < g->lives) ? RED : GREY;
        txt(g, g->fnt_sml, (i<g->lives)?"<3":"  ", SCREEN_W-90+i*28, 12, hc);
    }

    /* question box */
    box(g, 40,65, SCREEN_W-80,100, DARK, GREY);
    /* wrapped question text */
    SDL_Surface *qs = TTF_RenderUTF8_Blended_Wrapped(g->fnt_med, q->q, WHITE, SCREEN_W-120);
    if (qs) {
        SDL_Texture *qt = SDL_CreateTextureFromSurface(g->ren, qs);
        SDL_Rect dst = {SCREEN_W/2-qs->w/2, 80, qs->w, qs->h};
        SDL_FreeSurface(qs);
        if (qt) { SDL_RenderCopy(g->ren, qt, NULL, &dst); SDL_DestroyTexture(qt); }
    }

    /* 3 choice buttons */
    char labels[3][4] = {"1.", "2.", "3."};
    for (int i=0; i<3; i++) {
        int by = 185 + i*110;
        box(g, 55,by, SCREEN_W-110,90, DARK, GREY);
        /* number badge */
        box(g, 62,by+8, 40,40, ACCENT, ACCENT);
        txtc(g, g->fnt_med, labels[i], 82, by+14, BLACK);
        /* choice text */
        SDL_Surface *cs = TTF_RenderUTF8_Blended_Wrapped(g->fnt_sml, q->c[i], WHITE, SCREEN_W-200);
        if (cs) {
            SDL_Texture *ct = SDL_CreateTextureFromSurface(g->ren, cs);
            SDL_Rect cd = {115, by+(90-cs->h)/2, cs->w, cs->h};
            SDL_FreeSurface(cs);
            if (ct) { SDL_RenderCopy(g->ren, ct, NULL, &cd); SDL_DestroyTexture(ct); }
        }
    }

    timer_bar(g);
    txtc(g, g->fnt_sml, "Press 1 / 2 / 3  or click a choice", SCREEN_W/2, SCREEN_H-50, GREY);
}

/* ════════════════════════════════════════════════════════
   DRAW: RESULT  (rotozoom success/fail)
═════════════════════════════════════════════════════════ */
void draw_result(Game *g) {
    bg(g);

    /* dim */
    SDL_SetRenderDrawBlendMode(g->ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(g->ren, 0,0,0,140);
    SDL_Rect ov={0,0,SCREEN_W,SCREEN_H}; SDL_RenderFillRect(g->ren,&ov);
    SDL_SetRenderDrawBlendMode(g->ren, SDL_BLENDMODE_NONE);

    /* animate rotozoom */
    if (g->rz_scale < 1.0f) g->rz_scale += 0.06f;
    if (g->rz_scale > 1.0f) g->rz_scale  = 1.0f;
    g->rz_angle += (g->rz_angle > 0) ? -0.8f : 0.8f;
    if (fabsf(g->rz_angle) < 1.0f) g->rz_angle = (g->last_ok ? 8.0f : -8.0f);

    if (g->last_ok) {
        rotozoom_txt(g, "CORRECT!", GREEN);
        txtc(g, g->fnt_med, "+10 pts", SCREEN_W/2, SCREEN_H/2+70, GREEN);
    } else {
        rotozoom_txt(g, "WRONG!", RED);
        /* show correct answer */
        char buf[MAX_STR+12];
        snprintf(buf,sizeof(buf),"Answer: %s", g->bank[g->cur].c[g->bank[g->cur].ans]);
        txtc(g, g->fnt_sml, buf, SCREEN_W/2, SCREEN_H/2+70, WHITE);
    }

    /* lives left */
    if (g->lives <= 0)
        txtc(g, g->fnt_sml, "No lives left!", SCREEN_W/2, SCREEN_H/2+110, RED);

    txtc(g, g->fnt_sml, "SPACE / click  to continue", SCREEN_W/2, SCREEN_H-50, GREY);
}

/* ════════════════════════════════════════════════════════
   DRAW: GAMEOVER
═════════════════════════════════════════════════════════ */
void draw_gameover(Game *g) {
    bg(g);

    /* rotozoom spin */
    if (g->rz_scale < 1.0f) g->rz_scale += 0.025f;
    g->rz_angle += 0.5f;

    SDL_Color col = (g->lives > 0) ? ACCENT : RED;
    rotozoom_txt(g, (g->lives > 0) ? "WELL DONE!" : "GAME OVER", col);

    char buf[64];
    snprintf(buf,sizeof(buf),"Score: %d / 100", g->score);
    txtc(g, g->fnt_med, buf, SCREEN_W/2, SCREEN_H/2+75, WHITE);

    box(g, 300,430, 200,55, DARK, ACCENT);
    txtc(g, g->fnt_med, "AGAIN  [R]", SCREEN_W/2, 444, ACCENT);

    box(g, 300,505, 200,55, DARK, RED);
    txtc(g, g->fnt_med, "MENU  [M]", SCREEN_W/2, 519, RED);
}
