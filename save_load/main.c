#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include "menu.h"

void run_save_load_menu() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    TTF_Init();
    IMG_Init(IMG_INIT_PNG);
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) < 0) {
        printf("SDL_mixer Error: %s\n", Mix_GetError());
    }
    SDL_Window* window = SDL_CreateWindow("Save & Load Menu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    Mix_Music* bgMusic = Mix_LoadMUS("assets/music/NASA.mp3");
    if (bgMusic) {
        Mix_PlayMusic(bgMusic, -1);
    }
    Mix_Chunk* hoverSound = Mix_LoadWAV("assets/sounds/hover.wav");
    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 28);
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface* textSurface = TTF_RenderText_Blended(font, "Do you want to save your game?", white);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_Rect textRect = {150, 200, textSurface->w, textSurface->h};
    SDL_FreeSurface(textSurface);
    SDL_Texture* background = IMG_LoadTexture(renderer, "assets/images/background.png");
    MenuButtons buttons;
    initMenu(renderer, font, &buttons);
    int running = 1;
    SDL_Event event;
    int currentPage = 0;
    while(running) {
        renderMenu(renderer, &buttons, background, textTexture, textRect, currentPage);

        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) running = 0;
            handleMenuEvents(&event, &buttons, &currentPage, hoverSound);
        }
    }
    SDL_DestroyTexture(background);
    SDL_DestroyTexture(textTexture);
    SDL_DestroyTexture(buttons.yes);
    SDL_DestroyTexture(buttons.no);
    SDL_DestroyTexture(buttons.load);
    SDL_DestroyTexture(buttons.newGame);
    TTF_CloseFont(font);
    Mix_FreeChunk(hoverSound);
    Mix_FreeMusic(bgMusic);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_CloseAudio();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}
