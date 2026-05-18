

#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#define W_SCREEN        1280
#define H_SCREEN         650
#define W_HALF          (W_SCREEN / 2)
#define SCROLL_SPEED       4
#define MAX_PLATFORMS     20

#define PLATFORM_FIXED        0
#define PLATFORM_MOBILE       1
#define PLATFORM_DESTRUCTIBLE 2


#define MODE_SPLIT  0
#define MODE_SINGLE 1


typedef struct {
    SDL_Rect rect; 
    int type; 
    int dx; 
    int alive; 
} Platform;


typedef struct {
    SDL_Texture *image; 
    int imgW; 
    int imgH; 
    SDL_Rect posBack1; 
    SDL_Rect posBack2; 
    SDL_Rect posScreen1; 
    SDL_Rect posScreen2; 
    Platform platforms[MAX_PLATFORMS]; 
    int platformCount; 
    int level; 
    int screenMode; 
} Background;


typedef struct {
    Uint32 startTime; 
    Uint32 elapsed; 
} GameTimer;

#define GUIDE_MAX_LINES 20


typedef struct {
    int visible; 
    char lines[GUIDE_MAX_LINES][120]; 
    int lineCount; 
} Guide;


typedef struct {
    SDL_Rect      pos; 
    SDL_Texture  *texture; 
    int           hovered; 
} Button;


void init_background(Background *bg, SDL_Renderer *renderer, int level);
void init_platforms(Background *bg, int level);
void scroll_background(Background *bg, int numBack, int dx, int dy);
void update_platforms(Background *bg);
void render_background(Background *bg, SDL_Renderer *renderer);
void render_platforms(Background *bg, SDL_Renderer *renderer);
void free_background(Background *bg);


void init_timer(GameTimer *t);
void update_timer(GameTimer *t);
void render_timer(GameTimer *t, SDL_Renderer *renderer, TTF_Font *font,
                  int centerX, int y);


void init_guide(Guide *g, int player);
void toggle_guide(Guide *g);
void render_guide(Guide *g, SDL_Renderer *renderer, TTF_Font *font,
                  int offsetX, int maxW);


void fill_rect_alpha(SDL_Renderer *r, SDL_Rect rect,
                     Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha);


void init_button(Button *btn, SDL_Renderer *renderer, const char *path,
                 int x, int y, int w, int h);
void check_button_hover(Button *btn, int mx, int my);
int  button_clicked(Button *btn, int mx, int my);
void render_button(Button *btn, SDL_Renderer *renderer, TTF_Font *font,
                   const char *label);
void free_button(Button *btn);

#endif
