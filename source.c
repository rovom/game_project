#include "header.h"

#define STELLAR_MUSIC_PATH "stellar wars level 1 and menu .wav"
#define STELLAR_MENU_LOOP_MS 40000
#define STELLAR_GAME_LOOP_START_SEC 110.0

static Mix_Music *stellarMusic = NULL;
static Uint32 stellarMusicSegmentStartedAt = 0;
static int stellarMusicMode = 0;

int collisionAABB(SDL_Rect a, SDL_Rect b)
{
    if (a.x + a.w <= b.x)
        return 0;
    if (a.x >= b.x + b.w)
        return 0;
    if (a.y + a.h <= b.y)
        return 0;
    if (a.y >= b.y + b.h)
        return 0;

    return 1;
}

static void setNPCWalkFrame(NPC *npc)
{
    if (npc->walkTexture != NULL)
    {
        int textureW;
        int textureH;

        SDL_QueryTexture(npc->walkTexture, NULL, NULL, &textureW, &textureH);
        npc->srcRect.w = textureW / NPC_WALK_COLS;
        npc->srcRect.h = textureH / NPC_WALK_ROWS;
    }
    else
    {
        npc->srcRect.w = npc->w;
        npc->srcRect.h = npc->h;
    }

    npc->srcRect.x = 0;
    npc->srcRect.y = NPC_WALK_RIGHT_ROW * npc->srcRect.h;
}

static SDL_Rect getNPCGroundedDrawRect(NPC *npc, int w, int h)
{
    SDL_Rect drawRect;

    drawRect.x = npc->dstRect.x;
    drawRect.y = npc->dstRect.y + npc->dstRect.h - h + NPC_VISUAL_GROUND_OFFSET;
    drawRect.w = w;
    drawRect.h = h;
    return drawRect;
}

static int npcBaseGroundY(NPC *npc)
{
    return npc->groundY + PLAYER_H - npc->h;
}

static int npcPlatformFloorY(NPC *npc, SDL_Rect platform, int currentFloor)
{
    int npcBottom = (int)(npc->y + npc->h);

    if (npc->x + npc->w <= platform.x || npc->x >= platform.x + platform.w)
        return currentFloor;

    if (npcBottom <= platform.y + 18 && platform.y - npc->h < currentFloor)
        return platform.y - npc->h;

    return currentFloor;
}

static int getNPCFloorY(NPC *npc,
                        SDL_Rect stablePlatforms[], int stableCount,
                        SDL_Rect movingPlatforms[], int movingCount)
{
    int floorY = npcBaseGroundY(npc);
    int i;

    for (i = 0; i < stableCount; i++)
        floorY = npcPlatformFloorY(npc, stablePlatforms[i], floorY);

    for (i = 0; i < movingCount; i++)
        floorY = npcPlatformFloorY(npc, movingPlatforms[i], floorY);

    return floorY;
}

static int npcIsInsideView(NPC *npc, int cameraX, int viewW)
{
    int npcLeft = (int)npc->x;
    int npcRight = npcLeft + npc->w;
    int viewRight = cameraX + viewW;

    return npcRight > cameraX && npcLeft < viewRight;
}

static void maybeJumpNPC(NPC *npc, Joueur *player, int swarmAlerted, Uint32 now)
{
    if (!npc->onGround || now < npc->nextJumpAt)
        return;

    if (swarmAlerted && player != NULL && player->posScreen.y + player->posScreen.h < npc->dstRect.y + npc->dstRect.h - 28)
    {
        npc->velY = ENEMY_JUMP_FORCE;
        npc->onGround = 0;
        npc->nextJumpAt = now + ENEMY_JUMP_COOLDOWN;
        return;
    }

    if (!swarmAlerted && (rand() % 100) < ENEMY_RANDOM_JUMP_CHANCE)
    {
        npc->velY = ENEMY_JUMP_FORCE;
        npc->onGround = 0;
        npc->nextJumpAt = now + ENEMY_JUMP_COOLDOWN;
    }
}

int initSDL(SDL_Window **window, SDL_Renderer **renderer)
{
    int imgFlags = IMG_INIT_PNG;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
    {
        printf("SDL init error: %s\n", SDL_GetError());
        return 0;
    }

    if ((IMG_Init(imgFlags) & imgFlags) != imgFlags)
    {
        printf("SDL_image init error: %s\n", IMG_GetError());
        SDL_Quit();
        return 0;
    }

    if (TTF_Init() == -1)
    {
        printf("SDL_ttf init error: %s\n", TTF_GetError());
        IMG_Quit();
        SDL_Quit();
        return 0;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        printf("SDL_mixer init error: %s\n", Mix_GetError());
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 0;
    }

    *window = SDL_CreateWindow("Pluto + NPC",
                               SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED,
                               SCREEN_W,
                               SCREEN_H,
                               SDL_WINDOW_SHOWN);

    if (*window == NULL)
    {
        printf("Window error: %s\n", SDL_GetError());
        Mix_CloseAudio();
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 0;
    }

    *renderer = SDL_CreateRenderer(*window, -1,
                                   SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (*renderer == NULL)
        *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_SOFTWARE);

    if (*renderer == NULL)
    {
        printf("Renderer error: %s\n", SDL_GetError());
        SDL_DestroyWindow(*window);
        Mix_CloseAudio();
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 0;
    }

    return 1;
}

void shutdownSDL(SDL_Window *window, SDL_Renderer *renderer)
{
    stellarMusicStop();

    if (renderer != NULL)
        SDL_DestroyRenderer(renderer);

    if (window != NULL)
        SDL_DestroyWindow(window);

    Mix_CloseAudio();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

static int stellarMusicEnsureLoaded(void)
{
    if (stellarMusic != NULL)
        return 1;

    stellarMusic = Mix_LoadMUS(STELLAR_MUSIC_PATH);
    if (stellarMusic == NULL)
    {
        printf("[ERREUR] Mix_LoadMUS %s : %s\n", STELLAR_MUSIC_PATH, Mix_GetError());
        return 0;
    }

    return 1;
}

static void stellarMusicSeek(double seconds)
{
    if (Mix_SetMusicPosition(seconds) != 0)
        printf("[ERREUR] Mix_SetMusicPosition %.2f : %s\n", seconds, Mix_GetError());
}

void stellarMusicStartMenu(void)
{
    if (!stellarMusicEnsureLoaded())
        return;

    stellarMusicMode = 1;
    if (!Mix_PlayingMusic())
        Mix_PlayMusic(stellarMusic, 0);
    stellarMusicSeek(0.0);
    stellarMusicSegmentStartedAt = SDL_GetTicks();
}

void stellarMusicUpdateMenu(void)
{
    Uint32 now;

    if (stellarMusicMode != 1 || stellarMusic == NULL)
        return;

    now = SDL_GetTicks();
    if (!Mix_PlayingMusic() || now - stellarMusicSegmentStartedAt >= STELLAR_MENU_LOOP_MS)
    {
        Mix_PlayMusic(stellarMusic, 0);
        stellarMusicSeek(0.0);
        stellarMusicSegmentStartedAt = now;
    }
}

void stellarMusicStartGameplay(void)
{
    if (!stellarMusicEnsureLoaded())
        return;

    stellarMusicMode = 2;
    if (!Mix_PlayingMusic())
    {
        Mix_PlayMusic(stellarMusic, 0);
        stellarMusicSeek(STELLAR_GAME_LOOP_START_SEC);
    }
    stellarMusicSegmentStartedAt = SDL_GetTicks();
}

void stellarMusicUpdateGameplay(void)
{
    if (stellarMusicMode != 2 || stellarMusic == NULL)
        return;

    if (!Mix_PlayingMusic())
    {
        Mix_PlayMusic(stellarMusic, 0);
        stellarMusicSeek(STELLAR_GAME_LOOP_START_SEC);
        stellarMusicSegmentStartedAt = SDL_GetTicks();
    }
}

void stellarMusicStop(void)
{
    if (stellarMusic != NULL)
    {
        Mix_HaltMusic();
        Mix_FreeMusic(stellarMusic);
        stellarMusic = NULL;
    }

    stellarMusicMode = 0;
    stellarMusicSegmentStartedAt = 0;
}

void initNPC(NPC *npc, SDL_Renderer *renderer)
{
    SDL_Surface *surface;
    int srcW;
    int srcH;

    npc->walkTexture = NULL;
    npc->slashTexture = NULL;
    npc->dieTexture = NULL;
    npc->useSprite = 0;

    srcW = npc->w;
    srcH = npc->h;

    surface = IMG_Load("walk.png");
    if (surface != NULL)
    {
        npc->walkTexture = SDL_CreateTextureFromSurface(renderer, surface);
        srcW = surface->w / NPC_WALK_COLS;
        srcH = surface->h / NPC_WALK_ROWS;
        npc->useSprite = 1;
        SDL_FreeSurface(surface);
    }

    surface = IMG_Load("slash.png");
    if (surface != NULL)
    {
        npc->slashTexture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }

    surface = IMG_Load("die.png");
    if (surface != NULL)
    {
        npc->dieTexture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }

    if (npc->groundY == 0)
        npc->groundY = GROUND_Y;
    npc->y = npc->groundY + PLAYER_H - npc->h;
    npc->srcRect.x = 0;
    npc->srcRect.y = NPC_WALK_RIGHT_ROW * srcH;
    npc->srcRect.w = srcW;
    npc->srcRect.h = srcH;

    npc->dstRect.x = (int)npc->x;
    npc->dstRect.y = (int)npc->y;
    npc->dstRect.w = npc->w;
    npc->dstRect.h = npc->h;

    npc->direction = 0;
    npc->action = 0;
    npc->health = ENEMY_HEALTH_MAX;
    npc->maxHealth = ENEMY_HEALTH_MAX;
    npc->state = ENEMY_ALIVE;
    npc->lastFrameTime = 0;
    npc->frameDelay = 170;
    npc->attackStartedAt = 0;
    npc->nextAttackAt = 0;
    npc->velY = 0.0f;
    npc->onGround = 1;
    npc->nextJumpAt = SDL_GetTicks() + (Uint32)(rand() % ENEMY_JUMP_COOLDOWN);
}

void destroyNPC(NPC *npc)
{
    if (npc->walkTexture != NULL)
        SDL_DestroyTexture(npc->walkTexture);

    if (npc->slashTexture != NULL)
        SDL_DestroyTexture(npc->slashTexture);

    if (npc->dieTexture != NULL)
        SDL_DestroyTexture(npc->dieTexture);

    npc->walkTexture = NULL;
    npc->slashTexture = NULL;
    npc->dieTexture = NULL;
}

void removeNPCFromPlay(NPC *npc)
{
    npc->state = ENEMY_REMOVED;
    npc->health = 0;
    npc->action = 0;
    npc->velY = 0.0f;
    npc->onGround = 1;
    npc->dstRect.x = 0;
    npc->dstRect.y = 0;
    npc->dstRect.w = 0;
    npc->dstRect.h = 0;
}

void spawnNPCFromSide(NPC *npc, int side)
{
    int edgePadding = 70 + rand() % 90;

    if (side < 0)
    {
        npc->x = (float)(SHIP_END_X + edgePadding);
        npc->direction = 0;
    }
    else
    {
        npc->x = (float)(WORLD_W - npc->w - edgePadding);
        npc->direction = 1;
    }

    npc->groundY = MARS_GROUND_Y;
    npc->y = (float)npcBaseGroundY(npc);
    npc->posMin = (int)npc->x - 180;
    npc->posMax = (int)npc->x + 180;

    if (npc->posMin < SHIP_END_X)
        npc->posMin = SHIP_END_X;
    if (npc->posMax > WORLD_W - npc->w)
        npc->posMax = WORLD_W - npc->w;

    npc->action = 0;
    npc->health = ENEMY_HEALTH_MAX;
    npc->maxHealth = ENEMY_HEALTH_MAX;
    npc->state = ENEMY_ALIVE;
    npc->lastFrameTime = 0;
    npc->attackStartedAt = 0;
    npc->nextAttackAt = 0;
    npc->velY = 0.0f;
    npc->onGround = 1;
    npc->nextJumpAt = SDL_GetTicks() + (Uint32)(rand() % ENEMY_JUMP_COOLDOWN);
    setNPCWalkFrame(npc);

    npc->dstRect.x = (int)npc->x;
    npc->dstRect.y = (int)npc->y;
    npc->dstRect.w = npc->w;
    npc->dstRect.h = npc->h;
}

void spawnNPCInWave(NPC *npc, int index, int total, int avoidX, int avoidW)
{
    int spawnMin = 160;
    int spawnMax = WORLD_W - npc->w - 130;
    int usableWidth;
    int slotWidth;
    int slotCenter;
    int jitterRange;
    int jitter;
    int patrolReach;
    int avoidMin;
    int avoidMax;
    int leftSpace;
    int rightSpace;

    if (total <= 0)
    {
        spawnNPCFromSide(npc, (rand() % 2 == 0) ? -1 : 1);
        return;
    }

    if (index < 0)
        index = 0;
    if (index >= total)
        index = total - 1;

    if (spawnMax <= spawnMin)
    {
        spawnNPCFromSide(npc, (index % 2 == 0) ? -1 : 1);
        return;
    }

    usableWidth = spawnMax - spawnMin;
    slotWidth = usableWidth / total;
    if (slotWidth < 1)
        slotWidth = 1;

    slotCenter = spawnMin + slotWidth * index + slotWidth / 2;
    jitterRange = slotWidth / 5;
    if (jitterRange > 35)
        jitterRange = 35;

    jitter = 0;
    if (jitterRange > 0)
    {
        jitter = rand() % (jitterRange + 1);
        if (index % 2 == 0)
            jitter = -jitter;
    }

    npc->x = (float)(slotCenter + jitter);
    if (npc->x < spawnMin)
        npc->x = (float)spawnMin;
    if (npc->x > spawnMax)
        npc->x = (float)spawnMax;

    if (avoidW > 0)
    {
        avoidMin = avoidX - 260;
        avoidMax = avoidX + avoidW + 260;
        leftSpace = avoidMin - spawnMin;
        rightSpace = spawnMax - avoidMax;

        if ((int)npc->x + npc->w > avoidMin && (int)npc->x < avoidMax)
        {
            if ((index % 2 == 0 && leftSpace > npc->w) || rightSpace <= npc->w)
            {
                int leftCount = total / 2 + total % 2;
                int leftSlot = leftSpace / leftCount;
                int leftIndex = index / 2;

                if (leftSlot < 1)
                    leftSlot = 1;
                npc->x = (float)(spawnMin + leftSlot * leftIndex + leftSlot / 2);
            }
            else if (rightSpace > npc->w)
            {
                int rightCount = total / 2;
                int rightSlot;
                int rightIndex = index / 2;

                if (rightCount < 1)
                    rightCount = 1;
                rightSlot = rightSpace / rightCount;
                if (rightSlot < 1)
                    rightSlot = 1;
                npc->x = (float)(avoidMax + rightSlot * rightIndex + rightSlot / 2);
            }

            if (npc->x < spawnMin)
                npc->x = (float)spawnMin;
            if (npc->x > spawnMax)
                npc->x = (float)spawnMax;
        }
    }

    npc->direction = (index % 2 == 0) ? 0 : 1;
    if ((int)npc->x >= SHIP_END_X)
        npc->groundY = MARS_GROUND_Y;
    else
        npc->groundY = GROUND_Y;
    npc->y = (float)npcBaseGroundY(npc);

    patrolReach = 145 + rand() % 120;
    npc->posMin = (int)npc->x - patrolReach;
    npc->posMax = (int)npc->x + patrolReach;

    if (npc->posMin < 0)
        npc->posMin = 0;
    if (npc->posMax > WORLD_W - npc->w)
        npc->posMax = WORLD_W - npc->w;

    npc->action = 0;
    npc->health = ENEMY_HEALTH_MAX;
    npc->maxHealth = ENEMY_HEALTH_MAX;
    npc->state = ENEMY_ALIVE;
    npc->lastFrameTime = 0;
    npc->attackStartedAt = 0;
    npc->nextAttackAt = 0;
    npc->velY = 0.0f;
    npc->onGround = 1;
    npc->nextJumpAt = SDL_GetTicks() + (Uint32)(rand() % ENEMY_JUMP_COOLDOWN);
    setNPCWalkFrame(npc);

    npc->dstRect.x = (int)npc->x;
    npc->dstRect.y = (int)npc->y;
    npc->dstRect.w = npc->w;
    npc->dstRect.h = npc->h;
}

int npcCanSeePlayer(NPC *npc, Joueur *player)
{
    int npcCenter;
    int playerCenter;
    int distance;

    if (!isNPCActive(npc) || player == NULL || !player->alive || !player->visible)
        return 0;

    npcCenter = npc->dstRect.x + npc->dstRect.w / 2;
    playerCenter = player->posScreen.x + player->posScreen.w / 2;
    distance = playerCenter - npcCenter;

    if (distance < 0)
        distance = -distance;

    if (distance > ENEMY_DETECTION_RANGE)
        return 0;

    if (player->posScreen.y + player->posScreen.h <= npc->dstRect.y + 12 ||
        player->posScreen.y >= npc->dstRect.y + npc->dstRect.h)
        return 0;

    if (npc->direction == 0 && playerCenter < npcCenter)
        return 0;
    if (npc->direction != 0 && playerCenter > npcCenter)
        return 0;

    return 1;
}

void updateNPC(NPC *npc, Joueur *player, int swarmAlerted,
               int cameraX, int viewW,
               SDL_Rect stablePlatforms[], int stableCount,
               SDL_Rect movingPlatforms[], int movingCount)
{
    Uint32 now = SDL_GetTicks();
    int floorY;
    int npcCenter;
    int playerCenter;
    int canChase;

    if (npc->state == ENEMY_REMOVED)
        return;

    if (npc->state == ENEMY_NEUTRALIZED)
    {
        if (npc->dieTexture != NULL)
        {
            int dieW;
            int dieH;
            int dieFrameW;

            SDL_QueryTexture(npc->dieTexture, NULL, NULL, &dieW, &dieH);
            dieFrameW = dieW / NPC_DIE_COLS;

            if (now - npc->lastFrameTime >= npc->frameDelay)
            {
                npc->lastFrameTime = now;
                if (npc->action < NPC_DIE_COLS - 1)
                    npc->action++;
                else
                {
                    removeNPCFromPlay(npc);
                    return;
                }
            }

            npc->srcRect.x = npc->action * dieFrameW;
            npc->srcRect.y = 0;
            npc->srcRect.w = dieFrameW;
            npc->srcRect.h = dieH / NPC_DIE_ROWS;
        }

        npc->dstRect.x = (int)npc->x;
        npc->dstRect.y = (int)npc->y;
        npc->dstRect.w = npc->w;
        npc->dstRect.h = npc->h;
        return;
    }

    if (npc->state == ENEMY_ATTACKING)
    {
        if (npc->slashTexture != NULL)
        {
            int slashW;
            int slashH;
            int slashFrameW;
            int slashFrameH;
            int elapsed;
            int frame;

            SDL_QueryTexture(npc->slashTexture, NULL, NULL, &slashW, &slashH);
            slashFrameW = slashW / NPC_SLASH_COLS;
            slashFrameH = slashH / NPC_SLASH_ROWS;
            elapsed = (int)(now - npc->attackStartedAt);
            frame = (elapsed * NPC_SLASH_COLS) / ENEMY_ATTACK_DURATION;

            if (frame >= NPC_SLASH_COLS)
                frame = NPC_SLASH_COLS - 1;

            npc->srcRect.x = frame * slashFrameW;
            npc->srcRect.y = (npc->direction == 0) ? slashFrameH : 0;
            npc->srcRect.w = slashFrameW;
            npc->srcRect.h = slashFrameH;
        }

        if (now - npc->attackStartedAt >= ENEMY_ATTACK_DURATION)
        {
            if (npc->health < npc->maxHealth)
                npc->state = ENEMY_INJURED;
            else
                npc->state = ENEMY_ALIVE;
            setNPCWalkFrame(npc);
        }

        npc->dstRect.x = (int)npc->x;
        npc->dstRect.y = (int)npc->y;
        npc->dstRect.w = npc->w;
        npc->dstRect.h = npc->h;
        return;
    }

    canChase = swarmAlerted &&
               npcIsInsideView(npc, cameraX, viewW) &&
               player != NULL &&
               player->alive &&
               player->visible;

    if (canChase)
    {
        npcCenter = (int)npc->x + npc->w / 2;
        playerCenter = player->posScreen.x + player->posScreen.w / 2;

        if (playerCenter >= npcCenter)
        {
            npc->direction = 0;
            npc->x += ENEMY_CHASE_SPEED;
        }
        else
        {
            npc->direction = 1;
            npc->x -= ENEMY_CHASE_SPEED;
        }
    }
    else
    {
        if ((int)npc->x > npc->posMax)
            npc->direction = 1;

        if ((int)npc->x < npc->posMin)
            npc->direction = 0;

        if (npc->direction == 0)
            npc->x += ENEMY_PATROL_SPEED;
        else
            npc->x -= ENEMY_PATROL_SPEED;
    }

    if (npc->x < 0)
        npc->x = 0;
    if (npc->x + npc->w > WORLD_W)
        npc->x = WORLD_W - npc->w;

    maybeJumpNPC(npc, player, canChase, now);

    floorY = getNPCFloorY(npc, stablePlatforms, stableCount, movingPlatforms, movingCount);
    npc->velY += ENEMY_GRAVITY;
    npc->y += npc->velY;

    if (npc->y >= floorY)
    {
        npc->y = (float)floorY;
        npc->velY = 0.0f;
        npc->onGround = 1;
    }
    else
        npc->onGround = 0;

    if (npc->useSprite && now - npc->lastFrameTime >= npc->frameDelay)
    {
        npc->lastFrameTime = now;
        if (npc->srcRect.w > npc->w)
            setNPCWalkFrame(npc);

        npc->srcRect.y = NPC_WALK_RIGHT_ROW * npc->srcRect.h;

        if (npc->srcRect.x >= (NPC_WALK_COLS - 1) * npc->srcRect.w)
            npc->srcRect.x = 0;
        else
            npc->srcRect.x += npc->srcRect.w;
    }

    npc->dstRect.x = (int)npc->x;
    npc->dstRect.y = (int)npc->y;
    npc->dstRect.w = npc->w;
    npc->dstRect.h = npc->h;
}

void renderNPC(SDL_Renderer *renderer, NPC *npc)
{
    SDL_Rect healthBarBg;
    SDL_Rect healthBar;

    if (npc->state == ENEMY_REMOVED)
        return;

    if (npc->state == ENEMY_NEUTRALIZED && npc->action >= NPC_DIE_COLS - 1)
    {
        if (npc->dieTexture != NULL)
        {
            SDL_Rect dieDst = getNPCGroundedDrawRect(npc, npc->w, npc->h);

            SDL_RenderCopy(renderer, npc->dieTexture, &npc->srcRect, &dieDst);
        }
        return;
    }

    if (npc->state == ENEMY_NEUTRALIZED && npc->dieTexture != NULL)
    {
        SDL_Rect dieDst = getNPCGroundedDrawRect(npc, npc->w, npc->h);

        SDL_RenderCopy(renderer, npc->dieTexture, &npc->srcRect, &dieDst);
    }
    else if (npc->state == ENEMY_ATTACKING && npc->slashTexture != NULL)
    {
        SDL_Rect slashDst = getNPCGroundedDrawRect(npc, npc->w + ENEMY_ATTACK_RANGE, npc->h);

        if (npc->direction != 0)
            slashDst.x -= ENEMY_ATTACK_RANGE;

        SDL_RenderCopy(renderer, npc->slashTexture, &npc->srcRect, &slashDst);
    }
    else if (npc->useSprite && npc->walkTexture != NULL)
    {
        SDL_Rect walkDst = getNPCGroundedDrawRect(npc, npc->w, npc->h);
        SDL_RendererFlip flip = SDL_FLIP_NONE;

        if (npc->direction != 0)
            flip = SDL_FLIP_HORIZONTAL;

        SDL_RenderCopyEx(renderer, npc->walkTexture, &npc->srcRect, &walkDst, 0, NULL, flip);
    }
    else
    {
        SDL_SetRenderDrawColor(renderer, 140, 140, 140, 255);
        SDL_RenderFillRect(renderer, &npc->dstRect);
    }

    if (npc->state == ENEMY_NEUTRALIZED)
        return;

    healthBarBg.x = npc->dstRect.x;
    healthBarBg.y = npc->dstRect.y - 12;
    healthBarBg.w = npc->dstRect.w;
    healthBarBg.h = 8;
    healthBar = healthBarBg;
    healthBar.w = (npc->dstRect.w * npc->health) / npc->maxHealth;

    SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
    SDL_RenderFillRect(renderer, &healthBarBg);

    if (npc->state == ENEMY_ALIVE)
        SDL_SetRenderDrawColor(renderer, 40, 180, 40, 255);
    else if (npc->state == ENEMY_ATTACKING)
        SDL_SetRenderDrawColor(renderer, 210, 70, 40, 255);
    else
        SDL_SetRenderDrawColor(renderer, 200, 180, 40, 255);

    SDL_RenderFillRect(renderer, &healthBar);
}

int isNPCActive(NPC *npc)
{
    return npc->state != ENEMY_NEUTRALIZED && npc->state != ENEMY_REMOVED;
}
