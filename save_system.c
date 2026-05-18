#include "save_system.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define SAVE_MAGIC "PFINAL_SAVE"
#define SAVE_VERSION 3
#define MAX_SAVE_FILES 9
#define SAVE_MENU_BG "assets/save_load/images/background.png"
#define SAVE_MENU_YES "assets/save_load/images/button_yes.png"
#define SAVE_MENU_NO "assets/save_load/images/button_no.png"
#define SAVE_MENU_LOAD "assets/save_load/images/button_loading.png"
#define SAVE_MENU_NEW "assets/save_load/images/button_new.png"
#define SAVE_MENU_HOVER "assets/save_load/sounds/hover.wav"

typedef struct
{
    SDL_Rect posScreen;
    SDL_Rect posSprite;
    PlayerState state;
    int facing;
    int currentFrame;
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
} SavedPlayer;

typedef struct
{
    float x;
    float y;
    int w;
    int h;
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
    Uint32 frameDelay;
} SavedNPC;

typedef struct
{
    char magic[16];
    int version;
    int bulletCount;
    int npcCount;
    int stableCount;
    int movingCount;
    int joueurActif;
    int puzzleChallengeUsed;
    SavedPlayer J1;
    SavedPlayer J2;
    Bullet bullets[MAX_BULLETS];
    SavedNPC npcs[MAX_NPCS];
    SDL_Rect stablePlatforms[STABLE_PLATFORM_COUNT];
    SDL_Rect movingPlatforms[MOVING_PLATFORM_COUNT];
    int movingDir[MOVING_PLATFORM_COUNT];
    int movingMin[MOVING_PLATFORM_COUNT];
    int movingMax[MOVING_PLATFORM_COUNT];
    int multiplayerMode;
} SaveData;

typedef struct
{
    char path[SAVE_PATH_MAX];
    char name[96];
} SaveEntry;

typedef struct
{
    SDL_Texture *background;
    SDL_Texture *yes;
    SDL_Texture *no;
    SDL_Texture *load;
    SDL_Texture *newGame;
    Mix_Chunk *hover;
} SaveMenuVisuals;

static void copy_player_to_save(SavedPlayer *dst, const Joueur *src)
{
    dst->posScreen = src->posScreen;
    dst->posSprite = src->posSprite;
    dst->state = src->state;
    dst->facing = src->facing;
    dst->currentFrame = src->currentFrame;
    dst->frameDelay = src->frameDelay;
    dst->score = src->score;
    dst->vies = src->vies;
    dst->health = src->health;
    dst->alive = src->alive;
    dst->visible = src->visible;
    dst->startX = src->startX;
    dst->startY = src->startY;
    dst->floorY = src->floorY;
    dst->isJumping = src->isJumping;
    dst->jumpsUsed = src->jumpsUsed;
    dst->jumpHeld = src->jumpHeld;
    dst->velY = src->velY;
    dst->posYFloat = src->posYFloat;
}

static void copy_save_to_player(Joueur *dst, const SavedPlayer *src)
{
    dst->posScreen = src->posScreen;
    dst->posSprite = src->posSprite;
    dst->state = src->state;
    dst->facing = src->facing;
    dst->currentFrame = src->currentFrame;
    dst->lastFrameTime = 0;
    dst->frameDelay = src->frameDelay;
    dst->score = src->score;
    dst->vies = src->vies;
    dst->health = src->health;
    dst->alive = src->alive;
    dst->visible = src->visible;
    dst->startX = src->startX;
    dst->startY = src->startY;
    dst->floorY = src->floorY;
    dst->isJumping = src->isJumping;
    dst->jumpsUsed = src->jumpsUsed;
    dst->jumpHeld = 0;
    dst->velY = src->velY;
    dst->posYFloat = src->posYFloat;
    dst->moveLeft = 0;
    dst->moveRight = 0;
    dst->lastShotTime = 0;
    dst->invulnerableUntil = 0;
    dst->deathTime = 0;
}

static void copy_npc_to_save(SavedNPC *dst, const NPC *src)
{
    dst->x = src->x;
    dst->y = src->y;
    dst->w = src->w;
    dst->h = src->h;
    dst->srcRect = src->srcRect;
    dst->dstRect = src->dstRect;
    dst->useSprite = src->useSprite;
    dst->direction = src->direction;
    dst->posMin = src->posMin;
    dst->posMax = src->posMax;
    dst->groundY = src->groundY;
    dst->action = src->action;
    dst->health = src->health;
    dst->maxHealth = src->maxHealth;
    dst->state = src->state;
    dst->frameDelay = src->frameDelay;
}

static void copy_save_to_npc(NPC *dst, const SavedNPC *src)
{
    dst->x = src->x;
    dst->y = src->y;
    dst->w = src->w;
    dst->h = src->h;
    dst->srcRect = src->srcRect;
    dst->dstRect = src->dstRect;
    dst->useSprite = src->useSprite;
    dst->direction = src->direction;
    dst->posMin = src->posMin;
    dst->posMax = src->posMax;
    dst->groundY = src->groundY;
    dst->action = src->action;
    dst->health = src->health;
    dst->maxHealth = src->maxHealth;
    dst->state = src->state;
    dst->lastFrameTime = 0;
    dst->frameDelay = src->frameDelay;
    dst->attackStartedAt = 0;
    dst->nextAttackAt = 0;
    dst->velY = 0.0f;
    dst->onGround = 1;
    dst->nextJumpAt = SDL_GetTicks() + (Uint32)(rand() % ENEMY_JUMP_COOLDOWN);
}

static void load_save_menu_visuals(SDL_Renderer *renderer, SaveMenuVisuals *visuals)
{
    visuals->background = IMG_LoadTexture(renderer, SAVE_MENU_BG);
    visuals->yes = IMG_LoadTexture(renderer, SAVE_MENU_YES);
    visuals->no = IMG_LoadTexture(renderer, SAVE_MENU_NO);
    visuals->load = IMG_LoadTexture(renderer, SAVE_MENU_LOAD);
    visuals->newGame = IMG_LoadTexture(renderer, SAVE_MENU_NEW);
    visuals->hover = Mix_LoadWAV(SAVE_MENU_HOVER);
}

static void destroy_save_menu_visuals(SaveMenuVisuals *visuals)
{
    if (visuals->background != NULL)
        SDL_DestroyTexture(visuals->background);
    if (visuals->yes != NULL)
        SDL_DestroyTexture(visuals->yes);
    if (visuals->no != NULL)
        SDL_DestroyTexture(visuals->no);
    if (visuals->load != NULL)
        SDL_DestroyTexture(visuals->load);
    if (visuals->newGame != NULL)
        SDL_DestroyTexture(visuals->newGame);
    if (visuals->hover != NULL)
        Mix_FreeChunk(visuals->hover);
    memset(visuals, 0, sizeof(*visuals));
}

static void render_save_background(SDL_Renderer *renderer, SaveMenuVisuals *visuals)
{
    if (visuals != NULL && visuals->background != NULL)
        SDL_RenderCopy(renderer, visuals->background, NULL, NULL);
    else
    {
        SDL_SetRenderDrawColor(renderer, 5, 8, 18, 255);
        SDL_RenderClear(renderer);
    }
}

static int point_in_rect(int x, int y, SDL_Rect rect)
{
    return x >= rect.x && x <= rect.x + rect.w &&
           y >= rect.y && y <= rect.y + rect.h;
}

static void render_image_button(SDL_Renderer *renderer, SDL_Texture *texture,
                                SDL_Rect rect, int hovered)
{
    SDL_Rect draw = rect;

    if (hovered)
    {
        draw.x -= 10;
        draw.y -= 10;
        draw.w += 20;
        draw.h += 20;
    }

    if (texture != NULL)
        SDL_RenderCopy(renderer, texture, NULL, &draw);
    else
    {
        SDL_SetRenderDrawColor(renderer, hovered ? 80 : 45, hovered ? 95 : 55, hovered ? 130 : 80, 255);
        SDL_RenderFillRect(renderer, &draw);
        SDL_SetRenderDrawColor(renderer, 220, 220, 235, 255);
        SDL_RenderDrawRect(renderer, &draw);
    }
}

static void render_prompt(SDL_Renderer *renderer, TTF_Font *font,
                          const char *title, const char *detail)
{
    SaveMenuVisuals visuals;
    SDL_Rect panel;
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color yellow = {255, 220, 80, 255};
    SDL_Surface *surface;
    SDL_Texture *texture;
    SDL_Rect dst;

    memset(&visuals, 0, sizeof(visuals));
    load_save_menu_visuals(renderer, &visuals);
    render_save_background(renderer, &visuals);

    panel.x = SCREEN_W / 2 - 360;
    panel.y = SCREEN_H / 2 - 130;
    panel.w = 720;
    panel.h = 260;
    SDL_SetRenderDrawColor(renderer, 20, 24, 38, 255);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 210, 210, 230, 255);
    SDL_RenderDrawRect(renderer, &panel);

    if (font != NULL)
    {
        surface = TTF_RenderText_Blended(font, title, yellow);
        if (surface != NULL)
        {
            texture = SDL_CreateTextureFromSurface(renderer, surface);
            dst.x = SCREEN_W / 2 - surface->w / 2;
            dst.y = panel.y + 55;
            dst.w = surface->w;
            dst.h = surface->h;
            SDL_FreeSurface(surface);
            if (texture != NULL)
            {
                SDL_RenderCopy(renderer, texture, NULL, &dst);
                SDL_DestroyTexture(texture);
            }
        }

        surface = TTF_RenderText_Blended(font, detail, white);
        if (surface != NULL)
        {
            texture = SDL_CreateTextureFromSurface(renderer, surface);
            dst.x = SCREEN_W / 2 - surface->w / 2;
            dst.y = panel.y + 135;
            dst.w = surface->w;
            dst.h = surface->h;
            SDL_FreeSurface(surface);
            if (texture != NULL)
            {
                SDL_RenderCopy(renderer, texture, NULL, &dst);
                SDL_DestroyTexture(texture);
            }
        }
    }

    SDL_RenderPresent(renderer);
    destroy_save_menu_visuals(&visuals);
}

static void draw_text(SDL_Renderer *renderer, TTF_Font *font,
                      const char *text, int x, int y, SDL_Color color)
{
    SDL_Surface *surface;
    SDL_Texture *texture;
    SDL_Rect dst;

    if (font == NULL)
        return;

    surface = TTF_RenderText_Blended(font, text, color);
    if (surface == NULL)
        return;

    texture = SDL_CreateTextureFromSurface(renderer, surface);
    dst.x = x;
    dst.y = y;
    dst.w = surface->w;
    dst.h = surface->h;
    SDL_FreeSurface(surface);

    if (texture != NULL)
    {
        SDL_RenderCopy(renderer, texture, NULL, &dst);
        SDL_DestroyTexture(texture);
    }
}

static int wait_for_choice(SDL_Renderer *renderer, TTF_Font *font,
                           const char *title, const char *detail,
                           SDL_Keycode yesKey, SDL_Keycode noKey,
                           SDL_Keycode cancelKey)
{
    SaveMenuVisuals visuals;
    SDL_Event event;
    SDL_Rect yesRect = {SCREEN_W / 2 - 250, SCREEN_H / 2 + 45, 210, 105};
    SDL_Rect noRect = {SCREEN_W / 2 + 45, SCREEN_H / 2 + 43, 216, 109};
    int yesHovered = 0;
    int noHovered = 0;
    int mouseX;
    int mouseY;
    int oldYes;
    int oldNo;

    memset(&visuals, 0, sizeof(visuals));
    load_save_menu_visuals(renderer, &visuals);

    while (1)
    {
        SDL_GetMouseState(&mouseX, &mouseY);
        oldYes = yesHovered;
        oldNo = noHovered;
        yesHovered = point_in_rect(mouseX, mouseY, yesRect);
        noHovered = point_in_rect(mouseX, mouseY, noRect);
        if (visuals.hover != NULL &&
            ((yesHovered && !oldYes) || (noHovered && !oldNo)))
            Mix_PlayChannel(-1, visuals.hover, 0);

        render_save_background(renderer, &visuals);
        draw_text(renderer, font, title, SCREEN_W / 2 - 190, SCREEN_H / 2 - 110,
                  (SDL_Color){255, 220, 80, 255});
        draw_text(renderer, font, detail, SCREEN_W / 2 - 320, SCREEN_H / 2 - 55,
                  (SDL_Color){255, 255, 255, 255});
        render_image_button(renderer, visuals.yes, yesRect, yesHovered);
        render_image_button(renderer, visuals.no, noRect, noHovered);
        SDL_RenderPresent(renderer);

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                destroy_save_menu_visuals(&visuals);
                return -1;
            }

            if (event.type == SDL_MOUSEBUTTONDOWN &&
                event.button.button == SDL_BUTTON_LEFT)
            {
                if (point_in_rect(event.button.x, event.button.y, yesRect))
                {
                    destroy_save_menu_visuals(&visuals);
                    return 1;
                }
                if (point_in_rect(event.button.x, event.button.y, noRect))
                {
                    destroy_save_menu_visuals(&visuals);
                    return 0;
                }
            }

            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == yesKey)
                {
                    destroy_save_menu_visuals(&visuals);
                    return 1;
                }
                if (event.key.keysym.sym == noKey)
                {
                    destroy_save_menu_visuals(&visuals);
                    return 0;
                }
                if (cancelKey != 0 && event.key.keysym.sym == cancelKey)
                {
                    destroy_save_menu_visuals(&visuals);
                    return -1;
                }
            }
        }
        SDL_Delay(16);
    }
}

static void sanitize_save_name(const char *input, char *output, int outputSize)
{
    int i;
    int j = 0;
    char c;

    for (i = 0; input[i] != '\0' && j < outputSize - 1; i++)
    {
        c = input[i];
        if ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '_' || c == '-')
            output[j++] = c;
        else if (c == ' ')
            output[j++] = '_';
    }

    if (j == 0)
    {
        snprintf(output, outputSize, "%s", "save");
        return;
    }

    output[j] = '\0';
}

static void render_save_name_prompt(SDL_Renderer *renderer, TTF_Font *font,
                                    const char *name, const char *error)
{
    SaveMenuVisuals visuals;
    SDL_Rect panel;
    SDL_Rect inputBox;
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color yellow = {255, 220, 80, 255};
    SDL_Color muted = {170, 180, 200, 255};
    SDL_Color red = {230, 80, 80, 255};
    char line[128];

    memset(&visuals, 0, sizeof(visuals));
    load_save_menu_visuals(renderer, &visuals);
    render_save_background(renderer, &visuals);

    panel.x = SCREEN_W / 2 - 420;
    panel.y = SCREEN_H / 2 - 170;
    panel.w = 840;
    panel.h = 340;
    SDL_SetRenderDrawColor(renderer, 20, 24, 38, 255);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 210, 210, 230, 255);
    SDL_RenderDrawRect(renderer, &panel);

    draw_text(renderer, font, "Name your save file", panel.x + 35, panel.y + 35, yellow);
    draw_text(renderer, font, "Type a save name, then press Enter to validate.", panel.x + 35, panel.y + 90, muted);

    inputBox.x = panel.x + 35;
    inputBox.y = panel.y + 145;
    inputBox.w = panel.w - 70;
    inputBox.h = 58;
    SDL_SetRenderDrawColor(renderer, 8, 12, 24, 255);
    SDL_RenderFillRect(renderer, &inputBox);
    SDL_SetRenderDrawColor(renderer, 220, 220, 235, 255);
    SDL_RenderDrawRect(renderer, &inputBox);

    snprintf(line, sizeof(line), "%s_", name);
    draw_text(renderer, font, line, inputBox.x + 18, inputBox.y + 13, white);

    draw_text(renderer, font, "Backspace edits. Escape cancels saving.", panel.x + 35, panel.y + 230, muted);
    if (error != NULL && error[0] != '\0')
        draw_text(renderer, font, error, panel.x + 35, panel.y + 275, red);

    SDL_RenderPresent(renderer);
    destroy_save_menu_visuals(&visuals);
}

int save_exists(const char *path)
{
    FILE *file = fopen(path, "rb");

    if (file == NULL)
        return 0;

    fclose(file);
    return 1;
}

int make_save_path(char *path, int pathSize, const char *saveName)
{
    time_t rawTime;
    struct tm *timeInfo;
    char stamp[64];
    char cleanName[64];
    struct stat info;

    if (stat(SAVE_DIR, &info) != 0 && mkdir(SAVE_DIR, 0775) != 0)
        return 0;

    rawTime = time(NULL);
    timeInfo = localtime(&rawTime);
    if (timeInfo == NULL)
        return 0;

    sanitize_save_name(saveName, cleanName, sizeof(cleanName));
    strftime(stamp, sizeof(stamp), "%Y%m%d_%H%M%S", timeInfo);
    snprintf(path, pathSize, "%s/%s_%s_%u.dat", SAVE_DIR, cleanName, stamp, SDL_GetTicks());
    return 1;
}

int save_game_state(const char *path,
                    Joueur *J1, Joueur *J2,
                    Bullet bullets[], int bulletCount,
                    NPC npcs[], int npcCount,
                    SDL_Rect stablePlatforms[], int stableCount,
                    SDL_Rect movingPlatforms[], int movingCount,
                    int movingDir[], int movingMin[], int movingMax[],
                    int joueurActif, int puzzleChallengeUsed,
                    int multiplayerMode)
{
    FILE *file;
    SaveData data;
    int i;

    memset(&data, 0, sizeof(data));
    snprintf(data.magic, sizeof(data.magic), "%s", SAVE_MAGIC);
    data.version = SAVE_VERSION;
    data.bulletCount = bulletCount;
    data.npcCount = npcCount;
    data.stableCount = stableCount;
    data.movingCount = movingCount;
    data.joueurActif = joueurActif;
    data.puzzleChallengeUsed = puzzleChallengeUsed;
    data.multiplayerMode = multiplayerMode;
    copy_player_to_save(&data.J1, J1);
    copy_player_to_save(&data.J2, J2);

    for (i = 0; i < bulletCount && i < MAX_BULLETS; i++)
        data.bullets[i] = bullets[i];

    for (i = 0; i < npcCount && i < MAX_NPCS; i++)
        copy_npc_to_save(&data.npcs[i], &npcs[i]);

    for (i = 0; i < stableCount && i < STABLE_PLATFORM_COUNT; i++)
        data.stablePlatforms[i] = stablePlatforms[i];

    for (i = 0; i < movingCount && i < MOVING_PLATFORM_COUNT; i++)
    {
        data.movingPlatforms[i] = movingPlatforms[i];
        data.movingDir[i] = movingDir[i];
        data.movingMin[i] = movingMin[i];
        data.movingMax[i] = movingMax[i];
    }

    file = fopen(path, "wb");
    if (file == NULL)
        return 0;

    if (fwrite(&data, sizeof(data), 1, file) != 1)
    {
        fclose(file);
        return 0;
    }

    fclose(file);
    return 1;
}

int load_game_state(const char *path,
                    Joueur *J1, Joueur *J2,
                    Bullet bullets[], int bulletCount,
                    NPC npcs[], int npcCount,
                    SDL_Rect stablePlatforms[], int stableCount,
                    SDL_Rect movingPlatforms[], int movingCount,
                    int movingDir[], int movingMin[], int movingMax[],
                    int *joueurActif, int *puzzleChallengeUsed,
                    int *multiplayerMode)
{
    FILE *file;
    SaveData data;
    size_t readBytes;
    int i;

    memset(&data, 0, sizeof(data));
    file = fopen(path, "rb");
    if (file == NULL)
        return 0;

    readBytes = fread(&data, 1, sizeof(data), file);
    if (readBytes < sizeof(data) - sizeof(data.multiplayerMode))
    {
        fclose(file);
        return 0;
    }
    fclose(file);

    if (strcmp(data.magic, SAVE_MAGIC) != 0 ||
        (data.version != SAVE_VERSION && data.version != 2))
        return 0;

    copy_save_to_player(J1, &data.J1);
    copy_save_to_player(J2, &data.J2);

    for (i = 0; i < bulletCount && i < data.bulletCount && i < MAX_BULLETS; i++)
        bullets[i] = data.bullets[i];
    for (; i < bulletCount; i++)
        bullets[i].active = 0;

    for (i = 0; i < npcCount && i < data.npcCount && i < MAX_NPCS; i++)
        copy_save_to_npc(&npcs[i], &data.npcs[i]);

    for (i = 0; i < stableCount && i < data.stableCount && i < STABLE_PLATFORM_COUNT; i++)
        stablePlatforms[i] = data.stablePlatforms[i];

    for (i = 0; i < movingCount && i < data.movingCount && i < MOVING_PLATFORM_COUNT; i++)
    {
        movingPlatforms[i] = data.movingPlatforms[i];
        movingDir[i] = data.movingDir[i];
        movingMin[i] = data.movingMin[i];
        movingMax[i] = data.movingMax[i];
    }

    *joueurActif = data.joueurActif;
    *puzzleChallengeUsed = data.puzzleChallengeUsed;
    if (data.version >= 3 && readBytes >= sizeof(data))
        *multiplayerMode = (data.multiplayerMode != 0);
    else
        *multiplayerMode = 1;
    J1->visible = 1;
    J2->visible = *multiplayerMode;
    if (!*multiplayerMode)
        *joueurActif = 1;

    return 1;
}

static int load_save_entries(SaveEntry entries[], int maxEntries)
{
    DIR *dir;
    struct dirent *entry;
    int count = 0;
    int len;

    if (save_exists("savegame.dat") && count < maxEntries)
    {
        snprintf(entries[count].name, sizeof(entries[count].name), "%s", "savegame.dat");
        snprintf(entries[count].path, sizeof(entries[count].path), "%s", "savegame.dat");
        count++;
    }

    dir = opendir(SAVE_DIR);
    if (dir == NULL)
        return count;

    while ((entry = readdir(dir)) != NULL && count < maxEntries)
    {
        len = (int)strlen(entry->d_name);
        if (len > 4 && len < (int)sizeof(entries[count].name) &&
            strcmp(entry->d_name + len - 4, ".dat") == 0)
        {
            memcpy(entries[count].name, entry->d_name, (size_t)len + 1);
            if (snprintf(entries[count].path, sizeof(entries[count].path), "%s/%s",
                         SAVE_DIR, entry->d_name) < (int)sizeof(entries[count].path))
                count++;
        }
    }

    closedir(dir);
    return count;
}

static void render_save_list(SDL_Renderer *renderer, TTF_Font *font,
                             SaveEntry entries[], int count, int hoverIndex,
                             SDL_Rect newRect, int newHovered)
{
    SaveMenuVisuals visuals;
    SDL_Rect panel;
    SDL_Rect rowRect;
    SDL_Rect loadRect;
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color yellow = {255, 220, 80, 255};
    SDL_Color muted = {170, 180, 200, 255};
    char line[160];
    int i;

    memset(&visuals, 0, sizeof(visuals));
    load_save_menu_visuals(renderer, &visuals);
    render_save_background(renderer, &visuals);

    loadRect.x = SCREEN_W / 2 - 276;
    loadRect.y = 55;
    loadRect.w = 552;
    loadRect.h = 91;
    render_image_button(renderer, visuals.load, loadRect, 0);

    panel.x = SCREEN_W / 2 - 430;
    panel.y = 155;
    panel.w = 860;
    panel.h = SCREEN_H - 290;
    SDL_SetRenderDrawColor(renderer, 20, 24, 38, 255);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 210, 210, 230, 255);
    SDL_RenderDrawRect(renderer, &panel);

    draw_text(renderer, font, "Saved games", panel.x + 35, panel.y + 25, yellow);

    if (count == 0)
        draw_text(renderer, font, "No save files found. Press N to start a new game.", panel.x + 35, panel.y + 92, white);
    else
    {
        draw_text(renderer, font, "Press 1-9 or click a save, or press N for a new game.", panel.x + 35, panel.y + 76, muted);
        for (i = 0; i < count; i++)
        {
            rowRect.x = panel.x + 45;
            rowRect.y = panel.y + 118 + i * 42;
            rowRect.w = panel.w - 90;
            rowRect.h = 36;
            if (i == hoverIndex)
            {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 42);
                SDL_RenderFillRect(renderer, &rowRect);
            }
            snprintf(line, sizeof(line), "%d. %s", i + 1, entries[i].name);
            draw_text(renderer, font, line, panel.x + 55, panel.y + 130 + i * 42, white);
        }
    }

    render_image_button(renderer, visuals.newGame, newRect, newHovered);
    SDL_RenderPresent(renderer);
    destroy_save_menu_visuals(&visuals);
}

int prompt_select_save(SDL_Renderer *renderer, TTF_Font *font, char *path, int pathSize)
{
    SaveEntry entries[MAX_SAVE_FILES];
    SaveMenuVisuals visuals;
    SDL_Event event;
    SDL_Rect newRect = {SCREEN_W / 2 - 276, SCREEN_H - 130, 553, 93};
    SDL_Rect rowRect;
    int count;
    int index;
    int hoverIndex = -1;
    int newHovered = 0;
    int oldHoverIndex;
    int oldNewHovered;
    int mouseX;
    int mouseY;
    int i;

    count = load_save_entries(entries, MAX_SAVE_FILES);
    memset(&visuals, 0, sizeof(visuals));
    load_save_menu_visuals(renderer, &visuals);
    while (1)
    {
        stellarMusicUpdateMenu();
        SDL_GetMouseState(&mouseX, &mouseY);
        oldHoverIndex = hoverIndex;
        oldNewHovered = newHovered;
        hoverIndex = -1;
        newHovered = point_in_rect(mouseX, mouseY, newRect);
        for (i = 0; i < count; i++)
        {
            rowRect.x = SCREEN_W / 2 - 385;
            rowRect.y = 273 + i * 42;
            rowRect.w = 770;
            rowRect.h = 36;
            if (point_in_rect(mouseX, mouseY, rowRect))
                hoverIndex = i;
        }
        if (visuals.hover != NULL &&
            ((hoverIndex >= 0 && hoverIndex != oldHoverIndex) ||
             (newHovered && !oldNewHovered)))
            Mix_PlayChannel(-1, visuals.hover, 0);

        render_save_list(renderer, font, entries, count, hoverIndex, newRect, newHovered);

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                destroy_save_menu_visuals(&visuals);
                return 0;
            }

            if (event.type == SDL_MOUSEBUTTONDOWN &&
                event.button.button == SDL_BUTTON_LEFT)
            {
                if (point_in_rect(event.button.x, event.button.y, newRect))
                {
                    destroy_save_menu_visuals(&visuals);
                    return 0;
                }

                if (hoverIndex >= 0 && hoverIndex < count)
                {
                    snprintf(path, pathSize, "%s", entries[hoverIndex].path);
                    destroy_save_menu_visuals(&visuals);
                    return 1;
                }
            }

            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_n)
                {
                    destroy_save_menu_visuals(&visuals);
                    return 0;
                }

                if (event.key.keysym.sym >= SDLK_1 && event.key.keysym.sym <= SDLK_9)
                {
                    index = event.key.keysym.sym - SDLK_1;
                    if (index >= 0 && index < count)
                    {
                        snprintf(path, pathSize, "%s", entries[index].path);
                        destroy_save_menu_visuals(&visuals);
                        return 1;
                    }
                }
            }
        }
        SDL_Delay(16);
    }
}

int prompt_save_game(SDL_Renderer *renderer, TTF_Font *font)
{
    return wait_for_choice(renderer, font,
                           "Save your progress?",
                           "Press Y to save, N to quit, or ESC to cancel",
                           SDLK_y, SDLK_n, SDLK_ESCAPE);
}

int prompt_save_name(SDL_Renderer *renderer, TTF_Font *font, char *name, int nameSize)
{
    SDL_Event event;
    int len = 0;
    const char *error = "";

    name[0] = '\0';
    SDL_StartTextInput();
    while (1)
    {
        render_save_name_prompt(renderer, font, name, error);

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                SDL_StopTextInput();
                return 0;
            }

            if (event.type == SDL_TEXTINPUT)
            {
                int i;
                for (i = 0; event.text.text[i] != '\0' && len < nameSize - 1; i++)
                {
                    if (event.text.text[i] >= 32 && event.text.text[i] <= 126)
                        name[len++] = event.text.text[i];
                }
                name[len] = '\0';
                error = "";
            }

            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    SDL_StopTextInput();
                    return 0;
                }

                if (event.key.keysym.sym == SDLK_BACKSPACE && len > 0)
                {
                    name[--len] = '\0';
                    error = "";
                }

                if (event.key.keysym.sym == SDLK_RETURN ||
                    event.key.keysym.sym == SDLK_KP_ENTER)
                {
                    if (len == 0)
                    {
                        error = "Please enter a save name before validating.";
                    }
                    else
                    {
                        SDL_StopTextInput();
                        return 1;
                    }
                }
            }
        }
        SDL_Delay(16);
    }
}

void show_save_message(SDL_Renderer *renderer, TTF_Font *font, const char *message)
{
    while (1)
    {
        SDL_Event event;

        render_prompt(renderer, font, message, "Press any key to continue");

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_KEYDOWN || event.type == SDL_QUIT)
                return;
        }
        SDL_Delay(16);
    }
}
