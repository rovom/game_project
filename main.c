

#include "header.h"
#include "highscore.h"
#include "main_menu.h"
#include "minimap/minimap.h"
#include "puzzle_game/game.h"
#include "save_system.h"

typedef struct
{
    SDL_Texture *scoreTexture;
    int joueurActif;
    int score;
    int w;
    int h;
} HudTextCache;

#define PAUSE_ACTION_RESUME 0
#define PAUSE_ACTION_SAVE 1
#define PAUSE_ACTION_LOAD 2
#define PAUSE_ACTION_MAIN_MENU 3
#define PAUSE_ACTION_QUIT 4

#define SHOTGUN_MUZZLE_FRAME_W 96
#define SHOTGUN_MUZZLE_FRAME_H 64
#define SHOTGUN_MUZZLE_FRAMES 3
#define SHOTGUN_MUZZLE_DURATION 75
#define SHOTGUN_SMOKE_FRAME_W 56
#define SHOTGUN_SMOKE_FRAME_H 56
#define SHOTGUN_SMOKE_FRAMES 5
#define SHOTGUN_SMOKE_DURATION 360
#define SHOTGUN_MAX_MUZZLE_FLASHES 8
#define SHOTGUN_MAX_SMOKE_PUFFS 12
#define SHOTGUN_MAX_SHELLS 16
#define SHOTGUN_SHELL_DURATION 900
#define SHOTGUN_SHAKE_DURATION 145
#define SHOTGUN_SHAKE_INTENSITY 18
#define SHOTGUN_PLAYER_KNOCKBACK 24
#define SHOTGUN_ENEMY_KNOCKBACK 26.0f
#define SHOTGUN_ENEMY_FLASH_MS 130
#define SHOTGUN_HIT_PAUSE_MS 35

typedef struct
{
    int active;
    int x;
    int y;
    int direction;
    Uint32 startedAt;
} MuzzleFlash;

typedef struct
{
    int active;
    int x;
    int y;
    int direction;
    Uint32 startedAt;
} SmokePuff;

typedef struct
{
    int active;
    float startX;
    float startY;
    float vx;
    float vy;
    int direction;
    Uint32 startedAt;
} EjectedShell;

typedef struct
{
    SDL_Texture *muzzleTexture;
    SDL_Texture *smokeTexture;
    SDL_Texture *shellTexture;
    MuzzleFlash flashes[SHOTGUN_MAX_MUZZLE_FLASHES];
    SmokePuff smokes[SHOTGUN_MAX_SMOKE_PUFFS];
    EjectedShell shells[SHOTGUN_MAX_SHELLS];
    Uint32 shakeStartedAt;
    Uint32 shakeUntil;
    int shakeDirection;
} ShotgunFeedback;

static void afficherHUD(SDL_Renderer *renderer, TTF_Font *font,
                        Joueur *J1, Joueur *J2, int joueurActif,
                        HudTextCache *cache, int x, int y)
{
    SDL_Color jaune;
    char txt[128];
    SDL_Surface *s;
    SDL_Rect hudBg;
    SDL_Rect d;
    SDL_Rect lifeRect;
    Joueur *J;
    int score;
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
    hudBg.x = x;
    hudBg.y = y;
    hudBg.w = 360;
    hudBg.h = 55;
    SDL_RenderFillRect(renderer, &hudBg);

    lifeRect.y = y + 13;
    lifeRect.w = 20;
    lifeRect.h = 20;
    for (i = 0; i < PLAYER_LIVES; i++)
    {
        lifeRect.x = x + 12 + i * 28;
        if (i < J->vies)
            SDL_SetRenderDrawColor(renderer, 210, 40, 40, 255);
        else
            SDL_SetRenderDrawColor(renderer, 70, 25, 25, 255);
        SDL_RenderFillRect(renderer, &lifeRect);
    }

    if (font == NULL)
        return;

    score = (joueurActif == 1) ? J1->score : J2->score;
    if (joueurActif == 1)
        snprintf(txt, sizeof(txt), "J1  Score:%d", J1->score);
    else
        snprintf(txt, sizeof(txt), "J2  Score:%d", J2->score);

    if (cache != NULL &&
        (cache->scoreTexture == NULL ||
         cache->joueurActif != joueurActif ||
         cache->score != score))
    {
        if (cache->scoreTexture != NULL)
            SDL_DestroyTexture(cache->scoreTexture);
        cache->scoreTexture = NULL;

        s = TTF_RenderText_Solid(font, txt, jaune);
        if (s != NULL)
        {
            cache->scoreTexture = SDL_CreateTextureFromSurface(renderer, s);
            cache->w = s->w;
            cache->h = s->h;
            cache->joueurActif = joueurActif;
            cache->score = score;
            SDL_FreeSurface(s);
        }
    }

    if (cache != NULL && cache->scoreTexture != NULL)
    {
        if (joueurActif == 2)
            d.x = x + hudBg.w - cache->w - 12;
        else
            d.x = x + 110;
        d.y = y + 10;
        d.w = cache->w;
        d.h = cache->h;
        SDL_RenderCopy(renderer, cache->scoreTexture, NULL, &d);
    }
}

static void drawMenuText(SDL_Renderer *renderer, TTF_Font *font,
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
    dst = (SDL_Rect){x, y, surface->w, surface->h};
    SDL_FreeSurface(surface);

    if (texture != NULL)
    {
        SDL_RenderCopy(renderer, texture, NULL, &dst);
        SDL_DestroyTexture(texture);
    }
}

static int pointDansRect(int x, int y, SDL_Rect rect)
{
    return x >= rect.x && x <= rect.x + rect.w &&
           y >= rect.y && y <= rect.y + rect.h;
}

static void mouseLogical(SDL_Renderer *renderer, int *x, int *y)
{
    int wx;
    int wy;
    float lx;
    float ly;

    SDL_GetMouseState(&wx, &wy);
    SDL_RenderWindowToLogical(renderer, wx, wy, &lx, &ly);
    *x = (int)lx;
    *y = (int)ly;
}

static void eventButtonLogical(SDL_Renderer *renderer, const SDL_Event *event, int *x, int *y)
{
    float lx;
    float ly;

    SDL_RenderWindowToLogical(renderer, event->button.x, event->button.y, &lx, &ly);
    *x = (int)lx;
    *y = (int)ly;
}

static void fade_black(SDL_Renderer *renderer, int logicalW, int logicalH, int fadeOut)
{
    const Uint32 duration = 450;
    Uint32 start = SDL_GetTicks();
    Uint32 elapsed = 0;
    SDL_Rect full = {0, 0, logicalW, logicalH};

    SDL_RenderSetLogicalSize(renderer, logicalW, logicalH);
    while (elapsed < duration)
    {
        Uint8 alpha;
        elapsed = SDL_GetTicks() - start;
        if (elapsed > duration) elapsed = duration;
        alpha = fadeOut ? (Uint8)(255u * elapsed / duration)
                        : (Uint8)(255u - 255u * elapsed / duration);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, alpha);
        SDL_RenderFillRect(renderer, &full);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
}

static int runPuzzleWithFade(SDL_Renderer *renderer)
{
    int result;
    fade_black(renderer, SCREEN_W, SCREEN_H, 1);
    result = run_embedded_puzzle(renderer);
    SDL_RenderSetLogicalSize(renderer, SCREEN_W, SCREEN_H);
    fade_black(renderer, SCREEN_W, SCREEN_H, 0);
    return result;
}

typedef struct
{
    const char *question;
    const char *answers[4];
    int correct;
} QuizQuestion;

static void drawTextCentered(SDL_Renderer *renderer, TTF_Font *font,
                             const char *text, int centerX, int y, SDL_Color color)
{
    SDL_Surface *surface;
    SDL_Texture *texture;
    SDL_Rect dst;

    if (font == NULL || text == NULL)
        return;

    surface = TTF_RenderText_Blended(font, text, color);
    if (surface == NULL)
        return;

    texture = SDL_CreateTextureFromSurface(renderer, surface);
    dst = (SDL_Rect){centerX - surface->w / 2, y, surface->w, surface->h};
    SDL_FreeSurface(surface);
    if (texture != NULL)
    {
        SDL_RenderCopy(renderer, texture, NULL, &dst);
        SDL_DestroyTexture(texture);
    }
}

static int runMenuQuiz(SDL_Renderer *renderer, TTF_Font *font)
{
    static const QuizQuestion questions[7] = {
        {"Which planet is known as the Red Planet?",
         {"Mars", "Venus", "Jupiter", "Mercury"}, 0},
        {"What do astronauts wear in space?",
         {"Spacesuit", "Diving suit", "Raincoat", "Armor"}, 0},
        {"Which object gives light to Earth?",
         {"The Sun", "The Moon", "Mars", "Saturn"}, 0},
        {"What is a galaxy?",
         {"A group of stars", "A single planet", "A rocket fuel", "A comet tail"}, 0},
        {"Which force keeps planets in orbit?",
         {"Gravity", "Electricity", "Wind", "Fire"}, 0},
        {"What do rockets use to leave Earth?",
         {"Thrust", "Wheels", "Wings only", "Magnets"}, 0},
        {"Which planet has famous rings?",
         {"Saturn", "Earth", "Mars", "Venus"}, 0}
    };
    SDL_Event event;
    SDL_Rect answers[4];
    Uint32 start = SDL_GetTicks();
    const Uint32 limit = 5u * 60u * 1000u;
    int current = 0;
    int score = 0;
    int running = 1;
    int finished = 0;
    int mouseX = 0;
    int mouseY = 0;
    int selectedAnswer = -1;
    int selectedCorrect = 0;
    Uint32 feedbackUntil = 0;
    int i;

    SDL_RenderSetLogicalSize(renderer, SCREEN_W, SCREEN_H);
    fade_black(renderer, SCREEN_W, SCREEN_H, 0);

    for (i = 0; i < 4; i++)
        answers[i] = (SDL_Rect){SCREEN_W / 2 - 330, 260 + i * 78, 660, 58};

    while (running)
    {
        Uint32 now = SDL_GetTicks();
        Uint32 remaining = (now - start >= limit) ? 0 : limit - (now - start);
        char timerText[32];
        char progressText[64];

        mouseLogical(renderer, &mouseX, &mouseY);
        if (remaining == 0)
            finished = 1;
        if (selectedAnswer >= 0 && now >= feedbackUntil)
        {
            current++;
            selectedAnswer = -1;
            if (current >= 7)
                finished = 1;
        }

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                return 0;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
                running = 0;
            if (!finished && selectedAnswer < 0 &&
                event.type == SDL_MOUSEBUTTONDOWN &&
                event.button.button == SDL_BUTTON_LEFT)
            {
                int mx;
                int my;
                (void)event;
                mouseLogical(renderer, &mx, &my);
                mouseX = mx;
                mouseY = my;
                for (i = 0; i < 4; i++)
                {
                    if (pointDansRect(mx, my, answers[i]))
                    {
                        selectedAnswer = i;
                        selectedCorrect = (i == questions[current].correct);
                        feedbackUntil = SDL_GetTicks() + 520;
                        if (selectedCorrect)
                            score++;
                        break;
                    }
                }
            }
            if (finished && event.type == SDL_MOUSEBUTTONDOWN &&
                event.button.button == SDL_BUTTON_LEFT)
                running = 0;
        }

        SDL_SetRenderDrawColor(renderer, 5, 8, 18, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 20, 32, 60, 245);
        SDL_Rect panel = {120, 70, SCREEN_W - 240, SCREEN_H - 140};
        SDL_RenderFillRect(renderer, &panel);
        SDL_SetRenderDrawColor(renderer, 70, 150, 220, 220);
        SDL_RenderDrawRect(renderer, &panel);

        snprintf(timerText, sizeof(timerText), "%02u:%02u",
                 (unsigned)(remaining / 60000u),
                 (unsigned)((remaining / 1000u) % 60u));
        snprintf(progressText, sizeof(progressText), "Question %d / 7    Score %d",
                 finished ? 7 : current + 1, score);
        drawTextCentered(renderer, font, "Stellar Quiz", SCREEN_W / 2, 96,
                         (SDL_Color){255, 220, 80, 255});
        drawTextCentered(renderer, font, timerText, SCREEN_W / 2, 142,
                         (remaining < 30000u) ? (SDL_Color){255, 90, 80, 255}
                                               : (SDL_Color){180, 230, 255, 255});
        drawTextCentered(renderer, font, progressText, SCREEN_W / 2, 184,
                         (SDL_Color){200, 210, 235, 255});

        if (!finished)
        {
            drawTextCentered(renderer, font, questions[current].question, SCREEN_W / 2, 220,
                             (SDL_Color){255, 255, 255, 255});
            for (i = 0; i < 4; i++)
            {
                int hover = pointDansRect(mouseX, mouseY, answers[i]);
                if (selectedAnswer == i)
                {
                    int flashOn = ((int)(SDL_GetTicks() / 90u) % 2) == 0;
                    if (selectedCorrect)
                        SDL_SetRenderDrawColor(renderer, flashOn ? 35 : 20,
                                               flashOn ? 190 : 115,
                                               flashOn ? 95 : 60, 255);
                    else
                        SDL_SetRenderDrawColor(renderer, flashOn ? 220 : 130,
                                               flashOn ? 55 : 28,
                                               flashOn ? 55 : 35, 255);
                }
                else
                {
                    SDL_SetRenderDrawColor(renderer, hover && selectedAnswer < 0 ? 58 : 28,
                                           hover && selectedAnswer < 0 ? 95 : 54,
                                           hover && selectedAnswer < 0 ? 150 : 92, 245);
                }
                SDL_RenderFillRect(renderer, &answers[i]);
                if (selectedAnswer == i)
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                else
                    SDL_SetRenderDrawColor(renderer, hover && selectedAnswer < 0 ? 255 : 120,
                                           hover && selectedAnswer < 0 ? 240 : 165,
                                           hover && selectedAnswer < 0 ? 130 : 210, 255);
                SDL_RenderDrawRect(renderer, &answers[i]);
                drawMenuText(renderer, font, questions[current].answers[i],
                             answers[i].x + 28, answers[i].y + 15,
                             (SDL_Color){255, 255, 255, 255});
            }
        }
        else
        {
            char resultText[64];
            snprintf(resultText, sizeof(resultText), "Final Score: %d / 7", score);
            drawTextCentered(renderer, font, remaining == 0 ? "Time is up" : "Quiz complete",
                             SCREEN_W / 2, 300, (SDL_Color){255, 220, 80, 255});
            drawTextCentered(renderer, font, resultText, SCREEN_W / 2, 355,
                             (SDL_Color){255, 255, 255, 255});
            drawTextCentered(renderer, font, "Click anywhere to return",
                             SCREEN_W / 2, 430, (SDL_Color){170, 205, 245, 255});
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    fade_black(renderer, SCREEN_W, SCREEN_H, 1);
    return score;
}

static int afficherMenuPause(SDL_Renderer *renderer, TTF_Font *font,
                             int *debugCollision, int *musicMuted)
{
    SDL_Event event;
    SDL_Rect panel = {SCREEN_W / 2 - 260, 95, 520, 530};
    SDL_Rect buttons[5];
    const char *labels[5] = {"Resume", "Save Game", "Load Game", "Settings", "Return to Main Menu"};
    int mouseX;
    int mouseY;
    int i;
    int settings = 0;

    for (i = 0; i < 5; i++)
        buttons[i] = (SDL_Rect){panel.x + 70, panel.y + 105 + i * 74, panel.w - 140, 54};

    while (1)
    {
        mouseLogical(renderer, &mouseX, &mouseY);
        SDL_RenderSetViewport(renderer, NULL);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 185);
        SDL_RenderFillRect(renderer, NULL);

        SDL_SetRenderDrawColor(renderer, 18, 22, 34, 245);
        SDL_RenderFillRect(renderer, &panel);
        SDL_SetRenderDrawColor(renderer, 220, 220, 240, 255);
        SDL_RenderDrawRect(renderer, &panel);

        if (!settings)
        {
            drawMenuText(renderer, font, "Paused", panel.x + 205, panel.y + 32,
                         (SDL_Color){255, 220, 80, 255});
            for (i = 0; i < 5; i++)
            {
                int hover = pointDansRect(mouseX, mouseY, buttons[i]);
                SDL_SetRenderDrawColor(renderer, hover ? 78 : 38, hover ? 96 : 48,
                                       hover ? 135 : 78, 235);
                SDL_RenderFillRect(renderer, &buttons[i]);
                SDL_SetRenderDrawColor(renderer, 235, 235, 245, 255);
                SDL_RenderDrawRect(renderer, &buttons[i]);
                drawMenuText(renderer, font, labels[i], buttons[i].x + 28, buttons[i].y + 13,
                             (SDL_Color){255, 255, 255, 255});
            }
        }
        else
        {
            drawMenuText(renderer, font, "Settings", panel.x + 190, panel.y + 32,
                         (SDL_Color){255, 220, 80, 255});
            drawMenuText(renderer, font,
                         *debugCollision ? "F3 / D: Debug Collision  ON" : "F3 / D: Debug Collision  OFF",
                         panel.x + 70, panel.y + 145,
                         (SDL_Color){255, 255, 255, 255});
            drawMenuText(renderer, font,
                         *musicMuted ? "M: Music  MUTED" : "M: Music  ON",
                         panel.x + 70, panel.y + 205,
                         (SDL_Color){255, 255, 255, 255});
            drawMenuText(renderer, font, "Esc / Backspace: Back", panel.x + 70, panel.y + 330,
                         (SDL_Color){180, 190, 210, 255});
        }

        SDL_RenderPresent(renderer);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                return PAUSE_ACTION_QUIT;

            if (event.type == SDL_KEYDOWN)
            {
                if (!settings)
                {
                    if (event.key.keysym.sym == SDLK_ESCAPE ||
                        event.key.keysym.sym == SDLK_p)
                        return PAUSE_ACTION_RESUME;
                    if (event.key.keysym.sym == SDLK_s)
                        return PAUSE_ACTION_SAVE;
                    if (event.key.keysym.sym == SDLK_l)
                        return PAUSE_ACTION_LOAD;
                    if (event.key.keysym.sym == SDLK_m)
                        return PAUSE_ACTION_MAIN_MENU;
                }
                else
                {
                    if (event.key.keysym.sym == SDLK_ESCAPE ||
                        event.key.keysym.sym == SDLK_BACKSPACE)
                        settings = 0;
                    if (event.key.keysym.sym == SDLK_d ||
                        event.key.keysym.sym == SDLK_F3)
                        *debugCollision = !*debugCollision;
                    if (event.key.keysym.sym == SDLK_m)
                    {
                        *musicMuted = !*musicMuted;
                        Mix_VolumeMusic(*musicMuted ? 0 : MIX_MAX_VOLUME);
                    }
                }
            }

            if (!settings &&
                event.type == SDL_MOUSEBUTTONDOWN &&
                event.button.button == SDL_BUTTON_LEFT)
            {
                int mx;
                int my;
                eventButtonLogical(renderer, &event, &mx, &my);
                for (i = 0; i < 5; i++)
                {
                    if (!pointDansRect(mx, my, buttons[i]))
                        continue;

                    if (i == 0)
                        return PAUSE_ACTION_RESUME;
                    if (i == 1)
                        return PAUSE_ACTION_SAVE;
                    if (i == 2)
                        return PAUSE_ACTION_LOAD;
                    if (i == 3)
                        settings = 1;
                    if (i == 4)
                        return PAUSE_ACTION_MAIN_MENU;
                }
            }
        }

        SDL_Delay(16);
    }
}

static int lancerChallengePuzzle(SDL_Renderer *renderer)
{
    int round;
    int wins = 0;

    for (round = 0; round < PUZZLE_CHALLENGE_ROUNDS; round++)
        if (runPuzzleWithFade(renderer))
            wins++;

    return wins >= PUZZLE_CHALLENGE_REQUIRED;
}


static void verifierDefaiteJoueur(Joueur *J, Uint32 now,
                                  int *puzzleChallengeUsed, int *running,
                                  SDL_Renderer *renderer)
{
    if (J->alive || J->vies > 0 ||
        now - J->deathTime < PLAYER_RESPAWN_DELAY)
        return;

    if (!*puzzleChallengeUsed && lancerChallengePuzzle(renderer))
    {
        *puzzleChallengeUsed = 1;
        J->vies = 1;
        respawnJoueur(J);
    }
    else
        *running = 0;
}


static int sauvegarderProgression(Joueur *J1, Joueur *J2,
                                  Bullet bullets[], NPC npcs[],
                                  SecondaryEntity gems[], int gemCount,
                                  int gemDamageBuffTimers[],
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
                           gems, gemCount,
                           gemDamageBuffTimers,
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


static int centreJoueurSurPlateforme(Joueur *J, SDL_Rect platform)
{
    int centerX;

    if (!J->alive || !J->visible)
        return 0;

    centerX = J->posScreen.x + J->posScreen.w / 2;
    return centerX >= platform.x && centerX <= platform.x + platform.w;
}


static int joueurPorteParPlateforme(Joueur *J, SDL_Rect platform)
{
    int feetY;

    if (!centreJoueurSurPlateforme(J, platform))
        return 0;

    feetY = J->posScreen.y + J->posScreen.h;
    return !J->isJumping && feetY >= platform.y - 2 && feetY <= platform.y + 6;
}


static void deplacerJoueurAvecPlateforme(Joueur *J, int dx)
{
    if (!J->alive || !J->visible || dx == 0)
        return;

    J->posScreen.x += dx;
    if (J->posScreen.x < 0)
        J->posScreen.x = 0;
    if (J->posScreen.x + J->posScreen.w > WORLD_W)
        J->posScreen.x = WORLD_W - J->posScreen.w;
}


static int clampCameraX(int x, int viewW)
{
    if (x < 0)
        return 0;
    if (x > WORLD_W - viewW)
        return WORLD_W - viewW;
    return x;
}


static void initShotgunFeedback(ShotgunFeedback *feedback, SDL_Renderer *renderer)
{
    memset(feedback, 0, sizeof(*feedback));
    feedback->muzzleTexture = IMG_LoadTexture(renderer, "assets/vfx/shotgun_muzzle_flash.png");
    feedback->smokeTexture = IMG_LoadTexture(renderer, "assets/vfx/shotgun_smoke.png");
    feedback->shellTexture = IMG_LoadTexture(renderer, "assets/vfx/shotgun_shell.png");

    if (feedback->muzzleTexture != NULL)
        SDL_SetTextureBlendMode(feedback->muzzleTexture, SDL_BLENDMODE_BLEND);
    if (feedback->smokeTexture != NULL)
        SDL_SetTextureBlendMode(feedback->smokeTexture, SDL_BLENDMODE_BLEND);
    if (feedback->shellTexture != NULL)
        SDL_SetTextureBlendMode(feedback->shellTexture, SDL_BLENDMODE_BLEND);
}


static void destroyShotgunFeedback(ShotgunFeedback *feedback)
{
    SDL_DestroyTexture(feedback->muzzleTexture);
    SDL_DestroyTexture(feedback->smokeTexture);
    SDL_DestroyTexture(feedback->shellTexture);
}


static void clearShotgunFeedbackRuntime(ShotgunFeedback *feedback)
{
    SDL_Texture *muzzleTexture = feedback->muzzleTexture;
    SDL_Texture *smokeTexture = feedback->smokeTexture;
    SDL_Texture *shellTexture = feedback->shellTexture;

    memset(feedback, 0, sizeof(*feedback));
    feedback->muzzleTexture = muzzleTexture;
    feedback->smokeTexture = smokeTexture;
    feedback->shellTexture = shellTexture;
}


static void getShotgunBarrel(Joueur *J, int *x, int *y)
{
    *y = J->posScreen.y + 55;
    if (J->facing == FACE_RIGHT)
        *x = J->posScreen.x + J->posScreen.w / 2 + 52;
    else
        *x = J->posScreen.x + J->posScreen.w / 2 - 52;
}


static void spawnShotgunFeedback(ShotgunFeedback *feedback, Joueur *shooter, Uint32 now)
{
    int i;
    int barrelX;
    int barrelY;

    getShotgunBarrel(shooter, &barrelX, &barrelY);

    for (i = 0; i < SHOTGUN_MAX_MUZZLE_FLASHES; i++)
    {
        if (!feedback->flashes[i].active)
        {
            feedback->flashes[i].active = 1;
            feedback->flashes[i].x = barrelX;
            feedback->flashes[i].y = barrelY;
            feedback->flashes[i].direction = shooter->facing;
            feedback->flashes[i].startedAt = now;
            break;
        }
    }

    for (i = 0; i < SHOTGUN_MAX_SMOKE_PUFFS; i++)
    {
        if (!feedback->smokes[i].active)
        {
            feedback->smokes[i].active = 1;
            feedback->smokes[i].x = barrelX;
            feedback->smokes[i].y = barrelY;
            feedback->smokes[i].direction = shooter->facing;
            feedback->smokes[i].startedAt = now;
            break;
        }
    }

    for (i = 0; i < SHOTGUN_MAX_SHELLS; i++)
    {
        if (!feedback->shells[i].active)
        {
            int shotRight = shooter->facing == FACE_RIGHT;

            feedback->shells[i].active = 1;
            feedback->shells[i].startX = (float)(shooter->posScreen.x + shooter->posScreen.w / 2 + (shotRight ? -10 : 10));
            feedback->shells[i].startY = (float)(shooter->posScreen.y + 43);
            feedback->shells[i].vx = shotRight ? -2.8f : 2.8f;
            feedback->shells[i].vy = -5.2f;
            feedback->shells[i].direction = shooter->facing;
            feedback->shells[i].startedAt = now;
            break;
        }
    }

    feedback->shakeStartedAt = now;
    feedback->shakeUntil = now + SHOTGUN_SHAKE_DURATION;
    feedback->shakeDirection = (shooter->facing == FACE_RIGHT) ? 1 : -1;
}


static int shotgunShakeOffset(ShotgunFeedback *feedback, Uint32 now)
{
    Uint32 elapsed;
    int wave;
    int strength;

    if (now >= feedback->shakeUntil)
        return 0;

    elapsed = now - feedback->shakeStartedAt;
    strength = SHOTGUN_SHAKE_INTENSITY -
               (int)((elapsed * SHOTGUN_SHAKE_INTENSITY) / SHOTGUN_SHAKE_DURATION);
    if (strength < 0)
        strength = 0;

    wave = ((elapsed / 16) % 2 == 0) ? 1 : -1;
    return feedback->shakeDirection * strength + wave * (strength / 2);
}


static void applyShotgunPlayerKnockback(Joueur *shooter)
{
    if (shooter->facing == FACE_RIGHT)
        shooter->posScreen.x -= SHOTGUN_PLAYER_KNOCKBACK;
    else
        shooter->posScreen.x += SHOTGUN_PLAYER_KNOCKBACK;

    if (shooter->posScreen.x < 0)
        shooter->posScreen.x = 0;
    if (shooter->posScreen.x + shooter->posScreen.w > WORLD_W)
        shooter->posScreen.x = WORLD_W - shooter->posScreen.w;
}


static int tirerShotgunAvecFeedback(Bullet bullets[], int size, Joueur *shooter,
                                    int owner, Uint32 now, int shellMask,
                                    ShotgunFeedback *feedback)
{
    int shellsBefore = shooter->shotgunShells;
    Uint32 lastShotBefore = shooter->lastShotTime;

    tirerBullet(bullets, size, shooter, owner, now,
                shooter->posScreen.x + (shooter->facing == FACE_RIGHT ? 320 : -320),
                shooter->posScreen.y + shooter->posScreen.h / 2,
                shellMask);

    if (shooter->lastShotTime == now &&
        (shooter->shotgunShells != shellsBefore || shooter->lastShotTime != lastShotBefore))
    {
        spawnShotgunFeedback(feedback, shooter, now);
        applyShotgunPlayerKnockback(shooter);
        return 1;
    }

    return 0;
}


static Joueur *selectionnerCibleNPC(NPC *npc, Joueur *J1, Joueur *J2,
                                    int multiplayerMode, Joueur *fallback)
{
    int npcCenter;
    int j1Distance;
    int j2Distance;
    int j1Ready;
    int j2Ready;

    if (npc == NULL)
        return fallback;

    if (!multiplayerMode)
        return J1;

    j1Ready = J1 != NULL && J1->alive && J1->visible;
    j2Ready = J2 != NULL && J2->alive && J2->visible;

    if (j1Ready && !j2Ready)
        return J1;
    if (j2Ready && !j1Ready)
        return J2;
    if (!j1Ready && !j2Ready)
        return fallback;

    npcCenter = npc->dstRect.x + npc->dstRect.w / 2;
    j1Distance = J1->posScreen.x + J1->posScreen.w / 2 - npcCenter;
    j2Distance = J2->posScreen.x + J2->posScreen.w / 2 - npcCenter;

    if (j1Distance < 0)
        j1Distance = -j1Distance;
    if (j2Distance < 0)
        j2Distance = -j2Distance;

    return (j2Distance < j1Distance) ? J2 : J1;
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

typedef struct
{
    int x;
    int patrolReach;
    int direction;
    NPCType npcType;
} FixedNPCSpawn;

static void lancerPhasePNJ(NPC npcs[], int phaseIndex, int avoidX, int avoidW)
{
    static const FixedNPCSpawn phaseSpawns[] = {
        {820, 170, 0, NPC_TYPE_BASIC}, {1480, 180, 1, NPC_TYPE_BASIC},
        {2180, 170, 0, NPC_TYPE_RUSHER}, {2860, 190, 1, NPC_TYPE_BASIC},
        {3540, 160, 0, NPC_TYPE_BASIC}, {4260, 180, 1, NPC_TYPE_RUSHER},
        {4860, 145, 1, NPC_TYPE_BASIC}, {3240, 145, 0, NPC_TYPE_BASIC},
        {3880, 135, 1, NPC_TYPE_RUSHER}, {4540, 130, 0, NPC_TYPE_BASIC},
        {3680, 125, 1, NPC_TYPE_BASIC}, {4100, 120, 0, NPC_TYPE_RUSHER},
        {4480, 115, 1, NPC_TYPE_BASIC}, {4780, 105, 0, NPC_TYPE_RUSHER},
        {5000, 95, 1, NPC_TYPE_BASIC}};
    static const int phaseCounts[] = {7, 10, 15};
    int phaseTotal = (int)(sizeof(phaseCounts) / sizeof(phaseCounts[0]));
    int spawnCount;
    int i;

    (void)avoidX;
    (void)avoidW;

    if (phaseIndex < 0 || phaseIndex >= phaseTotal)
        return;

    spawnCount = phaseCounts[phaseIndex];
    if (spawnCount > MAX_NPCS)
        spawnCount = MAX_NPCS;

    for (i = 0; i < MAX_NPCS; i++)
        removeNPCFromPlay(&npcs[i]);

    for (i = 0; i < spawnCount; i++)
    {
        if (phaseSpawns[i].npcType == NPC_TYPE_RUSHER)
            spawnRusherAt(&npcs[i],
                          phaseSpawns[i].x,
                          phaseSpawns[i].patrolReach,
                          phaseSpawns[i].direction);
        else
            spawnNPCAt(&npcs[i],
                       phaseSpawns[i].x,
                       phaseSpawns[i].patrolReach,
                       phaseSpawns[i].direction);
    }
}

typedef struct
{
    int x;
    int y;
} FixedGemSpawn;

static int initSecondaryEntities(SecondaryEntity entities[], int count)
{
    static const FixedGemSpawn gemSpawns[] = {
        {1048, 230}, {1650, 175}, {2310, 260}, {3430, 185}, {4030, 245}};
    int spawnCount = (int)(sizeof(gemSpawns) / sizeof(gemSpawns[0]));
    int i;

    for (i = 0; i < count; i++)
        entities[i].active = 0;

    if (spawnCount > count)
        spawnCount = count;

    for (i = 0; i < spawnCount; i++)
    {
        entities[i].w = GEM_W;
        entities[i].h = GEM_H;
        entities[i].active = 1;
        entities[i].x = (float)gemSpawns[i].x;
        entities[i].y = (float)gemSpawns[i].y;
    }

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


static void renderShotgunFeedback(SDL_Renderer *renderer, ShotgunFeedback *feedback,
                                  int cameraX, Uint32 now)
{
    int i;

    for (i = 0; i < SHOTGUN_MAX_SMOKE_PUFFS; i++)
    {
        Uint32 elapsed;
        int frame;
        SDL_Rect src;
        SDL_Rect dst;
        SDL_RendererFlip flip = SDL_FLIP_NONE;

        if (!feedback->smokes[i].active)
            continue;

        elapsed = now - feedback->smokes[i].startedAt;
        if (elapsed >= SHOTGUN_SMOKE_DURATION)
        {
            feedback->smokes[i].active = 0;
            continue;
        }

        frame = (int)((elapsed * SHOTGUN_SMOKE_FRAMES) / SHOTGUN_SMOKE_DURATION);
        if (frame >= SHOTGUN_SMOKE_FRAMES)
            frame = SHOTGUN_SMOKE_FRAMES - 1;

        src = (SDL_Rect){frame * SHOTGUN_SMOKE_FRAME_W, 0,
                         SHOTGUN_SMOKE_FRAME_W, SHOTGUN_SMOKE_FRAME_H};
        dst.w = SHOTGUN_SMOKE_FRAME_W;
        dst.h = SHOTGUN_SMOKE_FRAME_H;
        dst.y = feedback->smokes[i].y - dst.h / 2 - (int)(elapsed / 45);
        if (feedback->smokes[i].direction == FACE_RIGHT)
            dst.x = feedback->smokes[i].x - cameraX + (int)(elapsed / 28) - 8;
        else
        {
            dst.x = feedback->smokes[i].x - cameraX - dst.w - (int)(elapsed / 28) + 8;
            flip = SDL_FLIP_HORIZONTAL;
        }

        if (feedback->smokeTexture != NULL)
            SDL_RenderCopyEx(renderer, feedback->smokeTexture, &src, &dst, 0, NULL, flip);
        else
        {
            SDL_SetRenderDrawColor(renderer, 150, 150, 150, 120);
            SDL_RenderFillRect(renderer, &dst);
        }
    }

    for (i = 0; i < SHOTGUN_MAX_MUZZLE_FLASHES; i++)
    {
        Uint32 elapsed;
        int frame;
        SDL_Rect src;
        SDL_Rect dst;
        SDL_RendererFlip flip = SDL_FLIP_NONE;

        if (!feedback->flashes[i].active)
            continue;

        elapsed = now - feedback->flashes[i].startedAt;
        if (elapsed >= SHOTGUN_MUZZLE_DURATION)
        {
            feedback->flashes[i].active = 0;
            continue;
        }

        frame = (int)((elapsed * SHOTGUN_MUZZLE_FRAMES) / SHOTGUN_MUZZLE_DURATION);
        if (frame >= SHOTGUN_MUZZLE_FRAMES)
            frame = SHOTGUN_MUZZLE_FRAMES - 1;

        src = (SDL_Rect){frame * SHOTGUN_MUZZLE_FRAME_W, 0,
                         SHOTGUN_MUZZLE_FRAME_W, SHOTGUN_MUZZLE_FRAME_H};
        dst.w = SHOTGUN_MUZZLE_FRAME_W;
        dst.h = SHOTGUN_MUZZLE_FRAME_H;
        dst.y = feedback->flashes[i].y - dst.h / 2;
        if (feedback->flashes[i].direction == FACE_RIGHT)
            dst.x = feedback->flashes[i].x - cameraX - 4;
        else
        {
            dst.x = feedback->flashes[i].x - cameraX - dst.w + 4;
            flip = SDL_FLIP_HORIZONTAL;
        }

        if (feedback->muzzleTexture != NULL)
            SDL_RenderCopyEx(renderer, feedback->muzzleTexture, &src, &dst, 0, NULL, flip);
        else
        {
            SDL_SetRenderDrawColor(renderer, 255, 210, 40, 220);
            SDL_RenderFillRect(renderer, &dst);
        }
    }

    for (i = 0; i < SHOTGUN_MAX_SHELLS; i++)
    {
        Uint32 elapsed;
        float t;
        SDL_Rect dst;
        double angle;
        SDL_RendererFlip flip = SDL_FLIP_NONE;

        if (!feedback->shells[i].active)
            continue;

        elapsed = now - feedback->shells[i].startedAt;
        if (elapsed >= SHOTGUN_SHELL_DURATION)
        {
            feedback->shells[i].active = 0;
            continue;
        }

        t = (float)elapsed / 16.0f;
        dst.w = 24;
        dst.h = 12;
        dst.x = (int)(feedback->shells[i].startX + feedback->shells[i].vx * t) - cameraX;
        dst.y = (int)(feedback->shells[i].startY + feedback->shells[i].vy * t + 0.20f * t * t);
        angle = (feedback->shells[i].direction == FACE_RIGHT) ? -(double)elapsed * 0.75 : (double)elapsed * 0.75;

        if (feedback->shells[i].direction == FACE_LEFT)
            flip = SDL_FLIP_HORIZONTAL;

        if (feedback->shellTexture != NULL)
            SDL_RenderCopyEx(renderer, feedback->shellTexture, NULL, &dst, angle, NULL, flip);
        else
        {
            SDL_SetRenderDrawColor(renderer, 190, 80, 45, 255);
            SDL_RenderFillRect(renderer, &dst);
        }
    }
}


static void dessinerJoueurSurMinimap(Minimap *minimap, SDL_Surface *surface,
                                      Joueur *J, Uint32 color)
{
    SDL_Rect dot;

    if (minimap == NULL || surface == NULL || J == NULL || !J->alive || !J->visible)
        return;

    dot.w = PLAYER_DOT_SIZE;
    dot.h = PLAYER_DOT_SIZE;
    dot.x = minimap->pos.x + (int)(J->posScreen.x * minimap->scaleX);
    dot.y = minimap->pos.y + (int)(J->posScreen.y * minimap->scaleY);

    if (dot.x < minimap->pos.x)
        dot.x = minimap->pos.x;
    if (dot.y < minimap->pos.y)
        dot.y = minimap->pos.y;
    if (dot.x > minimap->pos.x + minimap->pos.w - dot.w)
        dot.x = minimap->pos.x + minimap->pos.w - dot.w;
    if (dot.y > minimap->pos.y + minimap->pos.h - dot.h)
        dot.y = minimap->pos.y + minimap->pos.h - dot.h;

    SDL_FillRect(surface, &dot, color);
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


static void appliquerFeedbackImpactPNJ(NPC *npc, float *knockbackVel)
{
    if (!isNPCActive(npc))
    {
        *knockbackVel = 0.0f;
        return;
    }

    if (*knockbackVel > -0.2f && *knockbackVel < 0.2f)
    {
        *knockbackVel = 0.0f;
        return;
    }

    npc->x += *knockbackVel;
    if (npc->x < 0.0f)
        npc->x = 0.0f;
    if (npc->x + npc->w > WORLD_W)
        npc->x = (float)(WORLD_W - npc->w);

    npc->dstRect.x = (int)npc->x;
    if (npc->npcType == NPC_TYPE_RUSHER)
        *knockbackVel *= RUSHER_KNOCKBACK_RESISTANCE;
    else
        *knockbackVel *= 0.68f;
}

static int frameAttaquePNJ(NPC *npc, Uint32 now)
{
    int duration = (npc->npcType == NPC_TYPE_RUSHER) ? RUSHER_ATTACK_DURATION : ENEMY_ATTACK_DURATION;
    int frames = (npc->npcType == NPC_TYPE_RUSHER) ? RUSHER_ATTACK_FRAMES : NPC_SLASH_COLS;
    int frame;

    if (duration <= 0 || frames <= 0)
        return 0;

    frame = ((int)(now - npc->attackStartedAt) * frames) / duration;
    if (frame < 0)
        frame = 0;
    if (frame >= frames)
        frame = frames - 1;
    return frame;
}

static int attaquePNJActive(NPC *npc, Uint32 now)
{
    int frame;

    if (npc->npcType != NPC_TYPE_RUSHER)
        return 1;

    frame = frameAttaquePNJ(npc, now);
    return frame >= RUSHER_ATTACK_ACTIVE_START_FRAME &&
           frame <= RUSHER_ATTACK_ACTIVE_END_FRAME &&
           !npc->attackDamageApplied;
}

static SDL_Rect hitboxAttaquePNJ(NPC *npc)
{
    SDL_Rect attackRect;

    if (npc->npcType == NPC_TYPE_RUSHER)
    {
        attackRect.w = RUSHER_ATTACK_HITBOX_W;
        attackRect.h = RUSHER_ATTACK_HITBOX_H;
        attackRect.y = npc->dstRect.y + npc->dstRect.h / 2 - attackRect.h / 2;
        if (npc->direction == 0)
            attackRect.x = npc->dstRect.x + npc->dstRect.w - 6;
        else
            attackRect.x = npc->dstRect.x - attackRect.w + 6;
        return attackRect;
    }

    attackRect.y = npc->dstRect.y + 18;
    attackRect.w = npc->dstRect.w + ENEMY_ATTACK_RANGE;
    attackRect.h = npc->dstRect.h - 36;
    if (npc->direction == 0)
        attackRect.x = npc->dstRect.x;
    else
        attackRect.x = npc->dstRect.x - ENEMY_ATTACK_RANGE;
    return attackRect;
}


static void renderFlashImpactPNJ(SDL_Renderer *renderer, NPC *npc, Uint32 now,
                                 Uint32 flashUntil)
{
    SDL_Rect flash;
    Uint8 alpha;

    if (now >= flashUntil || npc->state == ENEMY_REMOVED)
        return;

    alpha = (Uint8)(80 + ((flashUntil - now) * 150) / SHOTGUN_ENEMY_FLASH_MS);
    flash = npc->dstRect;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
    SDL_SetRenderDrawColor(renderer, 255, 245, 210, alpha);
    SDL_RenderFillRect(renderer, &flash);
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
    HudTextCache hudCaches[2];
    ShotgunFeedback shotgunFeedback;
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
    int gemDamageBuffTimers[2];
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
    int musicMuted;
    int returnToMainMenu;
    int finalScore;
    int highscorePass;
    int hudPlayer;
    int characterCount;
    int renderShakeOffset;
    Uint32 hitPauseUntil;
    Uint32 enemyFlashUntil[MAX_NPCS];
    float enemyKnockbackVel[MAX_NPCS];
    char selectedSave[SAVE_PATH_MAX];
    Uint32 now;

    if (!initSDL(&window, &renderer))
        return 1;

    mapTexture = NULL;
    hudCaches[0] = (HudTextCache){NULL, 0, -1, 0, 0};
    hudCaches[1] = (HudTextCache){NULL, 0, -1, 0, 0};
    font = TTF_OpenFont(PLUTO_FONT_PATH, 28);
    if (font == NULL)
        printf("[ERREUR] TTF_OpenFont : %s\n", TTF_GetError());

main_menu_start:
    mapTexture = NULL;
    mapFrame = NULL;
    hudCaches[0] = (HudTextCache){NULL, 0, -1, 0, 0};
    hudCaches[1] = (HudTextCache){NULL, 0, -1, 0, 0};
    menuChoice = run_main_menu(renderer);
    while (menuChoice == MAIN_MENU_QUIZ)
    {
        runMenuQuiz(renderer, font);
        SDL_RenderSetLogicalSize(renderer, SCREEN_W, SCREEN_H);
        menuChoice = run_main_menu(renderer);
    }
    if (menuChoice != MAIN_MENU_START)
    {
        if (font != NULL)
            TTF_CloseFont(font);
        shutdownSDL(window, renderer);
        return 0;
    }

    fade_black(renderer, SCREEN_W, SCREEN_H, 0);
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
        fade_black(renderer, SCREEN_W, SCREEN_H, 1);
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

    if (!selectedSaveLoaded && multiplayerMode && characterCount > 0)
        fade_black(renderer, SCREEN_W, SCREEN_H, 1);
    if (!selectedSaveLoaded && multiplayerMode && characterCount > 0 && !runCharacterSelectMenu(renderer, font, &selection))
    {
        if (font != NULL)
            TTF_CloseFont(font);
        shutdownSDL(window, renderer);
        return 0;
    }

    fade_black(renderer, SCREEN_W, SCREEN_H, 1);
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
    initShotgunFeedback(&shotgunFeedback, renderer);

    mapFrame = SDL_CreateRGBSurfaceWithFormat(0, SCREEN_W, SCREEN_H, 32, SDL_PIXELFORMAT_RGBA32);
    minimapReady = 0;
    if (mapFrame != NULL && init_minimap(&minimap, "minimap/minimap_pixel.bmp", SCREEN_W, SCREEN_H, WORLD_W, WORLD_H) == 0)
    {
        mapTexture = SDL_CreateTexture(renderer,
                                       SDL_PIXELFORMAT_RGBA32,
                                       SDL_TEXTUREACCESS_STREAMING,
                                       SCREEN_W, SCREEN_H);
        if (mapTexture != NULL)
        {
            SDL_SetTextureBlendMode(mapTexture, SDL_BLENDMODE_BLEND);
            minimapReady = 1;
        }
        else
        {
            SDL_FreeSurface(minimap.thumbnail);
            SDL_FreeSurface(minimap.man);
            minimap.thumbnail = NULL;
            minimap.man = NULL;
        }
    }

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
        if (!initialiserJoueurAvecAssets(&J2, renderer, 220, GROUND_Y,
                                         &characters[selection.p2CharacterIndex].outfits[selection.p2OutfitIndex]))
        {
            libererJoueur(&J1);
            if (font != NULL)
                TTF_CloseFont(font);
            shutdownSDL(window, renderer);
            return 1;
        }
    }
    else if (!initialiserJoueur(&J2, renderer, 220, GROUND_Y))
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
    gemDamageBuffTimers[0] = 0;
    gemDamageBuffTimers[1] = 0;
    J1.visible = 1;
    J2.visible = multiplayerMode;
    J2.facing = FACE_LEFT;
    joueurActif = 1;

    npcPhaseIndex = 0;
    for (i = 0; i < MAX_NPCS; i++)
    {
        memset(&npcs[i], 0, sizeof(npcs[i]));
        npcs[i].y = MARS_GROUND_Y + PLAYER_H - 128;
        npcs[i].w = 128;
        npcs[i].h = 128;
        npcs[i].groundY = MARS_GROUND_Y;
        npcs[i].state = ENEMY_REMOVED;
        npcs[i].aiState = NPC_AI_IDLE;
        npcs[i].onGround = 1;
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
    returnToMainMenu = 0;
    debugCollision = 0;
    musicMuted = 0;
    Mix_VolumeMusic(MIX_MAX_VOLUME);
    npcSwarmAlerted = 0;
    puzzleChallengeUsed = 0;
    for (i = 0; i < MAX_ENEMIES; i++)
        mapEnemies[i].active = 0;
    for (i = 0; i < MAX_NPCS; i++)
    {
        enemyFlashUntil[i] = 0;
        enemyKnockbackVel[i] = 0.0f;
    }
    hitPauseUntil = 0;
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
                             gems, MAX_SECONDARY_ENTITIES, &gemCount,
                             gemDamageBuffTimers,
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
        now = SDL_GetTicks();
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

        cameraX = clampCameraX(cameraX, viewW);
        renderShakeOffset = shotgunShakeOffset(&shotgunFeedback, now);
        cameraX = clampCameraX(cameraX + renderShakeOffset, viewW);

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
        renderShotgunFeedback(renderer, &shotgunFeedback, camera.x, now);

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
            renderFlashImpactPNJ(renderer, &npcs[i], now, enemyFlashUntil[i]);
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
            if (multiplayerMode && sharedView)
            {
                minimap.pos.x = (SCREEN_W - minimap.pos.w) / 2;
                minimap.pos.y = 10;
            }
            else if (multiplayerMode)
            {
                minimap.pos.x = viewW - minimap.pos.w - MINIMAP_MARGIN;
                minimap.pos.y = MINIMAP_MARGIN;
            }
            else
            {
                minimap.pos.x = SCREEN_W - minimap.pos.w - MINIMAP_MARGIN;
                minimap.pos.y = MINIMAP_MARGIN;
            }
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
            if (multiplayerMode)
            {
                dessinerJoueurSurMinimap(&minimap, mapFrame, &J1,
                                         SDL_MapRGB(mapFrame->format, 40, 170, 255));
                dessinerJoueurSurMinimap(&minimap, mapFrame, &J2,
                                         SDL_MapRGB(mapFrame->format, 255, 80, 210));
            }
            if (mapTexture != NULL)
            {
                mapDst.x = 0;
                mapDst.y = 0;
                mapDst.w = SCREEN_W;
                mapDst.h = SCREEN_H;
                SDL_UpdateTexture(mapTexture, NULL, mapFrame->pixels, mapFrame->pitch);
                SDL_RenderCopy(renderer, mapTexture, NULL, &mapDst);
            }
        }
        if (debugCollision)
            afficherDebugCollision(renderer, &J1, &J2, camera.x,
                                   shipGround, marsGround,
                                   stablePlatforms, movingPlatforms);
        if (multiplayerMode && sharedView)
        {
            afficherHUD(renderer, font, &J1, &J2, 1, &hudCaches[0], 10, 10);
            afficherHUD(renderer, font, &J1, &J2, 2, &hudCaches[1], SCREEN_W - 370, 10);
        }
        else
        {
            hudPlayer = multiplayerMode ? renderPass + 1 : 1;
            afficherHUD(renderer, font, &J1, &J2, hudPlayer, &hudCaches[hudPlayer - 1], 10, 10);
        }
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
                                                gems, gemCount,
                                                gemDamageBuffTimers,
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
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE ||
                    event.key.keysym.scancode == SDL_SCANCODE_P)
                {
                    int pauseAction = afficherMenuPause(renderer, font, &debugCollision, &musicMuted);

                    if (pauseAction == PAUSE_ACTION_SAVE)
                    {
                        if (sauvegarderProgression(&J1, &J2, bullets, npcs,
                                                   gems, gemCount,
                                                   gemDamageBuffTimers,
                                                   stablePlatforms, movingPlatforms,
                                                   movingDir, movingMin, movingMax,
                                                   joueurActif, puzzleChallengeUsed,
                                                   multiplayerMode,
                                                   renderer, font))
                            show_save_message(renderer, font, "Game saved");
                        else
                            show_save_message(renderer, font, "Save failed");
                    }
                    else if (pauseAction == PAUSE_ACTION_LOAD)
                    {
                        if (prompt_select_save(renderer, font, selectedSave, sizeof(selectedSave)) == 1)
                        {
                            if (load_game_state(selectedSave,
                                                &J1, &J2,
                                                bullets, MAX_BULLETS,
                                                npcs, MAX_NPCS,
                                                gems, MAX_SECONDARY_ENTITIES, &gemCount,
                                                gemDamageBuffTimers,
                                                stablePlatforms, STABLE_PLATFORM_COUNT,
                                                movingPlatforms, MOVING_PLATFORM_COUNT,
                                                movingDir, movingMin, movingMax,
                                                &joueurActif, &puzzleChallengeUsed,
                                                &multiplayerMode))
                            {
                                clearShotgunFeedbackRuntime(&shotgunFeedback);
                                hitPauseUntil = 0;
                                for (i = 0; i < MAX_NPCS; i++)
                                {
                                    enemyFlashUntil[i] = 0;
                                    enemyKnockbackVel[i] = 0.0f;
                                }
                                show_save_message(renderer, font, "Game loaded");
                            }
                            else
                                show_save_message(renderer, font, "Load failed");
                        }
                    }
                    else if (pauseAction == PAUSE_ACTION_MAIN_MENU)
                    {
                        quitterSansScore = 1;
                        returnToMainMenu = 1;
                        running = 0;
                    }
                    else if (pauseAction == PAUSE_ACTION_QUIT)
                    {
                        quitterSansScore = 1;
                        running = 0;
                    }
                }

                if (event.key.keysym.scancode == SDL_SCANCODE_F5)
                {
                    if (sauvegarderProgression(&J1, &J2, bullets, npcs,
                                               gems, gemCount,
                                               gemDamageBuffTimers,
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
                {
                    int shellMask = (J1.shotgunShells & SHOTGUN_SHELL_FIRST) ?
                                    SHOTGUN_SHELL_FIRST : SHOTGUN_SHELL_SECOND;

                    tirerShotgunAvecFeedback(bullets, MAX_BULLETS, &J1, 1, now,
                                             shellMask, &shotgunFeedback);
                }

                if (multiplayerMode &&
                    (event.key.keysym.scancode == SDL_SCANCODE_RCTRL ||
                     event.key.keysym.scancode == SDL_SCANCODE_RETURN))
                {
                    int shellMask = (J2.shotgunShells & SHOTGUN_SHELL_FIRST) ?
                                    SHOTGUN_SHELL_FIRST : SHOTGUN_SHELL_SECOND;

                    tirerShotgunAvecFeedback(bullets, MAX_BULLETS, &J2, 2, now,
                                             shellMask, &shotgunFeedback);
                }

            }

        }

        if (!running)
            continue;

        if (now < hitPauseUntil)
        {
            SDL_Delay(8);
            continue;
        }

        for (p = 0; p < MOVING_PLATFORM_COUNT; p++)
        {
            SDL_Rect oldPlatform = movingPlatforms[p];
            int dx;
            int carryJ1 = joueurPorteParPlateforme(&J1, oldPlatform);
            int carryJ2 = multiplayerMode && joueurPorteParPlateforme(&J2, oldPlatform);

            movingPlatforms[p].x += movingDir[p] * 3;
            if (movingPlatforms[p].x < movingMin[p] || movingPlatforms[p].x > movingMax[p])
            {
                movingDir[p] = -movingDir[p];
                movingPlatforms[p].x += movingDir[p] * 3;
            }

            dx = movingPlatforms[p].x - oldPlatform.x;
            if (carryJ1)
                deplacerJoueurAvecPlateforme(&J1, dx);
            if (carryJ2)
                deplacerJoueurAvecPlateforme(&J2, dx);
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
            if (centreJoueurSurPlateforme(&J1, stablePlatforms[p]) &&
                J1.posScreen.y + J1.posScreen.h <= stablePlatforms[p].y + 12)
                J1.floorY = stablePlatforms[p].y - PLAYER_H;
            if (multiplayerMode &&
                centreJoueurSurPlateforme(&J2, stablePlatforms[p]) &&
                J2.posScreen.y + J2.posScreen.h <= stablePlatforms[p].y + 12)
                J2.floorY = stablePlatforms[p].y - PLAYER_H;
        }
        for (p = 0; p < MOVING_PLATFORM_COUNT; p++)
        {
            if (centreJoueurSurPlateforme(&J1, movingPlatforms[p]) &&
                J1.posScreen.y + J1.posScreen.h <= movingPlatforms[p].y + 12)
                J1.floorY = movingPlatforms[p].y - PLAYER_H;
            if (multiplayerMode &&
                centreJoueurSurPlateforme(&J2, movingPlatforms[p]) &&
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
            for (p = 0; p < 2; p++)
                if (gemDamageBuffTimers[p] > 0)
                    gemDamageBuffTimers[p]--;

            if (J1.alive && J1.visible)
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

                    if (collisionAABB(J1.posScreen, gemRect))
                    {
                        ajouterScoreJoueur(&J1, GEM_SCORE_VALUE);
                        gemDamageBuffTimers[0] = GEM_BUFF_DURATION;
                        gems[i].active = 0;
                    }
                }
            }

            if (multiplayerMode && J2.alive && J2.visible)
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

                    if (collisionAABB(J2.posScreen, gemRect))
                    {
                        ajouterScoreJoueur(&J2, GEM_SCORE_VALUE);
                        gemDamageBuffTimers[1] = GEM_BUFF_DURATION;
                        gems[i].active = 0;
                    }
                }
            }

            verifierDefaiteJoueur(&J1, now, &puzzleChallengeUsed, &running, renderer);
            if (running && multiplayerMode)
                verifierDefaiteJoueur(&J2, now, &puzzleChallengeUsed, &running, renderer);
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
                if (i == 0)
                    continue;

                if (npcCanSeePlayer(&npcs[i], joueurCourant) ||
                    (multiplayerMode && npcCanSeePlayer(&npcs[i], (joueurCourant == &J1) ? &J2 : &J1)))
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

            joueurCourant = selectionnerCibleNPC(&npcs[i], &J1, &J2,
                                                 multiplayerMode, joueurCourant);

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

            updateNPC(&npcs[i], joueurCourant, (i == 0) ? 0 : npcSwarmAlerted,
                      npcUpdateCameraX, npcUpdateViewW,
                      stablePlatforms, STABLE_PLATFORM_COUNT,
                      movingPlatforms, MOVING_PLATFORM_COUNT);
            appliquerFeedbackImpactPNJ(&npcs[i], &enemyKnockbackVel[i]);

            if (isNPCActive(&npcs[i]) &&
                npcs[i].aiState == NPC_AI_ATTACKING &&
                joueurCourant->alive &&
                joueurCourant->visible &&
                (npcs[i].npcType == NPC_TYPE_RUSHER || now >= npcs[i].nextAttackAt) &&
                now >= joueurCourant->invulnerableUntil &&
                attaquePNJActive(&npcs[i], now))
            {
                SDL_Rect enemyAttackRect = hitboxAttaquePNJ(&npcs[i]);

                if (collisionAABB(enemyAttackRect, joueurCourant->posScreen))
                {
                    if (npcs[i].npcType == NPC_TYPE_RUSHER)
                    {
                        npcs[i].attackDamageApplied = 1;
                    }
                    else
                    {
                        npcs[i].state = ENEMY_ATTACKING;
                        npcs[i].aiState = NPC_AI_ATTACKING;
                        npcs[i].attackStartedAt = now;
                        npcs[i].nextAttackAt = now + ENEMY_ATTACK_COOLDOWN;
                        npcs[i].action = 0;
                    }
                    ajouterScoreJoueur(joueurCourant, -ENEMY_DAMAGE_SCORE);
                    appliquerDegatsJoueur(joueurCourant,
                                          (npcs[i].npcType == NPC_TYPE_RUSHER) ? RUSHER_DAMAGE : 1,
                                          now);
                }
            }

            for (j = 0; j < MAX_BULLETS; j++)
            {
                if (!bullets[j].active)
                    continue;

                if (isNPCActive(&npcs[i]) &&
                    collisionAABB(bullets[j].rect, npcs[i].dstRect))
                {
                    int hitDamage = 1;

                    if (bullets[j].owner == 1 && gemDamageBuffTimers[0] > 0)
                        hitDamage = GEM_DAMAGE_BONUS;
                    else if (bullets[j].owner == 2 && gemDamageBuffTimers[1] > 0)
                        hitDamage = GEM_DAMAGE_BONUS;

                    enemyFlashUntil[i] = now + SHOTGUN_ENEMY_FLASH_MS;
                    enemyKnockbackVel[i] = (bullets[j].velX >= 0.0f) ?
                                           SHOTGUN_ENEMY_KNOCKBACK :
                                           -SHOTGUN_ENEMY_KNOCKBACK;
                    if (npcs[i].npcType == NPC_TYPE_RUSHER)
                        enemyKnockbackVel[i] *= RUSHER_KNOCKBACK_RESISTANCE;
                    hitPauseUntil = now + SHOTGUN_HIT_PAUSE_MS;
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
                        npcs[i].aiState = NPC_AI_IDLE;
                        npcs[i].action = 0;
                        if (bullets[j].owner == 1)
                            ajouterScoreJoueur(&J1, ENEMY_KILL_SCORE);
                        else if (bullets[j].owner == 2)
                            ajouterScoreJoueur(&J2, ENEMY_KILL_SCORE);
                    }
                    else
                    {
                        npcs[i].state = ENEMY_INJURED;
                        npcs[i].aiState = NPC_AI_AWARE;
                    }
                }
            }
        }

        separerPNJEntasses(npcs);
    }

    if (!quitterSansScore)
    {
        for (highscorePass = 0; highscorePass < (multiplayerMode ? 2 : 1); highscorePass++)
        {
            finalScore = (highscorePass == 0) ? J1.score : J2.score;
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
            if (hs.go_to_puzzle)
                runPuzzleWithFade(renderer);
            free_highscore(&hs);
        }
    }

    if (minimapReady)
    {
        SDL_FreeSurface(minimap.thumbnail);
        SDL_FreeSurface(minimap.man);
    }
    if (mapFrame != NULL)
        SDL_FreeSurface(mapFrame);
    SDL_DestroyTexture(mapTexture);
    SDL_DestroyTexture(background);
    SDL_DestroyTexture(backgroundMid);
    SDL_DestroyTexture(backgroundFore);
    SDL_DestroyTexture(shipGroundTex);
    SDL_DestroyTexture(marsGroundTex);
    SDL_DestroyTexture(shipPlatformTex);
    SDL_DestroyTexture(marsPlatformTex);
    SDL_DestroyTexture(gemTexture);
    destroyShotgunFeedback(&shotgunFeedback);

    for (i = 0; i < MAX_NPCS; i++)
        destroyNPC(&npcs[i]);
    libererJoueur(&J1);
    libererJoueur(&J2);

    for (i = 0; i < 2; i++)
    {
        if (hudCaches[i].scoreTexture != NULL)
        {
            SDL_DestroyTexture(hudCaches[i].scoreTexture);
            hudCaches[i].scoreTexture = NULL;
        }
    }

    if (returnToMainMenu)
        goto main_menu_start;

    if (font != NULL)
        TTF_CloseFont(font);

    shutdownSDL(window, renderer);
    return 0;
}
