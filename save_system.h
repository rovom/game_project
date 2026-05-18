#ifndef SAVE_SYSTEM_H
#define SAVE_SYSTEM_H

#include "header.h"

#define SAVE_DIR "saves"
#define SAVE_PATH_MAX 256

int save_exists(const char *path);
int make_save_path(char *path, int pathSize, const char *saveName);
int save_game_state(const char *path,
                    Joueur *J1, Joueur *J2,
                    Bullet bullets[], int bulletCount,
                    NPC npcs[], int npcCount,
                    SDL_Rect stablePlatforms[], int stableCount,
                    SDL_Rect movingPlatforms[], int movingCount,
                    int movingDir[], int movingMin[], int movingMax[],
                    int joueurActif, int puzzleChallengeUsed,
                    int multiplayerMode);
int load_game_state(const char *path,
                    Joueur *J1, Joueur *J2,
                    Bullet bullets[], int bulletCount,
                    NPC npcs[], int npcCount,
                    SDL_Rect stablePlatforms[], int stableCount,
                    SDL_Rect movingPlatforms[], int movingCount,
                    int movingDir[], int movingMin[], int movingMax[],
                    int *joueurActif, int *puzzleChallengeUsed,
                    int *multiplayerMode);
int prompt_select_save(SDL_Renderer *renderer, TTF_Font *font, char *path, int pathSize);
int prompt_save_game(SDL_Renderer *renderer, TTF_Font *font);
int prompt_save_name(SDL_Renderer *renderer, TTF_Font *font, char *name, int nameSize);
void show_save_message(SDL_Renderer *renderer, TTF_Font *font, const char *message);

#endif
