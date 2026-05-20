#include "game.h"

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    srand((unsigned)time(NULL));

    /* SDL init */
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    Game g;
    memset(&g, 0, sizeof(g));

    g.win = SDL_CreateWindow("Enigma Quiz",
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                SCREEN_W, SCREEN_H, SDL_WINDOW_SHOWN);

    g.ren = SDL_CreateRenderer(g.win, -1,
                SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    SDL_SetRenderDrawBlendMode(g.ren, SDL_BLENDMODE_BLEND);

    /* fonts */
    g.fnt_big = TTF_OpenFont(FONT_PATH, 48);
    g.fnt_med = TTF_OpenFont(FONT_PATH, 24);
    g.fnt_sml = TTF_OpenFont(FONT_PATH, 17);

    if (!g.fnt_big) {
        fprintf(stderr, "Font not found: %s\n  Try: sudo apt install fonts-dejavu\n", FONT_PATH);
        return 1;
    }

    /* load questions */
    game_load(&g);
    game_init(&g);
    g.state = S_MENU;

    /* main loop */
    int run = 1;
    while (run) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { run = 0; break; }
            switch (g.state) {
                case S_MENU:     ev_menu    (&g, &e); break;
                case S_PLAYING:  ev_playing (&g, &e); break;
                case S_RESULT:   ev_result  (&g, &e); break;
                case S_GAMEOVER: ev_gameover(&g, &e); break;
            }
        }

        if (g.state == S_PLAYING) game_tick(&g);

        switch (g.state) {
            case S_MENU:     draw_menu    (&g); break;
            case S_PLAYING:  draw_playing (&g); break;
            case S_RESULT:   draw_result  (&g); break;
            case S_GAMEOVER: draw_gameover(&g); break;
        }

        SDL_RenderPresent(g.ren);
        SDL_Delay(16);
    }

    /* cleanup */
    TTF_CloseFont(g.fnt_big);
    TTF_CloseFont(g.fnt_med);
    TTF_CloseFont(g.fnt_sml);
    SDL_DestroyRenderer(g.ren);
    SDL_DestroyWindow(g.win);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
