

#ifndef HEADER_H
#define HEADER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_W 1280
#define SCREEN_H 720
#define WORLD_W 5120
#define WORLD_H 850
#define GROUND_Y 500
#define MARS_GROUND_Y 470
#define MAX_NPCS 15
#define STABLE_PLATFORM_COUNT 3
#define MOVING_PLATFORM_COUNT 2
#define SHIP_END_X 3050
#define SHIP_GROUND_SURFACE_OFFSET 5
#define MARS_GROUND_SURFACE_OFFSET 49

#define PLAYER_W 120
#define PLAYER_H 120
#define PLAYER_SPEED 4
#define PLAYER_LIVES 3
#define PLAYER_HEALTH_MAX 3
#define PLAYER_DAMAGE_COOLDOWN 900
#define PLAYER_RESPAWN_DELAY 1200

#define PLUTO_IDLE_ROWS 2
#define PLUTO_IDLE_COLS 5
#define PLUTO_WALK_ROWS 2
#define PLUTO_WALK_COLS 5
#define PLUTO_FIRE_ROWS 2
#define PLUTO_FIRE_COLS 5
#define PLUTO_JUMP_ROWS 2
#define PLUTO_JUMP_COLS 7
#define PLUTO_FALL_ROWS 2
#define PLUTO_FALL_COLS 7
#define PLUTO_DIE_ROWS 2
#define PLUTO_DIE_COLS 6
#define PLUTO_MAX_FRAMES 8
#define PLUTO_IDLE_PATH "assets/characters/joueur1/orange/idle.png"
#define PLUTO_WALK_PATH "assets_pluto/walk.png"
#define PLUTO_FIRE_PATH "assets_pluto/fire.png"
#define PLUTO_JUMP_PATH "assets_pluto/jump.png"
#define PLUTO_FALL_PATH "assets/characters/joueur1/orange/fall.png"
#define PLUTO_DIE_PATH "assets_pluto/die.png"
#define PLUTO_FONT_PATH "assets_pluto/font.ttf"

#define NPC_WALK_COLS 9
#define NPC_WALK_ROWS 2
#define NPC_WALK_RIGHT_ROW 1
#define NPC_SLASH_COLS 6
#define NPC_SLASH_ROWS 2
#define NPC_DIE_COLS 6
#define NPC_DIE_ROWS 1

#define FACE_RIGHT 0
#define FACE_LEFT 1

#define MAX_BULLETS 50
#define BULLET_W 24
#define BULLET_H 8
#define BULLET_SPEED 14
#define FIRE_COOLDOWN 250

#define ENEMY_HEALTH_MAX 3
#define ENEMY_ATTACK_RANGE 45
#define ENEMY_ATTACK_DURATION 420
#define ENEMY_ATTACK_COOLDOWN 900
#define ENEMY_DAMAGE_SCORE 10
#define ENEMY_HIT_SCORE 10
#define ENEMY_KILL_SCORE 100
#define ENEMY_DETECTION_RANGE 310
#define ENEMY_CHASE_SPEED 2.4f
#define ENEMY_PATROL_SPEED 1.0f
#define ENEMY_JUMP_FORCE -14.0f
#define ENEMY_GRAVITY 0.7f
#define ENEMY_RANDOM_JUMP_CHANCE 2
#define ENEMY_JUMP_COOLDOWN 1300
#define MAX_SECONDARY_ENTITIES 5
#define MIN_SECONDARY_ENTITIES 2
#define GEM_SCORE_VALUE 50
#define GEM_DAMAGE_BONUS 2
#define GEM_BUFF_DURATION 600
#define GEM_W 64
#define GEM_H 82
#define NPC_VISUAL_GROUND_OFFSET 6
#define PUZZLE_CHALLENGE_ROUNDS 5
#define PUZZLE_CHALLENGE_REQUIRED 3
#define PUZZLE_COMMAND "cd puzzle_game && ./puzzle"
#define ENIGMA_COMMAND "cd enigma && ./enigma"

typedef enum
{
    STATE_IDLE,
    STATE_WALK,
    STATE_JUMP,
    STATE_FIRE,
    STATE_DIE,
    STATE_FALL
} PlayerState;

typedef enum
{
    ENEMY_ALIVE,
    ENEMY_INJURED,
    ENEMY_NEUTRALIZED,
    ENEMY_ATTACKING,
    ENEMY_REMOVED
} EnemyState;


typedef struct
{
    SDL_Texture *texture; 
    int rows; 
    int cols; 
    int frameW; 
    int frameH; 
    int drawW; 
    int drawH; 
    SDL_Rect frames[2][PLUTO_MAX_FRAMES]; 
} Animation;


typedef struct
{
    int active; 
    SDL_Rect rect; 
    int direction; 
    int owner; 
} Bullet;

typedef struct
{
    const char *idle;
    const char *walk;
    const char *jump;
    const char *fall;
    const char *fire;
    const char *die;
    const char *portrait;
    const char *characterName;
    const char *outfitName;
} PlayerAssets;

typedef struct
{
    int p1CharacterIndex;
    int p1OutfitIndex;
    int p2CharacterIndex;
    int p2OutfitIndex;
} CharacterSelection;

typedef struct
{
    const char *name;
    PlayerAssets outfits[2];
} CharacterDefinition;


typedef struct
{
    Animation idle; 
    Animation walk; 
    Animation jump; 
    Animation fall; 
    Animation fire; 
    Animation die; 
    SDL_Rect posScreen; 
    SDL_Rect posSprite; 
    SDL_Rect drawRect; 
    int drawOffsetX; 
    int drawOffsetY; 
    PlayerState state; 
    int facing; 
    int currentFrame; 
    Uint32 lastFrameTime; 
    Uint32 frameDelay; 
    int score; 
    int vies; 
    int health; 
    int alive; 
    int visible; 
    int startX; 
    int startY; 
    int floorY; 
    int isJumping; 
    int jumpsUsed; 
    int jumpHeld; 
    float velY; 
    float posYFloat; 
    int moveLeft; 
    int moveRight; 
    Uint32 lastShotTime; 
    Uint32 invulnerableUntil; 
    Uint32 deathTime; 
} Joueur;


typedef struct
{
    float x; 
    float y; 
    int w; 
    int h; 
    SDL_Texture *walkTexture; 
    SDL_Texture *slashTexture; 
    SDL_Texture *dieTexture; 
    SDL_Rect srcRect; 
    SDL_Rect dstRect; 
    int useSprite; 
    int direction; 
    int posMin; 
    int posMax; 
    int groundY; 
    int action; 
    int health; 
    int maxHealth; 
    EnemyState state; 
    Uint32 lastFrameTime; 
    Uint32 frameDelay; 
    Uint32 attackStartedAt; 
    Uint32 nextAttackAt; 
    float velY; 
    int onGround; 
    Uint32 nextJumpAt; 
} NPC;

typedef struct
{
    float x;
    float y;
    int w;
    int h;
    int active;
} SecondaryEntity;

int initSDL(SDL_Window **window, SDL_Renderer **renderer);
void shutdownSDL(SDL_Window *window, SDL_Renderer *renderer);
void stellarMusicStartMenu(void);
void stellarMusicUpdateMenu(void);
void stellarMusicStartGameplay(void);
void stellarMusicUpdateGameplay(void);
void stellarMusicStop(void);

int initialiserJoueur(Joueur *J, SDL_Renderer *renderer, int x, int y);
int initialiserJoueurAvecAssets(Joueur *J, SDL_Renderer *renderer, int x, int y,
                                const PlayerAssets *assets);
void libererJoueur(Joueur *J);
void gererEntreeJoueurClavier(Joueur *J, const Uint8 *keys,
                              SDL_Scancode left, SDL_Scancode right,
                              SDL_Scancode jumpKey);
void lancerSaut(Joueur *J);
void updateJoueur(Joueur *J, Uint32 now);
void renderJoueur(SDL_Renderer *renderer, Joueur *J);
SDL_Rect getJoueurDrawRect(Joueur *J);
void reinitialiserPositionJoueur(Joueur *J);
void appliquerDegatsJoueur(Joueur *J, int damage, Uint32 now);
void respawnJoueur(Joueur *J);
void ajouterScoreJoueur(Joueur *J, int delta);

void initBullets(Bullet bullets[], int size);
void effacerBullets(Bullet bullets[], int size);
void tirerBullet(Bullet bullets[], int size, Joueur *shooter, int owner, Uint32 now);
void updateBullets(Bullet bullets[], int size);
void renderBullets(SDL_Renderer *renderer, Bullet bullets[], int size);

void initNPC(NPC *npc, SDL_Renderer *renderer);
void destroyNPC(NPC *npc);
void removeNPCFromPlay(NPC *npc);
void spawnNPCFromSide(NPC *npc, int side);
void spawnNPCInWave(NPC *npc, int index, int total, int avoidX, int avoidW);
int npcCanSeePlayer(NPC *npc, Joueur *player);
void updateNPC(NPC *npc, Joueur *player, int swarmAlerted,
               int cameraX, int viewW,
               SDL_Rect stablePlatforms[], int stableCount,
               SDL_Rect movingPlatforms[], int movingCount);
void renderNPC(SDL_Renderer *renderer, NPC *npc);
int isNPCActive(NPC *npc);

int collisionAABB(SDL_Rect a, SDL_Rect b);
const CharacterDefinition *getCharacterDefinitions(int *count);
int runCharacterSelectMenu(SDL_Renderer *renderer, TTF_Font *font,
                           CharacterSelection *selection);

#endif
