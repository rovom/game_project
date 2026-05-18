#ifndef MAIN_MENU_H
#define MAIN_MENU_H

#include "header.h"

#define MAIN_MENU_QUIT 0
#define MAIN_MENU_START 1
#define GAME_MODE_SINGLE 1
#define GAME_MODE_MULTI 2

int run_main_menu(SDL_Renderer *renderer);
int prompt_game_mode(SDL_Renderer *renderer, TTF_Font *font);

#endif
