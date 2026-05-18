#ifndef MENU_H
#define MENU_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>


typedef struct {
    SDL_Texture* yes;
    SDL_Texture* no;
    SDL_Texture* load;
    SDL_Texture* newGame;
    SDL_Rect rectYes;
    SDL_Rect rectNo;
    SDL_Rect rectLoad;
    SDL_Rect rectNew;
    int yesHovered;
    int noHovered;
    int loadHovered;
    int newHovered;
} MenuButtons;


void initMenu(SDL_Renderer* renderer, TTF_Font* font, MenuButtons* buttons);
void handleMenuEvents(SDL_Event* event, MenuButtons* buttons, int* currentPage, Mix_Chunk* hoverSound);
void renderMenu(SDL_Renderer* renderer, MenuButtons* buttons, SDL_Texture* background, SDL_Texture* textTexture, SDL_Rect textRect, int currentPage);

#endif
