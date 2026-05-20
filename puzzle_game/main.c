

#include <stdio.h>
#include "game.h"

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    GameContext ctx;
    Uint32 result_started = 0;
    int exit_code = 2;

    
    if (!game_init(&ctx)) {
        fprintf(stderr, "game_init failed – aborting.\n");
        game_destroy(&ctx);
        return 1;
    }

    
    int running = 1;
    SDL_Event event;

    while (running) {
        game_render(&ctx);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
                break;
            }
            
            if (event.type == SDL_KEYDOWN &&
                event.key.keysym.sym == SDLK_ESCAPE)
            {
                
                if (ctx.state != STATE_PLAYING) {
                    running = 0;
                    break;
                }
            }
            game_handle_event(&ctx, &event);
        }

        game_update(&ctx);

        if (ctx.state != STATE_PLAYING) {
            if (result_started == 0)
                result_started = SDL_GetTicks();

            if (SDL_GetTicks() - result_started >= 1200) {
                exit_code = (ctx.state == STATE_SUCCESS) ? 0 : 2;
                running = 0;
            }
        }
    }

    
    game_destroy(&ctx);
    return exit_code;
}
