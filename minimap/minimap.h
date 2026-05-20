#ifndef MINIMAP_H
#define MINIMAP_H
#include <SDL2/SDL.h>
#define MAX_ENEMIES 15
#define MINIMAP_MARGIN 25
#define PLAYER_DOT_SIZE 8

typedef struct {
    SDL_Rect pos;
    SDL_Surface *sprite;
    int velY;
    int onGround;
} Player;

typedef struct {
    SDL_Rect pos;
    SDL_Surface *sprite;
    int speed;
    int active;
} Enemy;

typedef struct {
    SDL_Surface *thumbnail;
    SDL_Surface *man;
    SDL_Rect pos;
    SDL_Rect posMan;
    float scaleX;
    float scaleY;
} Minimap;

int  init_minimap(Minimap *m, const char *path, int sw, int sh, int bw, int bh);
void update_minimap(Minimap *m, SDL_Rect playerPos, SDL_Rect cam);
void display_minimap(Minimap m, SDL_Surface *dest);
void display_entities_on_map(Minimap m, SDL_Surface *dest, Enemy *enemies);
int  check_collision_aabb(SDL_Rect a, SDL_Rect b);
int  check_collision_pixel(SDL_Rect pj, SDL_Surface *mask, int direction);

#endif
