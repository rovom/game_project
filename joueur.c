#include "header.h"

static int chargerAnimation(Animation *anim, SDL_Renderer *renderer,
                            const char *path, int rows, int cols)
{
    SDL_Surface *surf;
    SDL_Surface *rgbaSurf;
    int texW;
    int texH;
    int r;
    int c;

    surf = IMG_Load(path);
    if (surf == NULL)
    {
        printf("[ERREUR] IMG_Load(\"%s\") : %s\n", path, IMG_GetError());
        return 0;
    }

    rgbaSurf = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surf);
    if (rgbaSurf == NULL)
    {
        printf("[ERREUR] SDL_ConvertSurfaceFormat(\"%s\") : %s\n", path, SDL_GetError());
        return 0;
    }

    anim->texture = SDL_CreateTextureFromSurface(renderer, rgbaSurf);

    if (anim->texture == NULL)
    {
        SDL_FreeSurface(rgbaSurf);
        printf("[ERREUR] SDL_CreateTextureFromSurface(\"%s\") : %s\n", path, SDL_GetError());
        return 0;
    }

    SDL_QueryTexture(anim->texture, NULL, NULL, &texW, &texH);
    anim->rows = rows;
    anim->cols = cols;
    anim->frameW = texW / cols;
    anim->frameH = texH / rows;
    anim->drawW = 0;
    anim->drawH = PLAYER_H;

    for (r = 0; r < rows && r < 2; r++)
    {
        for (c = 0; c < cols && c < PLUTO_MAX_FRAMES; c++)
        {
            int x0 = (c * texW) / cols;
            int x1 = ((c + 1) * texW) / cols;
            int y0 = (r * texH) / rows;
            int y1 = ((r + 1) * texH) / rows;
            int maxOpaqueY = y0;
            int x;
            int y;
            int found = 0;

            for (y = y0; y < y1; y++)
            {
                Uint32 *row = (Uint32 *)((Uint8 *)rgbaSurf->pixels + y * rgbaSurf->pitch);

                for (x = x0; x < x1; x++)
                {
                    Uint8 red;
                    Uint8 green;
                    Uint8 blue;
                    Uint8 alpha;

                    SDL_GetRGBA(row[x], rgbaSurf->format, &red, &green, &blue, &alpha);

                    if (alpha >= 32)
                    {
                        if (y > maxOpaqueY)
                            maxOpaqueY = y;
                        found = 1;
                    }
                }
            }

            if (found)
            {
                anim->frames[r][c].x = x0;
                anim->frames[r][c].y = y0;
                anim->frames[r][c].w = x1 - x0;
                anim->frames[r][c].h = maxOpaqueY - y0 + 1;
            }
            else
            {
                anim->frames[r][c].x = x0;
                anim->frames[r][c].y = y0;
                anim->frames[r][c].w = x1 - x0;
                anim->frames[r][c].h = y1 - y0;
            }

            if (anim->frames[r][c].h > 0)
            {
                int scaledW = (anim->frames[r][c].w * anim->drawH) / anim->frames[r][c].h;
                if (scaledW > anim->drawW)
                    anim->drawW = scaledW;
            }
        }
    }

    if (anim->drawW <= 0)
        anim->drawW = PLAYER_W;

    SDL_FreeSurface(rgbaSurf);

    return 1;
}

static void libererAnimation(Animation *anim)
{
    if (anim->texture != NULL)
    {
        SDL_DestroyTexture(anim->texture);
        anim->texture = NULL;
    }
}

static Animation *getCurrentAnimation(Joueur *J)
{
    switch (J->state)
    {
        case STATE_JUMP:
            return &J->jump;
        case STATE_FALL:
            return &J->fall;
        case STATE_FIRE:
            return &J->fire;
        case STATE_DIE:
            return &J->die;
        case STATE_IDLE:
            return &J->idle;
        case STATE_WALK:
        default:
            return &J->walk;
    }
}

static void setPlayerState(Joueur *J, PlayerState state)
{
    if (J->state != state)
    {
        J->state = state;
        J->currentFrame = 0;
        J->lastFrameTime = 0;
    }

    switch (state)
    {
        case STATE_IDLE:
            J->frameDelay = 160;
            J->currentFrame = 0;
            break;
        case STATE_WALK:
            J->frameDelay = 95;
            break;
        case STATE_JUMP:
            J->frameDelay = 105;
            break;
        case STATE_FALL:
            J->frameDelay = 105;
            break;
        case STATE_FIRE:
            J->frameDelay = 85;
            break;
        case STATE_DIE:
            J->frameDelay = 170;
            break;
    }
}

int initialiserJoueur(Joueur *J, SDL_Renderer *renderer, int x, int y)
{
    PlayerAssets assets;

    assets.idle = PLUTO_IDLE_PATH;
    assets.walk = PLUTO_WALK_PATH;
    assets.jump = PLUTO_JUMP_PATH;
    assets.fall = PLUTO_FALL_PATH;
    assets.fire = PLUTO_FIRE_PATH;
    assets.die = PLUTO_DIE_PATH;
    assets.portrait = "assets/characters/joueur1/orange/portrait.png";
    assets.characterName = "Joueur 1";
    assets.outfitName = "Combinaison orange";

    return initialiserJoueurAvecAssets(J, renderer, x, y, &assets);
}

int initialiserJoueurAvecAssets(Joueur *J, SDL_Renderer *renderer, int x, int y,
                                const PlayerAssets *assets)
{
    memset(J, 0, sizeof(Joueur));

    if (assets == NULL)
        return 0;

    if (!chargerAnimation(&J->idle, renderer, assets->idle, PLUTO_IDLE_ROWS, PLUTO_IDLE_COLS))
        return 0;

    if (!chargerAnimation(&J->walk, renderer, assets->walk, PLUTO_WALK_ROWS, PLUTO_WALK_COLS))
    {
        libererJoueur(J);
        return 0;
    }

    if (!chargerAnimation(&J->jump, renderer, assets->jump, PLUTO_JUMP_ROWS, PLUTO_JUMP_COLS))
    {
        libererJoueur(J);
        return 0;
    }

    if (!chargerAnimation(&J->fall, renderer, assets->fall, PLUTO_FALL_ROWS, PLUTO_FALL_COLS))
    {
        libererJoueur(J);
        return 0;
    }

    if (!chargerAnimation(&J->fire, renderer, assets->fire, PLUTO_FIRE_ROWS, PLUTO_FIRE_COLS))
    {
        libererJoueur(J);
        return 0;
    }

    if (!chargerAnimation(&J->die, renderer, assets->die, PLUTO_DIE_ROWS, PLUTO_DIE_COLS))
    {
        libererJoueur(J);
        return 0;
    }

    J->posScreen.x = x;
    J->posScreen.y = y;
    J->posScreen.w = PLAYER_W;
    J->posScreen.h = PLAYER_H;
    J->drawOffsetX = 0;
    J->drawOffsetY = 0;

    J->startX = x;
    J->startY = y;
    J->floorY = GROUND_Y;
    J->posYFloat = (float)y;

    J->score = 0;
    J->vies = PLAYER_LIVES;
    J->health = PLAYER_HEALTH_MAX;
    J->alive = 1;
    J->visible = 1;
    J->facing = FACE_RIGHT;
    J->isJumping = 0;
    J->jumpsUsed = 0;
    J->jumpHeld = 0;
    J->velY = 0.0f;
    J->lastShotTime = 0;
    J->invulnerableUntil = 0;
    J->deathTime = 0;

    setPlayerState(J, STATE_IDLE);
    return 1;
}

void libererJoueur(Joueur *J)
{
    libererAnimation(&J->idle);
    libererAnimation(&J->walk);
    libererAnimation(&J->jump);
    libererAnimation(&J->fall);
    libererAnimation(&J->fire);
    libererAnimation(&J->die);
}

void gererEntreeJoueurClavier(Joueur *J, const Uint8 *keys,
                              SDL_Scancode left, SDL_Scancode right,
                              SDL_Scancode jumpKey)
{
    if (!J->alive || !J->visible)
        return;

    J->moveLeft = 0;
    J->moveRight = 0;

    if (keys[left] && !keys[right])
    {
        J->moveLeft = 1;
        J->facing = FACE_LEFT;
    }
    else if (keys[right] && !keys[left])
    {
        J->moveRight = 1;
        J->facing = FACE_RIGHT;
    }

    if (keys[jumpKey] && !J->jumpHeld)
    {
        lancerSaut(J);
        J->jumpHeld = 1;
    }
    else if (!keys[jumpKey])
        J->jumpHeld = 0;
}

void lancerSaut(Joueur *J)
{
    if (!J->alive || J->jumpsUsed >= 2)
        return;

    J->isJumping = 1;
    J->jumpsUsed++;
    J->velY = -18.0f;
    setPlayerState(J, STATE_JUMP);
}

void updateJoueur(Joueur *J, Uint32 now)
{
    Animation *anim;

    if (!J->alive)
        setPlayerState(J, STATE_DIE);

    if (J->alive)
    {
        if (J->moveLeft)
            J->posScreen.x -= PLAYER_SPEED;
        else if (J->moveRight)
            J->posScreen.x += PLAYER_SPEED;

        if (J->posScreen.x < 0)
            J->posScreen.x = 0;

        if (J->posScreen.x + J->posScreen.w > WORLD_W)
            J->posScreen.x = WORLD_W - J->posScreen.w;

        if (J->isJumping)
        {
            J->velY += 0.7f;
            J->posYFloat += J->velY;
            J->posScreen.y = (int)J->posYFloat;

            if (J->posScreen.y >= J->floorY)
            {
                J->posScreen.y = J->floorY;
                J->posYFloat = (float)J->floorY;
                J->velY = 0.0f;
                J->isJumping = 0;
                J->jumpsUsed = 0;

                if (J->state != STATE_FIRE)
                {
                    if (J->moveLeft || J->moveRight)
                        setPlayerState(J, STATE_WALK);
                    else
                        setPlayerState(J, STATE_IDLE);
                }
            }
            else if (J->velY > 0.0f && J->state != STATE_FIRE)
                setPlayerState(J, STATE_FALL);
        }
        else
        {
            J->posScreen.y = J->floorY;
            J->posYFloat = (float)J->floorY;

            if (J->state != STATE_FIRE)
            {
                if (J->moveLeft || J->moveRight)
                    setPlayerState(J, STATE_WALK);
                else
                    setPlayerState(J, STATE_IDLE);
            }
        }
    }

    anim = getCurrentAnimation(J);

    if (J->state == STATE_IDLE)
        J->currentFrame = 0;
    else if (J->lastFrameTime == 0)
        J->lastFrameTime = now;
    else if (now - J->lastFrameTime >= J->frameDelay)
    {
        J->lastFrameTime = now;
        J->currentFrame++;

        if (J->currentFrame >= anim->cols)
        {
            if (J->state == STATE_FIRE)
            {
                J->currentFrame = 0;

                if (J->alive && J->isJumping)
                {
                    if (J->velY > 0.0f)
                        setPlayerState(J, STATE_FALL);
                    else
                        setPlayerState(J, STATE_JUMP);
                }
                else if (J->alive)
                {
                    if (J->moveLeft || J->moveRight)
                        setPlayerState(J, STATE_WALK);
                    else
                        setPlayerState(J, STATE_IDLE);
                }
            }
            else if (J->state == STATE_DIE)
            {
                J->currentFrame = anim->cols - 1;
                if (J->vies > 0 && now - J->deathTime >= PLAYER_RESPAWN_DELAY)
                    respawnJoueur(J);
            }
            else
                J->currentFrame = 0;
        }
    }

    anim = getCurrentAnimation(J);
    if (J->currentFrame >= anim->cols)
        J->currentFrame = 0;
    J->posSprite = anim->frames[J->facing][J->currentFrame];
}

SDL_Rect getJoueurDrawRect(Joueur *J)
{
    Animation *anim;
    SDL_Rect drawRect;

    anim = getCurrentAnimation(J);
    drawRect.w = (anim != NULL) ? anim->drawW : PLAYER_W;
    drawRect.h = (anim != NULL) ? anim->drawH : PLAYER_H;
    drawRect.x = J->posScreen.x + (J->posScreen.w - drawRect.w) / 2 + J->drawOffsetX;
    drawRect.y = J->posScreen.y + J->posScreen.h - drawRect.h + J->drawOffsetY;
    return drawRect;
}

void renderJoueur(SDL_Renderer *renderer, Joueur *J)
{
    Animation *anim;

    if (!J->visible)
        return;

    anim = getCurrentAnimation(J);
    if (anim == NULL || anim->texture == NULL)
        return;

    J->drawRect = getJoueurDrawRect(J);
    SDL_RenderCopy(renderer, anim->texture, &J->posSprite, &J->drawRect);
}

void reinitialiserPositionJoueur(Joueur *J)
{
    J->posScreen.x = J->startX;
    J->posScreen.y = J->startY;
    J->posYFloat = (float)J->startY;
    J->floorY = GROUND_Y;
    J->moveLeft = 0;
    J->moveRight = 0;
    J->isJumping = 0;
    J->jumpsUsed = 0;
    J->jumpHeld = 0;
    J->velY = 0.0f;
    setPlayerState(J, STATE_IDLE);
}

void respawnJoueur(Joueur *J)
{
    reinitialiserPositionJoueur(J);
    J->health = PLAYER_HEALTH_MAX;
    J->alive = 1;
    J->invulnerableUntil = SDL_GetTicks() + PLAYER_DAMAGE_COOLDOWN;
    J->deathTime = 0;
}

void appliquerDegatsJoueur(Joueur *J, int damage, Uint32 now)
{
    if (!J->alive || !J->visible || J->vies <= 0)
        return;

    if (now < J->invulnerableUntil)
        return;

    J->health -= damage;
    J->invulnerableUntil = now + PLAYER_DAMAGE_COOLDOWN;

    if (J->health <= 0)
    {
        J->health = 0;
        J->vies--;
        if (J->vies < 0)
            J->vies = 0;
        J->alive = 0;
        J->moveLeft = 0;
        J->moveRight = 0;
        J->isJumping = 0;
        J->jumpsUsed = 0;
        J->velY = 0.0f;
        J->deathTime = now;
        setPlayerState(J, STATE_DIE);
    }
}

void ajouterScoreJoueur(Joueur *J, int delta)
{
    J->score += delta;

    if (J->score < 0)
        J->score = 0;
}

void initBullets(Bullet bullets[], int size)
{
    int i;

    for (i = 0; i < size; i++)
        bullets[i].active = 0;
}

void effacerBullets(Bullet bullets[], int size)
{
    int i;

    for (i = 0; i < size; i++)
        bullets[i].active = 0;
}

void tirerBullet(Bullet bullets[], int size, Joueur *shooter, int owner, Uint32 now)
{
    int i;

    if (!shooter->alive || !shooter->visible)
        return;

    if (now - shooter->lastShotTime < FIRE_COOLDOWN)
        return;

    shooter->lastShotTime = now;
    setPlayerState(shooter, STATE_FIRE);

    for (i = 0; i < size; i++)
    {
        if (!bullets[i].active)
        {
            bullets[i].active = 1;
            bullets[i].direction = shooter->facing;
            bullets[i].owner = owner;
            bullets[i].rect.w = BULLET_W;
            bullets[i].rect.h = BULLET_H;
            bullets[i].rect.y = shooter->posScreen.y + shooter->posScreen.h / 2;

            if (shooter->facing == FACE_RIGHT)
                bullets[i].rect.x = shooter->posScreen.x + shooter->posScreen.w - 5;
            else
                bullets[i].rect.x = shooter->posScreen.x - BULLET_W + 5;

            return;
        }
    }
}

void updateBullets(Bullet bullets[], int size)
{
    int i;

    for (i = 0; i < size; i++)
    {
        if (!bullets[i].active)
            continue;

        if (bullets[i].direction == FACE_RIGHT)
            bullets[i].rect.x += BULLET_SPEED;
        else
            bullets[i].rect.x -= BULLET_SPEED;

        if (bullets[i].rect.x > WORLD_W || bullets[i].rect.x + bullets[i].rect.w < 0)
            bullets[i].active = 0;
    }
}

void renderBullets(SDL_Renderer *renderer, Bullet bullets[], int size)
{
    int i;
    SDL_Rect flash;

    SDL_SetRenderDrawColor(renderer, 255, 220, 50, 255);

    for (i = 0; i < size; i++)
    {
        if (!bullets[i].active)
            continue;

        SDL_RenderFillRect(renderer, &bullets[i].rect);

        flash = bullets[i].rect;
        flash.w = 4;
        flash.h = 10;
        flash.y -= 2;

        if (bullets[i].direction == FACE_RIGHT)
            flash.x = bullets[i].rect.x + bullets[i].rect.w;
        else
            flash.x = bullets[i].rect.x - 4;

        SDL_RenderFillRect(renderer, &flash);
    }
}
