#include <SDL2/SDL.h>
#include "minimap.h"

int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return -1;

    SDL_Window *win = SDL_CreateWindow("Space Mission", 100, 100, 1080, 607, 0);
    SDL_Surface *screen = SDL_GetWindowSurface(win);
    
    SDL_Surface *bg = SDL_LoadBMP("back.png");
    SDL_Surface *mask = SDL_LoadBMP("backgroundMasque.bmp");
    
    Player p;
    p.sprite = SDL_LoadBMP("player.png");
    p.pos = (SDL_Rect){100, 300, 50, 80};
    p.velY = 0;
    p.onGround = 0;

    Enemy enemies[MAX_ENEMIES];
    for(int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = 0;

    Minimap mm;
    init_minimap(&mm, "minimap.png", 1080, 607, 2000, 607);

    int running = 1;
    SDL_Event ev;
    
    while (running) {
        SDL_Rect camera = {0, 0, 1080, 607};
        update_minimap(&mm, p.pos, camera);

        SDL_BlitSurface(bg, NULL, screen, NULL);
        SDL_BlitSurface(p.sprite, NULL, screen, &p.pos);
        
        display_minimap(mm, screen);
        display_entities_on_map(mm, screen, enemies);
        
        SDL_UpdateWindowSurface(win);

        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = 0;
        }

        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        if (keys[SDL_SCANCODE_RIGHT]) p.pos.x += 5;
        if (keys[SDL_SCANCODE_LEFT]) p.pos.x -= 5;
        
        
        p.velY += 1;
        p.pos.y += p.velY;
        if (p.pos.y >= 500) { p.pos.y = 500; p.velY = 0; p.onGround = 1; }

        SDL_Delay(16);
    }

   
    SDL_FreeSurface(bg);
    SDL_FreeSurface(mask);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
