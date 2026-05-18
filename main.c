#include "header.h"
#include "highscore.h"
#include "main_menu.h"
#include "minimap/minimap.h"
#include "save_system.h"


static void afficherHUD(SDL_Renderer *renderer, TTF_Font *font,
                        Joueur *J1, Joueur *J2, int joueurActif)
{
    SDL_Color jaune;
    char txt[128];
    SDL_Surface *s;
    SDL_Texture *t;
    SDL_Rect hudBg;
    SDL_Rect d;
    SDL_Rect lifeRect;
    Joueur *J;
    int i;

    if (joueurActif == 1)
        J = J1;
    else
        J = J2;

    jaune.r = 255;
    jaune.g = 220;
    jaune.b = 50;
    jaune.a = 255;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 220);
    hudBg.x = 10;
    hudBg.y = 10;
    hudBg.w = 360;
    hudBg.h = 55;
    SDL_RenderFillRect(renderer, &hudBg);

    lifeRect.y = 23;
    lifeRect.w = 20;
    lifeRect.h = 20;
    for (i = 0; i < PLAYER_LIVES; i++)
    {
        lifeRect.x = 22 + i * 28;
        if (i < J->vies)
            SDL_SetRenderDrawColor(renderer, 210, 40, 40, 255);
        else
            SDL_SetRenderDrawColor(renderer, 70, 25, 25, 255);
        SDL_RenderFillRect(renderer, &lifeRect);
    }

    if (font == NULL)
        return;

    if (joueurActif == 1)
        snprintf(txt, sizeof(txt), "J1  Score:%d", J1->score);
    else
        snprintf(txt, sizeof(txt), "J2  Score:%d", J2->score);
    s = TTF_RenderText_Solid(font, txt, jaune);

    if (s != NULL)
    {
        t = SDL_CreateTextureFromSurface(renderer, s);
        d.x = 120;
        d.y = 20;
        d.w = s->w;
        d.h = s->h;
        SDL_RenderCopy(renderer, t, NULL, &d);
        SDL_DestroyTexture(t);
        SDL_FreeSurface(s);
    }
}


static int lancerChallengePuzzle(void)
{
    int round;
    int wins = 0;

    for (round = 0; round < PUZZLE_CHALLENGE_ROUNDS; round++)
        if (system(PUZZLE_COMMAND) == 0)
            wins++;

    return wins >= PUZZLE_CHALLENGE_REQUIRED;
}


static int sauvegarderProgression(Joueur *J1, Joueur *J2,
                                  Bullet bullets[], NPC npcs[],
                                  SDL_Rect stablePlatforms[],
                                  SDL_Rect movingPlatforms[],
                                  int movingDir[], int movingMin[], int movingMax[],
                                  int joueurActif, int puzzleChallengeUsed,
                                  int multiplayerMode,
                                  SDL_Renderer *renderer, TTF_Font *font)
{
    char savePath[SAVE_PATH_MAX];
    char saveName[64];

    if (!prompt_save_name(renderer, font, saveName, sizeof(saveName)))
        return 0;

    if (!make_save_path(savePath, sizeof(savePath), saveName))
        return 0;

    return save_game_state(savePath,
                           J1, J2,
                           bullets, MAX_BULLETS,
                           npcs, MAX_NPCS,
                           stablePlatforms, STABLE_PLATFORM_COUNT,
                           movingPlatforms, MOVING_PLATFORM_COUNT,
                           movingDir, movingMin, movingMax,
                           joueurActif, puzzleChallengeUsed,
                           multiplayerMode);
}


static int chevaucheHorizontalement(SDL_Rect player, SDL_Rect platform)
{
    return player.x + player.w > platform.x && player.x < platform.x + platform.w;
}


static void bloquerDessousPlateforme(Joueur *J, SDL_Rect previous, SDL_Rect platform)
{
    int platformBottom = platform.y + platform.h;

    if (!J->alive || !J->isJumping || J->velY >= 0.0f)
        return;

    if (!chevaucheHorizontalement(J->posScreen, platform))
        return;

    if (previous.y >= platformBottom && J->posScreen.y < platformBottom)
    {
        J->posScreen.y = platformBottom;
        J->posYFloat = (float)platformBottom;
        J->velY = 0.0f;
    }
}


static void appliquerCollisionDessousPlateformes(Joueur *J, SDL_Rect previous,
                                                 SDL_Rect stablePlatforms[],
                                                 SDL_Rect movingPlatforms[])
{
    int p;

    for (p = 0; p < STABLE_PLATFORM_COUNT; p++)
        bloquerDessousPlateforme(J, previous, stablePlatforms[p]);

    for (p = 0; p < MOVING_PLATFORM_COUNT; p++)
        bloquerDessousPlateforme(J, previous, movingPlatforms[p]);
}

static void alignerSolVisuel(SDL_Rect *ground, SDL_Texture *texture,
                             int floorLine, int surfaceOffset)
{
    int texW;
    int texH;
    int visibleBelowSurface;

    ground->y = floorLine;
    ground->h = SCREEN_H - floorLine;

    if (texture == NULL)
        return;

    if (SDL_QueryTexture(texture, NULL, NULL, &texW, &texH) != 0)
        return;

    visibleBelowSurface = texH - surfaceOffset;
    if (visibleBelowSurface <= 0)
        return;

    ground->h = ((SCREEN_H - floorLine) * texH + visibleBelowSurface - 1) /
                visibleBelowSurface;
    ground->y = floorLine - (surfaceOffset * ground->h) / texH;
}

static int phasePNJTerminee(NPC npcs[])
{
    int i;

    for (i = 0; i < MAX_NPCS; i++)
        if (npcs[i].state != ENEMY_REMOVED)
            return 0;

    return 1;
}

static void separerPNJEntasses(NPC npcs[])
{
    int i;
    int j;

    for (i = 0; i < MAX_NPCS; i++)
    {
        if (!isNPCActive(&npcs[i]))
            continue;

        for (j = i + 1; j < MAX_NPCS; j++)
        {
            int centerI;
            int centerJ;
            int distance;
            int minSpacing;
            int push;

            if (!isNPCActive(&npcs[j]))
                continue;

            if (npcs[i].dstRect.y + npcs[i].dstRect.h <= npcs[j].dstRect.y ||
                npcs[j].dstRect.y + npcs[j].dstRect.h <= npcs[i].dstRect.y)
                continue;

            centerI = (int)npcs[i].x + npcs[i].w / 2;
            centerJ = (int)npcs[j].x + npcs[j].w / 2;
            distance = centerJ - centerI;
            if (distance < 0)
                distance = -distance;

            minSpacing = (npcs[i].w + npcs[j].w) / 2 + 24;
            if (distance >= minSpacing)
                continue;

            push = (minSpacing - distance + 1) / 2;
            if (centerI <= centerJ)
            {
                npcs[i].x -= push;
                npcs[j].x += push;
            }
            else
            {
                npcs[i].x += push;
                npcs[j].x -= push;
            }

            if (npcs[i].x < 0)
                npcs[i].x = 0.0f;
            if (npcs[j].x < 0)
                npcs[j].x = 0.0f;
            if (npcs[i].x + npcs[i].w > WORLD_W)
                npcs[i].x = (float)(WORLD_W - npcs[i].w);
            if (npcs[j].x + npcs[j].w > WORLD_W)
                npcs[j].x = (float)(WORLD_W - npcs[j].w);

            npcs[i].dstRect.x = (int)npcs[i].x;
            npcs[j].dstRect.x = (int)npcs[j].x;
        }
    }
}

static void lancerPhasePNJ(NPC npcs[], int phaseIndex, int avoidX, int avoidW)
{
    static const int phaseCounts[] = {7, 10, 15};
    int phaseTotal = (int)(sizeof(phaseCounts) / sizeof(phaseCounts[0]));
    int spawnCount;
    int i;

    if (phaseIndex < 0 || phaseIndex >= phaseTotal)
        return;

    spawnCount = phaseCounts[phaseIndex];
    if (spawnCount > MAX_NPCS)
        spawnCount = MAX_NPCS;

    for (i = 0; i < MAX_NPCS; i++)
        removeNPCFromPlay(&npcs[i]);

    for (i = 0; i < spawnCount; i++)
        spawnNPCInWave(&npcs[i], i, spawnCount, avoidX, avoidW);
}

static void spawnSecondaryEntity(SecondaryEntity *entity)
{
    entity->w = GEM_W;
    entity->h = GEM_H;
    entity->active = 1;
    entity->x = (float)(SHIP_END_X + 80 +
                        rand() % (WORLD_W - SHIP_END_X - GEM_W - 160));
    entity->y = (float)(MARS_GROUND_Y + PLAYER_H - entity->h);
}

static int initSecondaryEntities(SecondaryEntity entities[], int count)
{
    int i;
    int spawnCount;

    for (i = 0; i < count; i++)
        entities[i].active = 0;

    spawnCount = MIN_SECONDARY_ENTITIES +
                 rand() % (MAX_SECONDARY_ENTITIES - MIN_SECONDARY_ENTITIES + 1);
    if (spawnCount > count)
        spawnCount = count;

    for (i = 0; i < spawnCount; i++)
        spawnSecondaryEntity(&entities[i]);

    return spawnCount;
}

static void renderSecondaryEntities(SDL_Renderer *renderer,
                                    SecondaryEntity entities[], int count,
                                    SDL_Texture *gemTexture, int cameraX)
{
    int i;

    for (i = 0; i < count; i++)
    {
        SDL_Rect dst;

        if (!entities[i].active)
            continue;

        dst.x = (int)entities[i].x - cameraX;
        dst.y = (int)entities[i].y;
        dst.w = entities[i].w;
        dst.h = entities[i].h;

        if (gemTexture != NULL)
            SDL_RenderCopy(renderer, gemTexture, NULL, &dst);
        else
        {
            SDL_SetRenderDrawColor(renderer, 30, 190, 255, 255);
            SDL_RenderFillRect(renderer, &dst);
        }
    }
}


static void afficherDebugCollision(SDL_Renderer *renderer,
                                   Joueur *J1, Joueur *J2,
                                   int cameraX,
                                   SDL_Rect shipGround, SDL_Rect marsGround,
                                   SDL_Rect stablePlatforms[],
                                   SDL_Rect movingPlatforms[])
{
    SDL_Rect r;
    SDL_Rect draw;
    Joueur temp;
    int p;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    r = shipGround;
    r.x -= cameraX;
    SDL_SetRenderDrawColor(renderer, 255, 220, 0, 190);
    SDL_RenderDrawRect(renderer, &r);
    SDL_RenderDrawLine(renderer, r.x, r.y, r.x + r.w, r.y);

    r = marsGround;
    r.x -= cameraX;
    SDL_SetRenderDrawColor(renderer, 255, 180, 0, 190);
    SDL_RenderDrawRect(renderer, &r);
    SDL_RenderDrawLine(renderer, r.x, r.y, r.x + r.w, r.y);

    SDL_SetRenderDrawColor(renderer, 255, 0, 255, 190);
    for (p = 0; p < STABLE_PLATFORM_COUNT; p++)
    {
        r = stablePlatforms[p];
        r.x -= cameraX;
        SDL_RenderDrawRect(renderer, &r);
        SDL_RenderDrawLine(renderer, r.x, r.y, r.x + r.w, r.y);
    }

    SDL_SetRenderDrawColor(renderer, 0, 255, 255, 190);
    for (p = 0; p < MOVING_PLATFORM_COUNT; p++)
    {
        r = movingPlatforms[p];
        r.x -= cameraX;
        SDL_RenderDrawRect(renderer, &r);
        SDL_RenderDrawLine(renderer, r.x, r.y, r.x + r.w, r.y);
    }

    temp = *J1;
    temp.posScreen.x -= cameraX;
    draw = getJoueurDrawRect(&temp);
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 220);
    SDL_RenderDrawRect(renderer, &temp.posScreen);
    SDL_SetRenderDrawColor(renderer, 80, 160, 255, 220);
    SDL_RenderDrawRect(renderer, &draw);
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 240);
    SDL_RenderDrawLine(renderer, temp.posScreen.x,
                       temp.posScreen.y + temp.posScreen.h,
                       temp.posScreen.x + temp.posScreen.w,
                       temp.posScreen.y + temp.posScreen.h);

    temp = *J2;
    temp.posScreen.x -= cameraX;
    draw = getJoueurDrawRect(&temp);
    SDL_SetRenderDrawColor(renderer, 0, 210, 0, 180);
    SDL_RenderDrawRect(renderer, &temp.posScreen);
    SDL_SetRenderDrawColor(renderer, 80, 130, 255, 180);
    SDL_RenderDrawRect(renderer, &draw);
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 220);
    SDL_RenderDrawLine(renderer, temp.posScreen.x,
                       temp.posScreen.y + temp.posScreen.h,
                       temp.posScreen.x + temp.posScreen.w,
                       temp.posScreen.y + temp.posScreen.h);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}


int main(void)
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *font;
    Joueur J1;
    Joueur J2;
    Bullet bullets[MAX_BULLETS];
    NPC npcs[MAX_NPCS];
    SecondaryEntity gems[MAX_SECONDARY_ENTITIES];
    SDL_Event event;
    const Uint8 *keys;
    SDL_Rect shipGround;
    SDL_Rect marsGround;
    SDL_Rect stablePlatforms[STABLE_PLATFORM_COUNT];
    SDL_Rect movingPlatforms[MOVING_PLATFORM_COUNT];
    SDL_Rect platformDraw;
    SDL_Rect camera;
    SDL_Rect bgSrc;
    SDL_Rect bgDst;
    SDL_Rect viewports[2];
    SDL_Rect mapDst;
    SDL_Rect mapPlayer;
    SDL_Rect mapPlatform;
    SDL_Rect midSrc;
    SDL_Rect foreSrc;
    SDL_Rect lightRect;
    SDL_Rect savedJ1;
    SDL_Rect savedJ2;
    SDL_Rect savedBullet;
    SDL_Rect savedNpc;
    SDL_Rect previousPlayer;
    SDL_Texture *background;
    SDL_Texture *backgroundMid;
    SDL_Texture *backgroundFore;
    SDL_Texture *shipGroundTex;
    SDL_Texture *marsGroundTex;
    SDL_Texture *shipPlatformTex;
    SDL_Texture *marsPlatformTex;
    SDL_Texture *gemTexture;
    SDL_Texture *mapTexture;
    SDL_Surface *mapFrame;
    Minimap minimap;
    Enemy mapEnemies[MAX_ENEMIES];
    Highscore hs;
    const CharacterDefinition *characters;
    CharacterSelection selection;
    int running;
    int i;
    int p;
    int cameraX;
    int npcUpdateCameraX;
    int npcUpdateViewW;
    int renderPass;
    int renderPasses;
    int sharedView;
    int viewW;
    int playerDistance;
    int minimapReady;
    int gemCount;
    int gemDamageBuffTimer;
    int movingDir[MOVING_PLATFORM_COUNT];
    int movingMin[MOVING_PLATFORM_COUNT];
    int movingMax[MOVING_PLATFORM_COUNT];
    int npcSwarmAlerted;
    int npcPhaseIndex;
    int joueurActif;
    int puzzleChallengeUsed;
    int multiplayerMode;
    int selectedSaveLoaded;
    int menuChoice;
    int quitterSansScore;
    int debugCollision;
    int finalScore;
    int characterCount;
    char selectedSave[SAVE_PATH_MAX];
    Uint32 now;

    if (!initSDL(&window, &renderer))
        return 1;

    font = TTF_OpenFont(PLUTO_FONT_PATH, 28);
    if (font == NULL)
        printf("[ERREUR] TTF_OpenFont : %s\n", TTF_GetError());

    menuChoice = run_main_menu(renderer);
    if (menuChoice != MAIN_MENU_START)
    {
        if (font != NULL)
            TTF_CloseFont(font);
        shutdownSDL(window, renderer);
        return 0;
    }

    selectedSaveLoaded = prompt_select_save(renderer, font, selectedSave, sizeof(selectedSave));
    if (selectedSaveLoaded < 0)
    {
        if (font != NULL)
            TTF_CloseFont(font);
        shutdownSDL(window, renderer);
        return 0;
    }

    multiplayerMode = 1;
    if (!selectedSaveLoaded)
    {
        multiplayerMode = prompt_game_mode(renderer, font);
        if (multiplayerMode < 0)
        {
            if (font != NULL)
                TTF_CloseFont(font);
            shutdownSDL(window, renderer);
            return 0;
        }
        multiplayerMode = (multiplayerMode == GAME_MODE_MULTI);
    }

    characters = getCharacterDefinitions(&characterCount);
    selection.p1CharacterIndex = 0;
    selection.p1OutfitIndex = 0;
    selection.p2CharacterIndex = (characterCount > 1) ? 1 : 0;
    selection.p2OutfitIndex = 0;

    if (!selectedSaveLoaded && multiplayerMode && characterCount > 0 && !runCharacterSelectMenu(renderer, font, &selection))
    {
        if (font != NULL)
            TTF_CloseFont(font);
        shutdownSDL(window, renderer);
        return 0;
    }

    stellarMusicStartGameplay();

    background = IMG_LoadTexture(renderer, "assets/mars_ship_level_full.png");
    backgroundMid = IMG_LoadTexture(renderer, "assets/mars_ship_level_mid.png");
    backgroundFore = IMG_LoadTexture(renderer, "assets/mars_ship_level_foreground.png");
    shipGroundTex = IMG_LoadTexture(renderer, "assets/ship_ground_pixel.png");
    marsGroundTex = IMG_LoadTexture(renderer, "assets/mars_ground_pixel.png");
    shipPlatformTex = IMG_LoadTexture(renderer, "assets/ship_platform_pixel.png");
    marsPlatformTex = IMG_LoadTexture(renderer, "assets/mars_platform_pixel.png");
    gemTexture = IMG_LoadTexture(renderer, "assets/blue_gem.png");
    if (background == NULL)
    {
        printf("[ERREUR] IMG_LoadTexture : %s\n", IMG_GetError());
        if (font != NULL)
            TTF_CloseFont(font);
        shutdownSDL(window, renderer);
        return 1;
    }
    if (backgroundMid != NULL)
        SDL_SetTextureBlendMode(backgroundMid, SDL_BLENDMODE_BLEND);
    if (backgroundFore != NULL)
        SDL_SetTextureBlendMode(backgroundFore, SDL_BLENDMODE_BLEND);
    if (shipGroundTex != NULL)
        SDL_SetTextureBlendMode(shipGroundTex, SDL_BLENDMODE_BLEND);
    if (marsGroundTex != NULL)
        SDL_SetTextureBlendMode(marsGroundTex, SDL_BLENDMODE_BLEND);
    if (shipPlatformTex != NULL)
        SDL_SetTextureBlendMode(shipPlatformTex, SDL_BLENDMODE_BLEND);
    if (marsPlatformTex != NULL)
        SDL_SetTextureBlendMode(marsPlatformTex, SDL_BLENDMODE_BLEND);
    if (gemTexture != NULL)
        SDL_SetTextureBlendMode(gemTexture, SDL_BLENDMODE_BLEND);

    mapFrame = SDL_CreateRGBSurfaceWithFormat(0, SCREEN_W, SCREEN_H, 32, SDL_PIXELFORMAT_RGBA32);
    minimapReady = 0;
    if (mapFrame != NULL && init_minimap(&minimap, "minimap/minimap_pixel.bmp", SCREEN_W, SCREEN_H, WORLD_W, WORLD_H) == 0)
        minimapReady = 1;

    if (characterCount > 0)
    {
        if (selection.p1CharacterIndex < 0 || selection.p1CharacterIndex >= characterCount)
            selection.p1CharacterIndex = 0;
        if (selection.p2CharacterIndex < 0 || selection.p2CharacterIndex >= characterCount)
            selection.p2CharacterIndex = 0;
        if (selection.p1OutfitIndex < 0 || selection.p1OutfitIndex > 1)
            selection.p1OutfitIndex = 0;
        if (selection.p2OutfitIndex < 0 || selection.p2OutfitIndex > 1)
            selection.p2OutfitIndex = 0;
    }

    if (characterCount > 0)
    {
        if (!initialiserJoueurAvecAssets(&J1, renderer, 100, GROUND_Y,
                                         &characters[selection.p1CharacterIndex].outfits[selection.p1OutfitIndex]))
        {
            if (font != NULL)
                TTF_CloseFont(font);
            shutdownSDL(window, renderer);
            return 1;
        }
    }
    else if (!initialiserJoueur(&J1, renderer, 100, GROUND_Y))
    {
        if (font != NULL)
            TTF_CloseFont(font);
        shutdownSDL(window, renderer);
        return 1;
    }

    if (characterCount > 0)
    {
        if (!initialiserJoueurAvecAssets(&J2, renderer, 1000, GROUND_Y,
                                         &characters[selection.p2CharacterIndex].outfits[selection.p2OutfitIndex]))
        {
            libererJoueur(&J1);
            if (font != NULL)
                TTF_CloseFont(font);
            shutdownSDL(window, renderer);
            return 1;
        }
    }
    else if (!initialiserJoueur(&J2, renderer, 1000, GROUND_Y))
    {
        libererJoueur(&J1);
        if (font != NULL)
            TTF_CloseFont(font);
        shutdownSDL(window, renderer);
        return 1;
    }

    srand((unsigned)SDL_GetTicks());
    initBullets(bullets, MAX_BULLETS);
    gemCount = initSecondaryEntities(gems, MAX_SECONDARY_ENTITIES);
    gemDamageBuffTimer = 0;
    J1.visible = 1;
    J2.visible = multiplayerMode;
    J2.facing = FACE_LEFT;
    joueurActif = 1;

    npcPhaseIndex = 0;
    for (i = 0; i < MAX_NPCS; i++)
    {
        npcs[i] = (NPC){0, MARS_GROUND_Y + PLAYER_H - 128, 128, 128, NULL, NULL, NULL, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, 0, MARS_GROUND_Y, 0, 0, 0, ENEMY_REMOVED, 0, 0, 0, 0, 0.0f, 1, 0};
        initNPC(&npcs[i], renderer);
        removeNPCFromPlay(&npcs[i]);
    }
    lancerPhasePNJ(npcs, npcPhaseIndex, 0, SCREEN_W);

    shipGround.x = 0;
    shipGround.y = GROUND_Y + PLAYER_H;
    shipGround.w = SHIP_END_X;
    shipGround.h = SCREEN_H - shipGround.y;
    alignerSolVisuel(&shipGround, shipGroundTex,
                     GROUND_Y + PLAYER_H, SHIP_GROUND_SURFACE_OFFSET);
    marsGround.x = SHIP_END_X;
    marsGround.y = MARS_GROUND_Y + PLAYER_H;
    marsGround.w = WORLD_W - SHIP_END_X;
    marsGround.h = SCREEN_H - marsGround.y;
    alignerSolVisuel(&marsGround, marsGroundTex,
                     MARS_GROUND_Y + PLAYER_H, MARS_GROUND_SURFACE_OFFSET);
    stablePlatforms[0] = (SDL_Rect){900, 410, 360, 34};
    stablePlatforms[1] = (SDL_Rect){2150, 440, 360, 34};
    stablePlatforms[2] = (SDL_Rect){3900, 425, 360, 34};
    movingPlatforms[0] = (SDL_Rect){1520, 360, 260, 30};
    movingPlatforms[1] = (SDL_Rect){3300, 370, 260, 30};
    movingDir[0] = 1;
    movingDir[1] = -1;
    movingMin[0] = 1420;
    movingMin[1] = 3180;
    movingMax[0] = 1900;
    movingMax[1] = 3700;

    running = 1;
    quitterSansScore = 0;
    debugCollision = 0;
    npcSwarmAlerted = 0;
    puzzleChallengeUsed = 0;
    for (i = 0; i < MAX_ENEMIES; i++)
        mapEnemies[i].active = 0;
    camera.x = 0;
    camera.y = 0;
    camera.w = SCREEN_W;
    camera.h = SCREEN_H;
    bgDst.x = 0;
    bgDst.y = 0;
    bgDst.w = SCREEN_W;
    bgDst.h = SCREEN_H;
    viewports[0] = (SDL_Rect){0, 0, SCREEN_W / 2, SCREEN_H};
    viewports[1] = (SDL_Rect){SCREEN_W / 2, 0, SCREEN_W / 2, SCREEN_H};

    if (selectedSaveLoaded)
    {
        if (!load_game_state(selectedSave,
                             &J1, &J2,
                             bullets, MAX_BULLETS,
                             npcs, MAX_NPCS,
                             stablePlatforms, STABLE_PLATFORM_COUNT,
                             movingPlatforms, MOVING_PLATFORM_COUNT,
                             movingDir, movingMin, movingMax,
                             &joueurActif, &puzzleChallengeUsed,
                             &multiplayerMode))
            show_save_message(renderer, font, "Could not load saved game");
    }

    while (running)
    {
        stellarMusicUpdateGameplay();
        J1.visible = 1;
        J2.visible = multiplayerMode;
        if (multiplayerMode)
        {
            playerDistance = J1.posScreen.x + J1.posScreen.w / 2 -
                             (J2.posScreen.x + J2.posScreen.w / 2);
            if (playerDistance < 0)
                playerDistance = -playerDistance;
            sharedView = playerDistance <= SCREEN_W - PLAYER_W;
        }
        else
            sharedView = 1;
        renderPasses = sharedView ? 1 : 2;

        SDL_RenderSetViewport(renderer, NULL);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        for (renderPass = 0; renderPass < renderPasses; renderPass++)
        {
        if (sharedView)
        {
            int j1Center = J1.posScreen.x + J1.posScreen.w / 2;
            int j2Center = J2.posScreen.x + J2.posScreen.w / 2;

            viewW = SCREEN_W;
            if (multiplayerMode)
                cameraX = (j1Center + j2Center) / 2 - viewW / 2;
            else
                cameraX = j1Center - viewW / 2;
            SDL_RenderSetViewport(renderer, NULL);
        }
        else
        {
            Joueur *focus = (renderPass == 0) ? &J1 : &J2;

            viewW = SCREEN_W / 2;
            cameraX = focus->posScreen.x + focus->posScreen.w / 2 - viewW / 2;
            SDL_RenderSetViewport(renderer, &viewports[renderPass]);
        }

        if (cameraX < 0)
            cameraX = 0;
        if (cameraX > WORLD_W - viewW)
            cameraX = WORLD_W - viewW;

        camera.x = cameraX;
        bgSrc.x = camera.x;
        bgSrc.y = 0;
        bgSrc.w = SCREEN_W;
        bgSrc.h = SCREEN_H;
        midSrc.x = camera.x;
        midSrc.y = 0;
        midSrc.w = SCREEN_W;
        midSrc.h = SCREEN_H;
        foreSrc.x = camera.x;
        foreSrc.y = 0;
        foreSrc.w = SCREEN_W;
        foreSrc.h = SCREEN_H;

        SDL_RenderCopy(renderer, background, &bgSrc, &bgDst);
        if (backgroundMid != NULL)
            SDL_RenderCopy(renderer, backgroundMid, &midSrc, &bgDst);
        platformDraw = shipGround;
        platformDraw.x -= camera.x;
        if (shipGroundTex != NULL)
            SDL_RenderCopy(renderer, shipGroundTex, NULL, &platformDraw);
        else
        {
            SDL_SetRenderDrawColor(renderer, 62, 66, 72, 255);
            SDL_RenderFillRect(renderer, &platformDraw);
        }
        platformDraw = marsGround;
        platformDraw.x -= camera.x;
        if (marsGroundTex != NULL)
            SDL_RenderCopy(renderer, marsGroundTex, NULL, &platformDraw);
        else
        {
            SDL_SetRenderDrawColor(renderer, 118, 58, 38, 255);
            SDL_RenderFillRect(renderer, &platformDraw);
        }
        for (p = 0; p < STABLE_PLATFORM_COUNT; p++)
        {
            platformDraw = stablePlatforms[p];
            platformDraw.x -= camera.x;
            if (stablePlatforms[p].x < SHIP_END_X && shipPlatformTex != NULL)
                SDL_RenderCopy(renderer, shipPlatformTex, NULL, &platformDraw);
            else if (stablePlatforms[p].x >= SHIP_END_X && marsPlatformTex != NULL)
                SDL_RenderCopy(renderer, marsPlatformTex, NULL, &platformDraw);
            else if (stablePlatforms[p].x < SHIP_END_X)
            {
                SDL_SetRenderDrawColor(renderer, 78, 82, 88, 255);
                SDL_RenderFillRect(renderer, &platformDraw);
            }
            else
            {
                SDL_SetRenderDrawColor(renderer, 150, 74, 42, 255);
                SDL_RenderFillRect(renderer, &platformDraw);
            }
        }
        for (p = 0; p < MOVING_PLATFORM_COUNT; p++)
        {
            platformDraw = movingPlatforms[p];
            platformDraw.x -= camera.x;
            if (movingPlatforms[p].x < SHIP_END_X && shipPlatformTex != NULL)
                SDL_RenderCopy(renderer, shipPlatformTex, NULL, &platformDraw);
            else if (marsPlatformTex != NULL)
                SDL_RenderCopy(renderer, marsPlatformTex, NULL, &platformDraw);
            else
            {
                SDL_SetRenderDrawColor(renderer, 118, 112, 96, 255);
                SDL_RenderFillRect(renderer, &platformDraw);
            }
        }

        renderSecondaryEntities(renderer, gems, gemCount, gemTexture, camera.x);

        savedJ1 = J1.posScreen;
        savedJ2 = J2.posScreen;
        J1.posScreen.x -= camera.x;
        J2.posScreen.x -= camera.x;
        renderJoueur(renderer, &J1);
        if (multiplayerMode)
            renderJoueur(renderer, &J2);

        for (i = 0; i < MAX_BULLETS; i++)
        {
            if (!bullets[i].active)
                continue;
            savedBullet = bullets[i].rect;
            bullets[i].rect.x -= camera.x;
            renderBullets(renderer, &bullets[i], 1);
            bullets[i].rect = savedBullet;
        }

        for (i = 0; i < MAX_NPCS; i++)
        {
            savedNpc = npcs[i].dstRect;
            npcs[i].dstRect.x -= camera.x;
            renderNPC(renderer, &npcs[i]);
            npcs[i].dstRect = savedNpc;
        }
        J1.posScreen = savedJ1;
        J2.posScreen = savedJ2;

        if (backgroundFore != NULL)
            SDL_RenderCopy(renderer, backgroundFore, &foreSrc, &bgDst);

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        if (camera.x < SHIP_END_X)
        {
            SDL_SetRenderDrawColor(renderer, 3, 5, 12, 76);
            SDL_RenderFillRect(renderer, NULL);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
            lightRect.x = 160 - camera.x;
            lightRect.y = 80;
            lightRect.w = 280;
            lightRect.h = 520;
            SDL_SetRenderDrawColor(renderer, 255, 115, 35, 38);
            SDL_RenderFillRect(renderer, &lightRect);
            lightRect.x = 1120 - camera.x;
            lightRect.y = 30;
            lightRect.w = 360;
            lightRect.h = 540;
            SDL_SetRenderDrawColor(renderer, 255, 55, 35, 30);
            SDL_RenderFillRect(renderer, &lightRect);
            lightRect.x = 2140 - camera.x;
            lightRect.y = 90;
            lightRect.w = 420;
            lightRect.h = 520;
            SDL_SetRenderDrawColor(renderer, 255, 150, 55, 34);
            SDL_RenderFillRect(renderer, &lightRect);
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, 18, 12, 9, 44);
            SDL_RenderFillRect(renderer, NULL);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
            lightRect.x = SHIP_END_X - camera.x;
            lightRect.y = 0;
            lightRect.w = SCREEN_W;
            lightRect.h = SCREEN_H;
            SDL_SetRenderDrawColor(renderer, 120, 74, 48, 22);
            SDL_RenderFillRect(renderer, &lightRect);
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        lightRect.x = 0;
        lightRect.y = 0;
        lightRect.w = SCREEN_W;
        lightRect.h = 90;
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 70);
        SDL_RenderFillRect(renderer, &lightRect);
        lightRect.y = SCREEN_H - 90;
        SDL_RenderFillRect(renderer, &lightRect);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        if (minimapReady)
        {
            SDL_FillRect(mapFrame, NULL, SDL_MapRGBA(mapFrame->format, 0, 0, 0, 0));
            if (!multiplayerMode || joueurActif == 1)
                mapPlayer = J1.posScreen;
            else
                mapPlayer = J2.posScreen;
            mapPlayer.x -= camera.x;
            for (i = 0; i < MAX_NPCS; i++)
            {
                mapEnemies[i].pos = npcs[i].dstRect;
                mapEnemies[i].active = isNPCActive(&npcs[i]);
            }
            update_minimap(&minimap, mapPlayer, camera);
            display_minimap(minimap, mapFrame);
            for (p = 0; p < STABLE_PLATFORM_COUNT; p++)
            {
                mapPlatform.x = minimap.pos.x + (int)(stablePlatforms[p].x * minimap.scaleX);
                mapPlatform.y = minimap.pos.y + (int)(stablePlatforms[p].y * minimap.scaleY);
                mapPlatform.w = (int)(stablePlatforms[p].w * minimap.scaleX);
                mapPlatform.h = 3;
                SDL_FillRect(mapFrame, &mapPlatform, SDL_MapRGB(mapFrame->format, 210, 180, 90));
            }
            for (p = 0; p < MOVING_PLATFORM_COUNT; p++)
            {
                mapPlatform.x = minimap.pos.x + (int)(movingPlatforms[p].x * minimap.scaleX);
                mapPlatform.y = minimap.pos.y + (int)(movingPlatforms[p].y * minimap.scaleY);
                mapPlatform.w = (int)(movingPlatforms[p].w * minimap.scaleX);
                mapPlatform.h = 3;
                SDL_FillRect(mapFrame, &mapPlatform, SDL_MapRGB(mapFrame->format, 90, 220, 230));
            }
            display_entities_on_map(minimap, mapFrame, mapEnemies);
            mapTexture = SDL_CreateTextureFromSurface(renderer, mapFrame);
            if (mapTexture != NULL)
            {
                SDL_SetTextureBlendMode(mapTexture, SDL_BLENDMODE_BLEND);
                mapDst.x = 0;
                mapDst.y = 0;
                mapDst.w = SCREEN_W;
                mapDst.h = SCREEN_H;
                SDL_RenderCopy(renderer, mapTexture, NULL, &mapDst);
                SDL_DestroyTexture(mapTexture);
            }
        }
        if (debugCollision)
            afficherDebugCollision(renderer, &J1, &J2, camera.x,
                                   shipGround, marsGround,
                                   stablePlatforms, movingPlatforms);
        afficherHUD(renderer, font, &J1, &J2,
                    multiplayerMode ? (sharedView ? joueurActif : renderPass + 1) : 1);
        }
        SDL_RenderSetViewport(renderer, NULL);
        if (!sharedView)
        {
            SDL_Rect divider = {SCREEN_W / 2 - 2, 0, 4, SCREEN_H};

            SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
            SDL_RenderFillRect(renderer, &divider);
        }
        SDL_RenderPresent(renderer);

        now = SDL_GetTicks();
        keys = SDL_GetKeyboardState(NULL);

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                int saveChoice = prompt_save_game(renderer, font);

                if (saveChoice == 1)
                {
                    if (sauvegarderProgression(&J1, &J2, bullets, npcs,
                                                stablePlatforms, movingPlatforms,
                                                movingDir, movingMin, movingMax,
                                                joueurActif, puzzleChallengeUsed,
                                                multiplayerMode,
                                                renderer, font))
                        running = 0;
                    else
                        show_save_message(renderer, font, "Save cancelled");
                }
                else if (saveChoice == 0)
                {
                    quitterSansScore = 1;
                    running = 0;
                }
            }

            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                {
                    int saveChoice = prompt_save_game(renderer, font);

                    if (saveChoice == 1)
                    {
                        if (sauvegarderProgression(&J1, &J2, bullets, npcs,
                                                   stablePlatforms, movingPlatforms,
                                                   movingDir, movingMin, movingMax,
                                                   joueurActif, puzzleChallengeUsed,
                                                   multiplayerMode,
                                                   renderer, font))
                            running = 0;
                        else
                            show_save_message(renderer, font, "Save cancelled");
                    }
                    else if (saveChoice == 0)
                    {
                        quitterSansScore = 1;
                        running = 0;
                    }
                }

                if (event.key.keysym.scancode == SDL_SCANCODE_F5)
                {
                    if (sauvegarderProgression(&J1, &J2, bullets, npcs,
                                               stablePlatforms, movingPlatforms,
                                               movingDir, movingMin, movingMax,
                                               joueurActif, puzzleChallengeUsed,
                                               multiplayerMode,
                                               renderer, font))
                        show_save_message(renderer, font, "Game saved");
                    else
                        show_save_message(renderer, font, "Save failed");
                }

                if (event.key.keysym.scancode == SDL_SCANCODE_F3)
                    debugCollision = !debugCollision;

                if (event.key.keysym.scancode == SDL_SCANCODE_M && multiplayerMode)
                {
                    joueurActif = (joueurActif == 1) ? 2 : 1;
                }

                if (event.key.keysym.scancode == SDL_SCANCODE_E)
                    tirerBullet(bullets, MAX_BULLETS, &J1, 1, now);

                if (multiplayerMode &&
                    (event.key.keysym.scancode == SDL_SCANCODE_RCTRL ||
                     event.key.keysym.scancode == SDL_SCANCODE_RETURN))
                {
                    tirerBullet(bullets, MAX_BULLETS, &J2, 2, now);
                }
            }

            if (event.type == SDL_MOUSEBUTTONDOWN &&
                event.button.button == SDL_BUTTON_LEFT &&
                multiplayerMode)
            {
                tirerBullet(bullets, MAX_BULLETS, &J2, 2, now);
            }
        }

        if (!running)
            continue;

        for (p = 0; p < MOVING_PLATFORM_COUNT; p++)
        {
            movingPlatforms[p].x += movingDir[p] * 2;
            if (movingPlatforms[p].x < movingMin[p] || movingPlatforms[p].x > movingMax[p])
            {
                movingDir[p] = -movingDir[p];
                movingPlatforms[p].x += movingDir[p] * 2;
            }
        }

        if (J1.posScreen.x >= SHIP_END_X)
            J1.floorY = MARS_GROUND_Y;
        else
            J1.floorY = GROUND_Y;
        if (multiplayerMode)
        {
            if (J2.posScreen.x >= SHIP_END_X)
                J2.floorY = MARS_GROUND_Y;
            else
                J2.floorY = GROUND_Y;
        }
        for (p = 0; p < STABLE_PLATFORM_COUNT; p++)
        {
            if (J1.posScreen.x + J1.posScreen.w > stablePlatforms[p].x &&
                J1.posScreen.x < stablePlatforms[p].x + stablePlatforms[p].w &&
                J1.posScreen.y + J1.posScreen.h <= stablePlatforms[p].y + 12)
                J1.floorY = stablePlatforms[p].y - PLAYER_H;
            if (multiplayerMode &&
                J2.posScreen.x + J2.posScreen.w > stablePlatforms[p].x &&
                J2.posScreen.x < stablePlatforms[p].x + stablePlatforms[p].w &&
                J2.posScreen.y + J2.posScreen.h <= stablePlatforms[p].y + 12)
                J2.floorY = stablePlatforms[p].y - PLAYER_H;
        }
        for (p = 0; p < MOVING_PLATFORM_COUNT; p++)
        {
            if (J1.posScreen.x + J1.posScreen.w > movingPlatforms[p].x &&
                J1.posScreen.x < movingPlatforms[p].x + movingPlatforms[p].w &&
                J1.posScreen.y + J1.posScreen.h <= movingPlatforms[p].y + 12)
                J1.floorY = movingPlatforms[p].y - PLAYER_H;
            if (multiplayerMode &&
                J2.posScreen.x + J2.posScreen.w > movingPlatforms[p].x &&
                J2.posScreen.x < movingPlatforms[p].x + movingPlatforms[p].w &&
                J2.posScreen.y + J2.posScreen.h <= movingPlatforms[p].y + 12)
                J2.floorY = movingPlatforms[p].y - PLAYER_H;
        }

        previousPlayer = J1.posScreen;
        gererEntreeJoueurClavier(&J1, keys, SDL_SCANCODE_A, SDL_SCANCODE_D, SDL_SCANCODE_W);
        updateJoueur(&J1, now);
        appliquerCollisionDessousPlateformes(&J1, previousPlayer,
                                             stablePlatforms, movingPlatforms);

        if (multiplayerMode)
        {
            previousPlayer = J2.posScreen;
            gererEntreeJoueurClavier(&J2, keys, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_UP);
            updateJoueur(&J2, now);
            appliquerCollisionDessousPlateformes(&J2, previousPlayer,
                                                 stablePlatforms, movingPlatforms);
        }

        if (running)
        {
            Joueur *joueurCourant;
            if (!multiplayerMode || joueurActif == 1)
                joueurCourant = &J1;
            else
                joueurCourant = &J2;

            if (gemDamageBuffTimer > 0)
                gemDamageBuffTimer--;

            if (joueurCourant->alive && joueurCourant->visible)
            {
                for (i = 0; i < gemCount; i++)
                {
                    SDL_Rect gemRect;

                    if (!gems[i].active)
                        continue;

                    gemRect.x = (int)gems[i].x;
                    gemRect.y = (int)gems[i].y;
                    gemRect.w = gems[i].w;
                    gemRect.h = gems[i].h;

                    if (collisionAABB(joueurCourant->posScreen, gemRect))
                    {
                        ajouterScoreJoueur(joueurCourant, GEM_SCORE_VALUE);
                        gemDamageBuffTimer = GEM_BUFF_DURATION;
                        gems[i].active = 0;
                    }
                }
            }

            if (!joueurCourant->alive &&
                joueurCourant->vies <= 0 &&
                now - joueurCourant->deathTime >= PLAYER_RESPAWN_DELAY)
            {
                if (!puzzleChallengeUsed && lancerChallengePuzzle())
                {
                    puzzleChallengeUsed = 1;
                    joueurCourant->vies = 1;
                    respawnJoueur(joueurCourant);
                }
                else
                    running = 0;
            }
        }

        updateBullets(bullets, MAX_BULLETS);

        if (phasePNJTerminee(npcs) && npcPhaseIndex < 2)
        {
            Joueur *joueurCourant;
            int phaseAvoidX;
            int phaseAvoidW;

            npcPhaseIndex++;
            npcSwarmAlerted = 0;

            if (!multiplayerMode || joueurActif == 1)
                joueurCourant = &J1;
            else
                joueurCourant = &J2;

            if (sharedView)
            {
                int j1Center = J1.posScreen.x + J1.posScreen.w / 2;
                int j2Center = J2.posScreen.x + J2.posScreen.w / 2;

                phaseAvoidW = SCREEN_W;
                if (multiplayerMode)
                    phaseAvoidX = (j1Center + j2Center) / 2 - phaseAvoidW / 2;
                else
                    phaseAvoidX = joueurCourant->posScreen.x + joueurCourant->posScreen.w / 2 - phaseAvoidW / 2;
            }
            else
            {
                phaseAvoidW = SCREEN_W / 2;
                phaseAvoidX = joueurCourant->posScreen.x + joueurCourant->posScreen.w / 2 - phaseAvoidW / 2;
            }

            if (phaseAvoidX < 0)
                phaseAvoidX = 0;
            if (phaseAvoidX > WORLD_W - phaseAvoidW)
                phaseAvoidX = WORLD_W - phaseAvoidW;

            lancerPhasePNJ(npcs, npcPhaseIndex, phaseAvoidX, phaseAvoidW);
        }

        if (!npcSwarmAlerted)
        {
            Joueur *joueurCourant;

            if (!multiplayerMode || joueurActif == 1)
                joueurCourant = &J1;
            else
                joueurCourant = &J2;

            for (i = 0; i < MAX_NPCS; i++)
            {
                if (npcCanSeePlayer(&npcs[i], joueurCourant))
                {
                    npcSwarmAlerted = 1;
                    break;
                }
            }
        }

        for (i = 0; i < MAX_NPCS; i++)
        {
            int j;
            Joueur *joueurCourant;

            if (!multiplayerMode || joueurActif == 1)
                joueurCourant = &J1;
            else
                joueurCourant = &J2;

            if (sharedView)
            {
                int j1Center = J1.posScreen.x + J1.posScreen.w / 2;
                int j2Center = J2.posScreen.x + J2.posScreen.w / 2;

                npcUpdateViewW = SCREEN_W;
                if (multiplayerMode)
                    npcUpdateCameraX = (j1Center + j2Center) / 2 - npcUpdateViewW / 2;
                else
                    npcUpdateCameraX = joueurCourant->posScreen.x + joueurCourant->posScreen.w / 2 - npcUpdateViewW / 2;
            }
            else
            {
                npcUpdateViewW = SCREEN_W / 2;
                npcUpdateCameraX = joueurCourant->posScreen.x + joueurCourant->posScreen.w / 2 - npcUpdateViewW / 2;
            }

            if (npcUpdateCameraX < 0)
                npcUpdateCameraX = 0;
            if (npcUpdateCameraX > WORLD_W - npcUpdateViewW)
                npcUpdateCameraX = WORLD_W - npcUpdateViewW;

            updateNPC(&npcs[i], joueurCourant, npcSwarmAlerted,
                      npcUpdateCameraX, npcUpdateViewW,
                      stablePlatforms, STABLE_PLATFORM_COUNT,
                      movingPlatforms, MOVING_PLATFORM_COUNT);

            if (isNPCActive(&npcs[i]) &&
                joueurCourant->alive &&
                joueurCourant->visible &&
                now >= npcs[i].nextAttackAt &&
                now >= joueurCourant->invulnerableUntil)
            {
                SDL_Rect enemyAttackRect;
                enemyAttackRect.y = npcs[i].dstRect.y + 18;
                enemyAttackRect.w = npcs[i].dstRect.w + ENEMY_ATTACK_RANGE;
                enemyAttackRect.h = npcs[i].dstRect.h - 36;

                if (npcs[i].direction == 0)
                    enemyAttackRect.x = npcs[i].dstRect.x;
                else
                    enemyAttackRect.x = npcs[i].dstRect.x - ENEMY_ATTACK_RANGE;

                if (collisionAABB(enemyAttackRect, joueurCourant->posScreen))
                {
                    npcs[i].state = ENEMY_ATTACKING;
                    npcs[i].attackStartedAt = now;
                    npcs[i].nextAttackAt = now + ENEMY_ATTACK_COOLDOWN;
                    npcs[i].action = 0;
                    ajouterScoreJoueur(joueurCourant, -ENEMY_DAMAGE_SCORE);
                    appliquerDegatsJoueur(joueurCourant, 1, now);
                }
            }

            for (j = 0; j < MAX_BULLETS; j++)
            {
                if (!bullets[j].active)
                    continue;

                if (isNPCActive(&npcs[i]) &&
                    collisionAABB(bullets[j].rect, npcs[i].dstRect))
                {
                    int hitDamage = gemDamageBuffTimer > 0 ? GEM_DAMAGE_BONUS : 1;

                    bullets[j].active = 0;
                    npcs[i].health -= hitDamage;
                    if (bullets[j].owner == 1)
                        ajouterScoreJoueur(&J1, ENEMY_HIT_SCORE);
                    else if (bullets[j].owner == 2)
                        ajouterScoreJoueur(&J2, ENEMY_HIT_SCORE);

                    if (npcs[i].health <= 0)
                    {
                        npcs[i].health = 0;
                        npcs[i].state = ENEMY_NEUTRALIZED;
                        npcs[i].action = 0;
                        if (bullets[j].owner == 1)
                            ajouterScoreJoueur(&J1, ENEMY_KILL_SCORE);
                        else if (bullets[j].owner == 2)
                            ajouterScoreJoueur(&J2, ENEMY_KILL_SCORE);
                    }
                    else
                        npcs[i].state = ENEMY_INJURED;
                }
            }
        }

        separerPNJEntasses(npcs);
    }

    if (!quitterSansScore)
    {
        finalScore = J1.score;
        if (multiplayerMode)
            finalScore += J2.score;
        init_highscore(&hs, renderer, finalScore);
        running = 1;
        while (running)
        {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            render_highscore(&hs, renderer);
            SDL_RenderPresent(renderer);

            while (SDL_PollEvent(&event))
                handle_highscore_event(&hs, event, &running);

            if (hs.go_to_menu || hs.go_to_puzzle)
                running = 0;
            SDL_Delay(16);
        }
        free_highscore(&hs);
    }

    if (minimapReady)
    {
        SDL_FreeSurface(minimap.thumbnail);
        SDL_FreeSurface(minimap.man);
    }
    if (mapFrame != NULL)
        SDL_FreeSurface(mapFrame);
    SDL_DestroyTexture(background);
    SDL_DestroyTexture(backgroundMid);
    SDL_DestroyTexture(backgroundFore);
    SDL_DestroyTexture(shipGroundTex);
    SDL_DestroyTexture(marsGroundTex);
    SDL_DestroyTexture(shipPlatformTex);
    SDL_DestroyTexture(marsPlatformTex);
    SDL_DestroyTexture(gemTexture);

    for (i = 0; i < MAX_NPCS; i++)
        destroyNPC(&npcs[i]);
    libererJoueur(&J1);
    libererJoueur(&J2);

    if (font != NULL)
        TTF_CloseFont(font);

    shutdownSDL(window, renderer);
    return 0;
}
