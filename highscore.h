#ifndef HIGHSCORE_H_INCLUDED
#define HIGHSCORE_H_INCLUDED

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

typedef struct {
    SDL_Texture *texture;
    SDL_Texture *texture_hover;
    SDL_Rect     pos;
    int          hovered;
} Button;

typedef struct {
    SDL_Texture *background_entry;
    SDL_Texture *background_scores;
    SDL_Texture *star;

    Button validate;
    Button input;
    Button back;
    Button quit;

    int  interface;

    char playerName[50];
    char names[3][50];
    int  scores[3];
    int  playerScore;

    TTF_Font  *font;
    TTF_Font  *fontTitle;
    SDL_Color  textColor;

    Mix_Chunk *victorySound;
    Mix_Chunk *hoverSound;
    Mix_Music *victoryMusic;

    int go_to_menu;
    int go_to_puzzle;
} Highscore;


SDL_Texture *loadTexture(const char *path, SDL_Renderer *renderer);
SDL_Texture *makeHoverTexture(SDL_Texture *src, SDL_Renderer *renderer);
void         safe_query_texture(SDL_Texture *tex, SDL_Rect *pos, int fw, int fh);


void save_scores(Highscore *h);
void load_scores(Highscore *h);
void insert_score(Highscore *h, const char *name, int score);


void check_hover(Button *btn, int mx, int my, Mix_Chunk *hoverSound);
void render_button(SDL_Renderer *renderer, Button *btn);


void init_highscore(Highscore *h, SDL_Renderer *renderer, int playerScore);
void render_highscore(Highscore *h, SDL_Renderer *renderer);
void handle_highscore_event(Highscore *h, SDL_Event event, int *running);
void free_highscore(Highscore *h);
void validate_entry(Highscore *h);

#endif
