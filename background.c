

#include "background.h"
#include <stdio.h>
#include <string.h>






void init_platforms(Background *bg, int level)
{
    int count;

    bg->level         = level;
    bg->platformCount = 0;

    if (level == 1) {
        Platform lvl1[] = {
            { {  80, 550, 220, 18 }, PLATFORM_FIXED,   0, 1 },
            { { 380, 450, 180, 18 }, PLATFORM_FIXED,   0, 1 },
            { { 620, 340, 200, 18 }, PLATFORM_FIXED,   0, 1 },
            { { 250, 390, 140, 18 }, PLATFORM_MOBILE,  3, 1 },
        };
        count = sizeof(lvl1) / sizeof(lvl1[0]);
        bg->platformCount = count;
        memcpy(bg->platforms, lvl1, sizeof(lvl1));

    } else if (level == 2) {
        Platform lvl2[] = {
            { {  60, 570, 200, 18 }, PLATFORM_FIXED,         0, 1 },
            { { 320, 460, 160, 18 }, PLATFORM_FIXED,         0, 1 },
            { { 520, 360, 150, 18 }, PLATFORM_MOBILE,        4, 1 },
            { { 700, 420, 130, 18 }, PLATFORM_DESTRUCTIBLE,  0, 1 },
            { { 900, 310, 120, 18 }, PLATFORM_DESTRUCTIBLE,  0, 1 },
        };
        count = sizeof(lvl2) / sizeof(lvl2[0]);
        bg->platformCount = count;
        memcpy(bg->platforms, lvl2, sizeof(lvl2));
    }
}






void init_background(Background *bg, SDL_Renderer *renderer, int level)
{
    int y;

    memset(bg, 0, sizeof(*bg));

    bg->screenMode = MODE_SPLIT;

    bg->image = IMG_LoadTexture(renderer, "assets/background.png");
    if (!bg->image) {
        bg->imgW = W_SCREEN * 2;
        bg->imgH = H_SCREEN + 200;
    } else {
        SDL_QueryTexture(bg->image, NULL, NULL, &bg->imgW, &bg->imgH);
    }

    y = (bg->imgH - H_SCREEN) / 2;
    if (y < 0) y = 0;

    bg->posBack1   = (SDL_Rect){ 0,      y, W_HALF,    H_SCREEN };
    bg->posBack2   = (SDL_Rect){ 0,      y, W_HALF,    H_SCREEN };
    bg->posScreen1 = (SDL_Rect){ 0,      0, W_HALF,    H_SCREEN };
    bg->posScreen2 = (SDL_Rect){ W_HALF, 0, W_HALF,    H_SCREEN };

    init_platforms(bg, level);
}


void scroll_background(Background *bg, int numBack, int dx, int dy)
{
    SDL_Rect *cam;

    cam = (numBack == 1) ? &bg->posBack1 : &bg->posBack2;

    cam->x += dx;
    cam->y += dy;

    if (cam->x < 0)                 cam->x = 0;
    if (cam->x + cam->w > bg->imgW) cam->x = bg->imgW - cam->w;
    if (cam->y < 0)                 cam->y = 0;
    if (cam->y + cam->h > bg->imgH) cam->y = bg->imgH - cam->h;
}


void update_platforms(Background *bg)
{
    int i;
    Platform *p;

    for (i = 0; i < bg->platformCount; i++) {
        p = &bg->platforms[i];
        if (!p->alive) continue;

        if (p->type == PLATFORM_MOBILE) {
            p->rect.x += p->dx;

            if (p->rect.x < 0) {
                p->rect.x = 0;
                p->dx = -p->dx;
            }
            if (p->rect.x + p->rect.w > bg->imgW) {
                p->rect.x = bg->imgW - p->rect.w;
                p->dx = -p->dx;
            }
        }
    }
}


void render_background(Background *bg, SDL_Renderer *renderer)
{
    
    if (bg->screenMode == MODE_SINGLE) {
        bg->posBack1.w   = W_SCREEN;
        bg->posScreen1.w = W_SCREEN;
    } else {
        bg->posBack1.w   = W_HALF;
        bg->posScreen1.w = W_HALF;
    }

    if (bg->image) {
        if (bg->screenMode == MODE_SPLIT)
            SDL_RenderCopy(renderer, bg->image, &bg->posBack2, &bg->posScreen2);
        SDL_RenderCopy(renderer, bg->image, &bg->posBack1, &bg->posScreen1);
    } else {
        SDL_SetRenderDrawColor(renderer, 8, 8, 48, 255);
        SDL_RenderFillRect(renderer, &bg->posScreen1);
        if (bg->screenMode == MODE_SPLIT) {
            SDL_SetRenderDrawColor(renderer, 18, 5, 38, 255);
            SDL_RenderFillRect(renderer, &bg->posScreen2);
        }
    }

    
    if (bg->screenMode == MODE_SPLIT) {
        SDL_SetRenderDrawColor(renderer, 210, 210, 210, 255);
        SDL_RenderDrawLine(renderer, W_HALF, 0, W_HALF, H_SCREEN);
    }
}


void render_platforms(Background *bg, SDL_Renderer *renderer)
{
    int i;
    Platform *p;
    SDL_Rect  r1, r2;

    for (i = 0; i < bg->platformCount; i++) {
        p = &bg->platforms[i];
        if (!p->alive) continue;

        if (p->type == PLATFORM_FIXED)
            SDL_SetRenderDrawColor(renderer, 180, 140,  60, 255);
        else if (p->type == PLATFORM_MOBILE)
            SDL_SetRenderDrawColor(renderer,  60, 200, 220, 255);
        else if (p->type == PLATFORM_DESTRUCTIBLE)
            SDL_SetRenderDrawColor(renderer, 220,  70,  70, 255);

        
        r1.x = p->rect.x - bg->posBack1.x;
        r1.y = p->rect.y - bg->posBack1.y;
        r1.w = p->rect.w;
        r1.h = p->rect.h;

        if (bg->screenMode == MODE_SINGLE) {
            
            if (r1.x + r1.w > 0 && r1.x < W_SCREEN) {
                SDL_RenderSetClipRect(renderer, &bg->posScreen1);
                SDL_RenderFillRect(renderer, &r1);
            }
        } else {
            if (r1.x + r1.w > 0 && r1.x < W_HALF) {
                SDL_RenderSetClipRect(renderer, &bg->posScreen1);
                SDL_RenderFillRect(renderer, &r1);
            }

            
            r2.x = p->rect.x - bg->posBack2.x + W_HALF;
            r2.y = p->rect.y - bg->posBack2.y;
            r2.w = p->rect.w;
            r2.h = p->rect.h;

            if (r2.x + r2.w > W_HALF && r2.x < W_SCREEN) {
                SDL_RenderSetClipRect(renderer, &bg->posScreen2);
                SDL_RenderFillRect(renderer, &r2);
            }
        }
    }

    SDL_RenderSetClipRect(renderer, NULL);
}


void free_background(Background *bg)
{
    if (bg->image) {
        SDL_DestroyTexture(bg->image);
        bg->image = NULL;
    }
}






void init_timer(GameTimer *t)
{
    t->startTime = SDL_GetTicks();
    t->elapsed   = 0;
}


void update_timer(GameTimer *t)
{
    t->elapsed = SDL_GetTicks() - t->startTime;
}


void render_timer(GameTimer *t, SDL_Renderer *renderer, TTF_Font *font,
                  int centerX, int y)
{
    Uint32       total_s;
    Uint32       minutes;
    Uint32       seconds;
    char         buf[16];
    SDL_Color    colour;
    SDL_Surface *surf;
    SDL_Texture *tex;
    int          w, h;
    SDL_Rect     dst;

    if (!font) return;

    total_s = t->elapsed / 1000;
    minutes = total_s / 60;
    seconds = total_s % 60;

    snprintf(buf, sizeof(buf), "%02u:%02u", minutes, seconds);

    colour.r = 255; colour.g = 220; colour.b = 100; colour.a = 255;

    surf = TTF_RenderText_Solid(font, buf, colour);
    if (!surf) return;

    tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);
    if (!tex) return;

    SDL_QueryTexture(tex, NULL, NULL, &w, &h);

    dst.x = centerX - w / 2;
    dst.y = y;
    dst.w = w;
    dst.h = h;

    SDL_RenderCopy(renderer, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
}






void fill_rect_alpha(SDL_Renderer *r, SDL_Rect rect,
                     Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, red, green, blue, alpha);
    SDL_RenderFillRect(r, &rect);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}






void init_guide(Guide *g, int player)
{
    g->visible   = 0;
    g->lineCount = 0;

    #define GL(str) snprintf(g->lines[g->lineCount++], 120, "%s", str)

    if (player == 1) {
        GL("╔════════ PLAYER 1 GUIDE ════════╗");
        GL("                                  ");
        GL("  MOVE / SCROLL:                  ");
        GL("  Arrow keys  ← → ↑ ↓            ");
        GL("                                  ");
        GL("  PLATFORMS:                      ");
        GL("  Golden = Fixed                  ");
        GL("  Cyan   = Mobile (bounces)       ");
        GL("  Red    = Destructible           ");
        GL("                                  ");
        GL("  G / F1  Toggle this guide       ");
        GL("  ESC     Quit                    ");
        GL("╚════════════════════════════════╝");
    } else {
        GL("╔════════ PLAYER 2 GUIDE ════════╗");
        GL("                                  ");
        GL("  MOVE / SCROLL:                  ");
        GL("  W A S D                         ");
        GL("                                  ");
        GL("  PLATFORMS:                      ");
        GL("  Golden = Fixed                  ");
        GL("  Cyan   = Mobile (bounces)       ");
        GL("  Red    = Destructible           ");
        GL("                                  ");
        GL("  H      Toggle this guide        ");
        GL("  ESC    Quit                     ");
        GL("╚════════════════════════════════╝");
    }

    #undef GL
}


void toggle_guide(Guide *g)
{
    g->visible = !g->visible;
}


void render_guide(Guide *g, SDL_Renderer *renderer, TTF_Font *font,
                  int offsetX, int maxW)
{
    int          PAD, LINE_H, panelW, panelH;
    int          panelX, panelY;
    SDL_Rect     panel, inner, clip, dst;
    SDL_Color    white, yellow, col;
    SDL_Surface *surf;
    SDL_Texture *tex;
    int          i, tw, th, textX, textY;

    if (!g->visible || !font) return;

    PAD    = 20;
    LINE_H = 24;
    panelW = maxW - 40;
    if (panelW > 620) panelW = 620;
    panelH = g->lineCount * LINE_H + PAD * 2;

    panelX = offsetX + (maxW - panelW) / 2;
    panelY = (H_SCREEN - panelH) / 2;

    clip.x = offsetX;
    clip.y = 0;
    clip.w = maxW;
    clip.h = H_SCREEN;
    SDL_RenderSetClipRect(renderer, &clip);

    panel.x = panelX; panel.y = panelY;
    panel.w = panelW; panel.h = panelH;
    fill_rect_alpha(renderer, panel, 10, 10, 30, 230);

    SDL_SetRenderDrawColor(renderer, 180, 160, 255, 255);
    SDL_RenderDrawRect(renderer, &panel);
    inner.x = panelX + 1; inner.y = panelY + 1;
    inner.w = panelW - 2; inner.h = panelH - 2;
    SDL_RenderDrawRect(renderer, &inner);

    white.r  = 240; white.g  = 240; white.b  = 255; white.a  = 255;
    yellow.r = 255; yellow.g = 220; yellow.b =  80; yellow.a = 255;

    for (i = 0; i < g->lineCount; i++) {
        col = (i == 0 || i == g->lineCount - 1) ? yellow : white;

        surf = TTF_RenderUTF8_Blended(font, g->lines[i], col);
        if (!surf) continue;

        tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_FreeSurface(surf);
        if (!tex) continue;

        SDL_QueryTexture(tex, NULL, NULL, &tw, &th);

        textX = panelX + (panelW - tw) / 2;
        textY = panelY + PAD + i * LINE_H;

        dst.x = textX; dst.y = textY;
        dst.w = tw;    dst.h = th;
        SDL_RenderCopy(renderer, tex, NULL, &dst);
        SDL_DestroyTexture(tex);
    }

    SDL_RenderSetClipRect(renderer, NULL);
}






void init_button(Button *btn, SDL_Renderer *renderer, const char *path,
                 int x, int y, int w, int h)
{
    SDL_Surface *surf;

    btn->hovered = 0;
    btn->pos.x = x; btn->pos.y = y;
    btn->pos.w = w; btn->pos.h = h;

    if (path) {
        surf = IMG_Load(path);
        if (surf) {
            btn->texture = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_FreeSurface(surf);
        } else {
            btn->texture = NULL;
        }
    } else {
        btn->texture = NULL;
    }
}


void check_button_hover(Button *btn, int mx, int my)
{
    SDL_Point p;
    p.x = mx;
    p.y = my;
    btn->hovered = SDL_PointInRect(&p, &btn->pos);
}


int button_clicked(Button *btn, int mx, int my)
{
    SDL_Point p;
    p.x = mx;
    p.y = my;
    return SDL_PointInRect(&p, &btn->pos);
}


void render_button(Button *btn, SDL_Renderer *renderer, TTF_Font *font,
                   const char *label)
{
    SDL_Rect     dest;
    SDL_Color    col;
    SDL_Surface *surf;
    SDL_Texture *tex;
    int          tw, th;
    SDL_Rect     tdst;

    if (!btn->pos.w || !btn->pos.h) return;

    dest = btn->pos;

    
    if (btn->hovered) {
        dest.w = (int)(btn->pos.w * 1.08f);
        dest.h = (int)(btn->pos.h * 1.08f);
        dest.x = btn->pos.x - (dest.w - btn->pos.w) / 2;
        dest.y = btn->pos.y - (dest.h - btn->pos.h) / 2;
    }

    if (btn->texture) {
        SDL_RenderCopy(renderer, btn->texture, NULL, &dest);
    } else {
        
        if (btn->hovered)
            SDL_SetRenderDrawColor(renderer, 120, 200, 120, 255);
        else
            SDL_SetRenderDrawColor(renderer,  80, 160,  80, 255);
        SDL_RenderFillRect(renderer, &dest);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &dest);

        if (font && label) {
            col.r = 255; col.g = 255; col.b = 255; col.a = 255;
            surf = TTF_RenderText_Solid(font, label, col);
            if (surf) {
                tex = SDL_CreateTextureFromSurface(renderer, surf);
                SDL_FreeSurface(surf);
                if (tex) {
                    SDL_QueryTexture(tex, NULL, NULL, &tw, &th);
                    tdst.x = dest.x + (dest.w - tw) / 2;
                    tdst.y = dest.y + (dest.h - th) / 2;
                    tdst.w = tw;
                    tdst.h = th;
                    SDL_RenderCopy(renderer, tex, NULL, &tdst);
                    SDL_DestroyTexture(tex);
                }
            }
        }
    }
}


void free_button(Button *btn)
{
    if (btn->texture) {
        SDL_DestroyTexture(btn->texture);
        btn->texture = NULL;
    }
}
