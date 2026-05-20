#include "menu.h"
#include <SDL2/SDL_image.h>
#include <stdio.h>

void initMenu(SDL_Renderer* renderer, TTF_Font* font, MenuButtons* buttons) {
   
    buttons->yes     = IMG_LoadTexture(renderer, "assets/images/button_yes.png");
    buttons->no      = IMG_LoadTexture(renderer, "assets/images/button_no.png");
    buttons->load    = IMG_LoadTexture(renderer, "assets/images/button_loading.png");
    buttons->newGame = IMG_LoadTexture(renderer, "assets/images/button_new.png");

   
    buttons->rectYes  = (SDL_Rect){300, 300, 200, 70};
    buttons->rectNo   = (SDL_Rect){300, 400, 200, 70};
    buttons->rectLoad = (SDL_Rect){300, 250, 200, 70};
    buttons->rectNew  = (SDL_Rect){300, 350, 200, 70};

    
    buttons->yesHovered = 0;
    buttons->noHovered  = 0;
    buttons->loadHovered = 0;
    buttons->newHovered = 0;
}

void handleMenuEvents(SDL_Event* event, MenuButtons* buttons, int* currentPage, Mix_Chunk* hoverSound) {
    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    if(event->type == SDL_MOUSEBUTTONDOWN) {
        if(mouseX >= buttons->rectYes.x && mouseX <= buttons->rectYes.x + buttons->rectYes.w &&
           mouseY >= buttons->rectYes.y && mouseY <= buttons->rectYes.y + buttons->rectYes.h) {
            *currentPage = 1;
        }
        if(*currentPage == 1) {
            if(mouseX >= buttons->rectNew.x && mouseX <= buttons->rectNew.x + buttons->rectNew.w &&
               mouseY >= buttons->rectNew.y && mouseY <= buttons->rectNew.y + buttons->rectNew.h) {
                *currentPage = 2;
            }
        }
    }

    if(event->type == SDL_KEYDOWN) {
        if(*currentPage == 1 && event->key.keysym.sym == SDLK_n) {
            *currentPage = 2;
        }
    }

   
    if(*currentPage == 0) {
        buttons->yesHovered = (mouseX >= buttons->rectYes.x && mouseX <= buttons->rectYes.x + buttons->rectYes.w &&
                               mouseY >= buttons->rectYes.y && mouseY <= buttons->rectYes.y + buttons->rectYes.h);
        if(buttons->yesHovered) Mix_PlayChannel(-1, hoverSound, 0);

        buttons->noHovered = (mouseX >= buttons->rectNo.x && mouseX <= buttons->rectNo.x + buttons->rectNo.w &&
                              mouseY >= buttons->rectNo.y && mouseY <= buttons->rectNo.y + buttons->rectNo.h);
        if(buttons->noHovered) Mix_PlayChannel(-1, hoverSound, 0);
    } else if(*currentPage == 1) {
        buttons->loadHovered = (mouseX >= buttons->rectLoad.x && mouseX <= buttons->rectLoad.x + buttons->rectLoad.w &&
                                mouseY >= buttons->rectLoad.y && mouseY <= buttons->rectLoad.y + buttons->rectLoad.h);
        if(buttons->loadHovered) Mix_PlayChannel(-1, hoverSound, 0);

        buttons->newHovered = (mouseX >= buttons->rectNew.x && mouseX <= buttons->rectNew.x + buttons->rectNew.w &&
                               mouseY >= buttons->rectNew.y && mouseY <= buttons->rectNew.y + buttons->rectNew.h);
        if(buttons->newHovered) Mix_PlayChannel(-1, hoverSound, 0);
    }
}

void renderMenu(SDL_Renderer* renderer, MenuButtons* buttons, SDL_Texture* background, SDL_Texture* textTexture, SDL_Rect textRect, int currentPage) {
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, background, NULL, NULL);

    if(currentPage == 0) {
        SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
        SDL_Rect yesHoverRect = {buttons->rectYes.x-10, buttons->rectYes.y-10, buttons->rectYes.w+20, buttons->rectYes.h+20};
        SDL_Rect noHoverRect  = {buttons->rectNo.x-10,  buttons->rectNo.y-10,  buttons->rectNo.w+20, buttons->rectNo.h+20};
        SDL_RenderCopy(renderer, buttons->yes, NULL, buttons->yesHovered ? &yesHoverRect : &buttons->rectYes);
        SDL_RenderCopy(renderer, buttons->no, NULL, buttons->noHovered ? &noHoverRect : &buttons->rectNo);
    } else if(currentPage == 1) {
        SDL_Rect loadHoverRect = {buttons->rectLoad.x-10, buttons->rectLoad.y-10, buttons->rectLoad.w+20, buttons->rectLoad.h+20};
        SDL_Rect newHoverRect  = {buttons->rectNew.x-10,  buttons->rectNew.y-10,  buttons->rectNew.w+20, buttons->rectNew.h+20};
        SDL_RenderCopy(renderer, buttons->load, NULL, buttons->loadHovered ? &loadHoverRect : &buttons->rectLoad);
        SDL_RenderCopy(renderer, buttons->newGame, NULL, buttons->newHovered ? &newHoverRect : &buttons->rectNew);
    }

    SDL_RenderPresent(renderer);
}
