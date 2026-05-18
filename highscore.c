#include "highscore.h"
#include <stdio.h>
#include <string.h>





SDL_Texture *loadTexture(const char *path, SDL_Renderer *renderer)
{
    SDL_Surface *surface;
    SDL_Texture *texture;

    surface = IMG_Load(path);
    if (!surface) {
        printf("IMG_Load Error (%s): %s\n", path, IMG_GetError());
        return NULL;
    }
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

SDL_Texture *makeHoverTexture(SDL_Texture *src, SDL_Renderer *renderer)
{
    SDL_Texture *dst;
    SDL_Rect     full;
    int          w, h;

    if (!src) return NULL;

    SDL_QueryTexture(src, NULL, NULL, &w, &h);

    dst = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                            SDL_TEXTUREACCESS_TARGET, w, h);
    if (!dst) return NULL;

    SDL_SetTextureBlendMode(dst, SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(renderer, dst);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, src, NULL, NULL);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
    SDL_SetRenderDrawColor(renderer, 80, 80, 0, 0);
    full.x = 0; full.y = 0; full.w = w; full.h = h;
    SDL_RenderFillRect(renderer, &full);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderTarget(renderer, NULL);
    return dst;
}

void safe_query_texture(SDL_Texture *tex, SDL_Rect *pos, int fw, int fh)
{
    if (tex)
        SDL_QueryTexture(tex, NULL, NULL, &pos->w, &pos->h);
    else {
        pos->w = fw;
        pos->h = fh;
    }
}





void save_scores(Highscore *h)
{
    FILE *f;
    int   i;

    f = fopen("scores.txt", "w");
    if (!f) return;
    for (i = 0; i < 3; i++)
        fprintf(f, "%s %d\n", h->names[i], h->scores[i]);
    fclose(f);
}

void load_scores(Highscore *h)
{
    FILE *f;
    int   i;

    f = fopen("scores.txt", "r");
    if (!f) return;
    for (i = 0; i < 3; i++) {
        if (fscanf(f, "%49s %d", h->names[i], &h->scores[i]) != 2) {
            h->names[i][0] = '\0';
            h->scores[i]   = 0;
        }
    }
    fclose(f);
}

void insert_score(Highscore *h, const char *name, int score)
{
    int i, j;

    for (i = 0; i < 3; i++) {
        if (score > h->scores[i]) {
            for (j = 2; j > i; j--) {
                h->scores[j] = h->scores[j - 1];
                strncpy(h->names[j], h->names[j - 1], 49);
            }
            h->scores[i] = score;
            strncpy(h->names[i], name, 49);
            h->names[i][49] = '\0';
            break;
        }
    }
}





void check_hover(Button *btn, int mx, int my, Mix_Chunk *hoverSound)
{
    SDL_Point p;
    int       was;

    p.x = mx;
    p.y = my;
    was         = btn->hovered;
    btn->hovered = SDL_PointInRect(&p, &btn->pos);
    if (btn->hovered && !was && hoverSound)
        Mix_PlayChannel(-1, hoverSound, 0);
}

void render_button(SDL_Renderer *renderer, Button *btn)
{
    SDL_Rect     dest;
    SDL_Texture *tex;
    int          new_w, new_h;

    if (!btn->pos.w || !btn->pos.h) return;

    dest = btn->pos;

    if (btn->hovered) {
        new_w  = (int)(btn->pos.w * 1.1f);
        new_h  = (int)(btn->pos.h * 1.1f);
        dest.x = btn->pos.x - (new_w - btn->pos.w) / 2;
        dest.y = btn->pos.y - (new_h - btn->pos.h) / 2;
        dest.w = new_w;
        dest.h = new_h;
    }

    tex = (btn->hovered && btn->texture_hover)
          ? btn->texture_hover : btn->texture;

    if (!tex) {
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderFillRect(renderer, &dest);
    } else {
        SDL_RenderCopy(renderer, tex, NULL, &dest);
    }
}





void validate_entry(Highscore *h)
{
    if (strlen(h->playerName) == 0)
        strncpy(h->playerName, "Player", 49);
    insert_score(h, h->playerName, h->playerScore);
    save_scores(h);
    h->interface = 1;
    Mix_HaltMusic();
    if (h->victoryMusic)
        Mix_PlayMusic(h->victoryMusic, -1);
    if (h->victorySound)
        Mix_PlayChannel(-1, h->victorySound, 0);
}





void init_highscore(Highscore *h, SDL_Renderer *renderer, int playerScore)
{
    int i;

    memset(h, 0, sizeof(Highscore));

    h->playerScore  = playerScore;
    h->go_to_menu   = 0;
    h->go_to_puzzle = 0;

    
    h->background_entry  = loadTexture("img/png/background-01.png", renderer);
    h->background_scores = loadTexture("img/png/background-01.png", renderer);
    h->star              = loadTexture("img/png/star-01-01.png",     renderer);

    
    h->input.texture = loadTexture("img/png/input-01.png", renderer);
    safe_query_texture(h->input.texture, &h->input.pos, 400, 60);
    h->input.pos.x = (1280 - h->input.pos.w) / 2;
    h->input.pos.y = 250;

    
    h->validate.texture       = loadTexture("img/png/validate-01.png", renderer);
    h->validate.texture_hover = makeHoverTexture(h->validate.texture, renderer);
    safe_query_texture(h->validate.texture, &h->validate.pos, 200, 60);
    h->validate.pos.x = (1280 - h->validate.pos.w) / 2;
    h->validate.pos.y = h->input.pos.y + h->input.pos.h + 30;

    
    h->back.texture       = loadTexture("img/png/return-01.png", renderer);
    h->back.texture_hover = makeHoverTexture(h->back.texture, renderer);
    safe_query_texture(h->back.texture, &h->back.pos, 150, 60);
    h->back.pos.x = 200;
    h->back.pos.y = 500;

    
    h->quit.texture       = loadTexture("img/png/exit-01.png", renderer);
    h->quit.texture_hover = makeHoverTexture(h->quit.texture, renderer);
    safe_query_texture(h->quit.texture, &h->quit.pos, 150, 60);
    h->quit.pos.x = 850;
    h->quit.pos.y = 500;

    
    h->font      = TTF_OpenFont("fonts/arial.ttf", 28);
    h->fontTitle = TTF_OpenFont("fonts/arial.ttf", 52);
    if (!h->font)      printf("TTF_OpenFont Error (font): %s\n",      TTF_GetError());
    if (!h->fontTitle) printf("TTF_OpenFont Error (fontTitle): %s\n", TTF_GetError());

    h->textColor.r = 255;
    h->textColor.g = 215;
    h->textColor.b = 0;
    h->textColor.a = 255;

    
    h->hoverSound   = Mix_LoadWAV("sounds/hover.wav");
    h->victorySound = Mix_LoadWAV("sounds/victory.wav");
    h->victoryMusic = Mix_LoadMUS("sounds/victory_music.wav");
    if (!h->hoverSound)   printf("hoverSound not loaded: %s\n",   Mix_GetError());
    if (!h->victorySound) printf("victorySound not loaded: %s\n", Mix_GetError());
    if (!h->victoryMusic) printf("victoryMusic not loaded: %s\n", Mix_GetError());

    
    for (i = 0; i < 3; i++) {
        h->names[i][0] = '\0';
        h->scores[i]   = 0;
    }
    load_scores(h);

    h->interface     = 0;
    h->playerName[0] = '\0';

    SDL_StartTextInput();
}





void render_highscore(Highscore *h, SDL_Renderer *renderer)
{
    char         scoreStr[64];
    char         line[120];
    char         rank[4];
    SDL_Surface *surf;
    SDL_Texture *tex;
    SDL_Rect     r;
    SDL_Rect     starPos;
    int          i;
    int          y_offset;

    
    if (h->interface == 0) {

        if (h->background_entry)
            SDL_RenderCopy(renderer, h->background_entry, NULL, NULL);

        if (h->font) {
            
            sprintf(scoreStr, "Your score: %d", h->playerScore);
            surf = TTF_RenderText_Blended(h->font, scoreStr, h->textColor);
            if (surf) {
                tex  = SDL_CreateTextureFromSurface(renderer, surf);
                r.x  = (1280 - surf->w) / 2;
                r.y  = h->input.pos.y - 100;
                r.w  = surf->w;
                r.h  = surf->h;
                SDL_RenderCopy(renderer, tex, NULL, &r);
                SDL_FreeSurface(surf);
                SDL_DestroyTexture(tex);
            }

            
            surf = TTF_RenderText_Blended(h->font, "Enter your name:", h->textColor);
            if (surf) {
                tex  = SDL_CreateTextureFromSurface(renderer, surf);
                r.x  = (1280 - surf->w) / 2;
                r.y  = h->input.pos.y - 55;
                r.w  = surf->w;
                r.h  = surf->h;
                SDL_RenderCopy(renderer, tex, NULL, &r);
                SDL_FreeSurface(surf);
                SDL_DestroyTexture(tex);
            }
        }

        
        if (h->input.texture)
            SDL_RenderCopy(renderer, h->input.texture, NULL, &h->input.pos);
        else {
            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
            SDL_RenderFillRect(renderer, &h->input.pos);
        }

        
        if (h->font && strlen(h->playerName) > 0) {
            surf = TTF_RenderText_Blended(h->font, h->playerName, h->textColor);
            if (surf) {
                tex  = SDL_CreateTextureFromSurface(renderer, surf);
                r.x  = h->input.pos.x + 20;
                r.y  = h->input.pos.y + (h->input.pos.h - surf->h) / 2;
                r.w  = surf->w;
                r.h  = surf->h;
                SDL_RenderCopy(renderer, tex, NULL, &r);
                SDL_FreeSurface(surf);
                SDL_DestroyTexture(tex);
            }
        }

        render_button(renderer, &h->validate);
    }

    
    if (h->interface == 1) {

        if (h->background_scores)
            SDL_RenderCopy(renderer, h->background_scores, NULL, NULL);

        
        if (h->fontTitle) {
            surf = TTF_RenderText_Blended(h->fontTitle, "HIGH SCORES", h->textColor);
            if (surf) {
                tex  = SDL_CreateTextureFromSurface(renderer, surf);
                r.x  = (1280 - surf->w) / 2;
                r.y  = 30;
                r.w  = surf->w;
                r.h  = surf->h;
                SDL_RenderCopy(renderer, tex, NULL, &r);
                SDL_FreeSurface(surf);
                SDL_DestroyTexture(tex);
            }
        }

        
        if (h->font) {
            y_offset = 160;
            for (i = 0; i < 3; i++) {

                
                sprintf(rank, "%d.", i + 1);
                surf = TTF_RenderText_Blended(h->font, rank, h->textColor);
                if (surf) {
                    tex     = SDL_CreateTextureFromSurface(renderer, surf);
                    r.x     = 340;
                    r.y     = y_offset;
                    r.w     = surf->w;
                    r.h     = surf->h;
                    SDL_RenderCopy(renderer, tex, NULL, &r);
                    SDL_FreeSurface(surf);
                    SDL_DestroyTexture(tex);
                }

                
                starPos.x = 380;
                starPos.y = y_offset;
                starPos.w = 32;
                starPos.h = 32;
                if (h->star)
                    SDL_RenderCopy(renderer, h->star, NULL, &starPos);

                
                if (strlen(h->names[i]) > 0)
                    sprintf(line, "%s  -  %d", h->names[i], h->scores[i]);
                else
                    sprintf(line, "---");

                surf = TTF_RenderText_Blended(h->font, line, h->textColor);
                if (surf) {
                    tex     = SDL_CreateTextureFromSurface(renderer, surf);
                    r.x     = 430;
                    r.y     = y_offset;
                    r.w     = surf->w;
                    r.h     = surf->h;
                    SDL_RenderCopy(renderer, tex, NULL, &r);
                    SDL_FreeSurface(surf);
                    SDL_DestroyTexture(tex);
                }

                y_offset += 70;
            }
        }

        render_button(renderer, &h->back);
        render_button(renderer, &h->quit);
    }
}





void handle_highscore_event(Highscore *h, SDL_Event event, int *running)
{
    SDL_Point  p;
    SDL_Keycode key;
    int         mx, my;

    if (event.type == SDL_QUIT) { *running = 0; return; }

    
    if (event.type == SDL_MOUSEMOTION) {
        mx = event.motion.x;
        my = event.motion.y;
        if (h->interface == 0)
            check_hover(&h->validate, mx, my, h->hoverSound);
        if (h->interface == 1) {
            check_hover(&h->back, mx, my, h->hoverSound);
            check_hover(&h->quit, mx, my, h->hoverSound);
        }
    }

    
    if (event.type == SDL_MOUSEBUTTONDOWN &&
        event.button.button == SDL_BUTTON_LEFT) {
        p.x = event.button.x;
        p.y = event.button.y;
        if (h->interface == 0 && SDL_PointInRect(&p, &h->validate.pos))
            validate_entry(h);
        if (h->interface == 1) {
            if (SDL_PointInRect(&p, &h->back.pos)) {
                Mix_HaltMusic();
                h->go_to_menu = 1;
            }
            if (SDL_PointInRect(&p, &h->quit.pos)) {
                Mix_HaltMusic();
                *running = 0;
            }
        }
    }

    
    if (event.type == SDL_KEYDOWN) {
        key = event.key.keysym.sym;
        if (key == SDLK_ESCAPE) { Mix_HaltMusic(); *running = 0; }
        if (key == SDLK_e && h->interface == 1) { h->go_to_puzzle = 1; }
        if (h->interface == 0) {
            if (key == SDLK_RETURN)
                validate_entry(h);
            if (key == SDLK_BACKSPACE && strlen(h->playerName) > 0)
                h->playerName[strlen(h->playerName) - 1] = '\0';
        }
    }

    
    if (event.type == SDL_TEXTINPUT && h->interface == 0)
        if (strlen(h->playerName) < 49)
            strcat(h->playerName, event.text.text);
}





void free_highscore(Highscore *h)
{
    SDL_DestroyTexture(h->background_entry);
    SDL_DestroyTexture(h->background_scores);
    SDL_DestroyTexture(h->star);
    SDL_DestroyTexture(h->validate.texture);
    SDL_DestroyTexture(h->validate.texture_hover);
    SDL_DestroyTexture(h->input.texture);
    SDL_DestroyTexture(h->back.texture);
    SDL_DestroyTexture(h->back.texture_hover);
    SDL_DestroyTexture(h->quit.texture);
    SDL_DestroyTexture(h->quit.texture_hover);
    if (h->font)         TTF_CloseFont(h->font);
    if (h->fontTitle)    TTF_CloseFont(h->fontTitle);
    if (h->hoverSound)   Mix_FreeChunk(h->hoverSound);
    if (h->victorySound) Mix_FreeChunk(h->victorySound);
    if (h->victoryMusic) Mix_FreeMusic(h->victoryMusic);
    Mix_HaltMusic();
    SDL_StopTextInput();
}
