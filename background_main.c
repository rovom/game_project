#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <string.h>
#include "background.h"

int main(void)
{
    
    SDL_Window   *window;
    SDL_Renderer *renderer;
    TTF_Font     *font;
    Background    bg;
    GameTimer     timer1;
    GameTimer     timer2;
    Guide         guide1;
    Guide         guide2;
    Mix_Music    *bgMusic;
    SDL_Event     event;
    int           keysHeld[SDL_NUM_SCANCODES];
    int           running;
    int           next;
    int           p1CenterX;
    int           p2CenterX;

    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        printf("SDL_Init: %s\n", SDL_GetError()); return 1;
    }
    if (IMG_Init(IMG_INIT_PNG) == 0) {
        printf("IMG_Init: %s\n", IMG_GetError()); return 1;
    }
    if (TTF_Init() == -1) {
        printf("TTF_Init: %s\n", TTF_GetError()); return 1;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("Mix_OpenAudio: %s\n", Mix_GetError()); return 1;
    }

    
    window = SDL_CreateWindow(
        "STELLAR WARS",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        W_SCREEN, H_SCREEN,
        SDL_WINDOW_SHOWN
    );
    if (!window)   { printf("Window: %s\n",   SDL_GetError()); return 1; }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) { printf("Renderer: %s\n", SDL_GetError()); return 1; }

    
    font = TTF_OpenFont("assets/font.ttf", 24);
    if (!font)
        printf("[Font] %s - timer/guide text disabled.\n", TTF_GetError());

    
    init_background(&bg, renderer, 1);
    init_timer(&timer1);
    init_timer(&timer2);
    init_guide(&guide1, 1);
    init_guide(&guide2, 2);

    
    bgMusic = Mix_LoadMUS("assets/background_sound.mp3");
    if (!bgMusic)
        printf("[Music] %s - background music disabled.\n", Mix_GetError());
    else
        Mix_PlayMusic(bgMusic, -1);
    Mix_VolumeMusic(64);

    
    memset(keysHeld, 0, sizeof(keysHeld));
    running = 1;

    
    while (running) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        render_background(&bg, renderer);
        render_platforms(&bg, renderer);

        if (bg.screenMode == MODE_SPLIT) {
            p1CenterX = W_HALF / 2;
            p2CenterX = W_HALF + W_HALF / 2;
            render_timer(&timer1, renderer, font, p1CenterX, 15);
            render_timer(&timer2, renderer, font, p2CenterX, 15);
        } else {
            p1CenterX = W_SCREEN / 2;
            render_timer(&timer1, renderer, font, p1CenterX, 15);
        }

        if (bg.screenMode == MODE_SPLIT) {
            render_guide(&guide1, renderer, font, 0,      W_HALF);
            render_guide(&guide2, renderer, font, W_HALF, W_HALF);
        } else {
            render_guide(&guide1, renderer, font, 0, W_SCREEN);
        }

        SDL_RenderPresent(renderer);

        
        while (SDL_PollEvent(&event)) {

            if (event.type == SDL_QUIT) { running = 0; break; }

            if (event.type == SDL_KEYDOWN) {
                keysHeld[event.key.keysym.scancode] = 1;

                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    running = 0;
                    break;
                }

                
                if (event.key.keysym.scancode == SDL_SCANCODE_G ||
                    event.key.keysym.scancode == SDL_SCANCODE_F1)
                    toggle_guide(&guide1);

                
                if (event.key.keysym.scancode == SDL_SCANCODE_H)
                    toggle_guide(&guide2);

                
                if (event.key.keysym.scancode == SDL_SCANCODE_L) {
                    next = (bg.level == 1) ? 2 : 1;
                    init_platforms(&bg, next);
                }

                
                if (event.key.keysym.scancode == SDL_SCANCODE_T) {
                    if (bg.screenMode == MODE_SPLIT)
                        bg.screenMode = MODE_SINGLE;
                    else
                        bg.screenMode = MODE_SPLIT;
                }
            }

            if (event.type == SDL_KEYUP)
                keysHeld[event.key.keysym.scancode] = 0;
        }

        
        if (keysHeld[SDL_SCANCODE_LEFT])  scroll_background(&bg, 1, -SCROLL_SPEED,  0);
        if (keysHeld[SDL_SCANCODE_RIGHT]) scroll_background(&bg, 1,  SCROLL_SPEED,  0);
        if (keysHeld[SDL_SCANCODE_UP])    scroll_background(&bg, 1,  0, -SCROLL_SPEED);
        if (keysHeld[SDL_SCANCODE_DOWN])  scroll_background(&bg, 1,  0,  SCROLL_SPEED);

        
        if (bg.screenMode == MODE_SPLIT) {
            if (keysHeld[SDL_SCANCODE_A]) scroll_background(&bg, 2, -SCROLL_SPEED,  0);
            if (keysHeld[SDL_SCANCODE_D]) scroll_background(&bg, 2,  SCROLL_SPEED,  0);
            if (keysHeld[SDL_SCANCODE_W]) scroll_background(&bg, 2,  0, -SCROLL_SPEED);
            if (keysHeld[SDL_SCANCODE_S]) scroll_background(&bg, 2,  0,  SCROLL_SPEED);
        }

        
        update_platforms(&bg);
        update_timer(&timer1);
        if (bg.screenMode == MODE_SPLIT)
            update_timer(&timer2);
        SDL_Delay(16);
    }

    
    Mix_HaltMusic();
    if (bgMusic) Mix_FreeMusic(bgMusic);
    free_background(&bg);
    if (font) TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_CloseAudio();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
