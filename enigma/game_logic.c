#include "game.h"

/* ════════════════════════════════════════════════════════
   FILE LOADING  — format per question:
     question text
     choice A
     choice B
     choice C
     correct index (0/1/2)
═════════════════════════════════════════════════════════ */
void game_load(Game *g) {
    FILE *f = fopen(QUESTIONS_FILE, "r");
    if (!f) { fprintf(stderr,"Cannot open %s\n", QUESTIONS_FILE); return; }
    g->total = 0;
    while (g->total < MAX_Q) {
        Question *q = &g->bank[g->total];
        /* question */
        if (!fgets(q->q, MAX_STR, f)) break;
        q->q[strcspn(q->q,"\n")] = 0;
        if (!q->q[0]) continue;           /* skip blank lines */
        /* 3 choices */
        int ok = 1;
        for (int i=0; i<3; i++) {
            if (!fgets(q->c[i], MAX_STR, f)) { ok=0; break; }
            q->c[i][strcspn(q->c[i],"\n")] = 0;
            /* strip trailing " •" markers from your file */
            char *dot = strstr(q->c[i]," •");
            if (dot) *dot = 0;
        }
        if (!ok) break;
        /* answer index */
        char idx[8];
        if (!fgets(idx, sizeof(idx), f)) break;
        q->ans  = atoi(idx);
        q->seen = 0;
        g->total++;
    }
    fclose(f);
}

/* ════════════════════════════════════════════════════════
   GAME LOGIC
═════════════════════════════════════════════════════════ */
void game_init(Game *g) {
    g->score   = 0;
    g->lives   = 3;
    g->asked   = 0;
    g->cur     = -1;
    g->last_ok = 0;
    for (int i=0; i<g->total; i++) g->bank[i].seen = 0;
}

/* pick a random unseen question */
void game_pick(Game *g) {
    /* after 10 questions → session done */
    if (g->asked >= 10) { g->state = S_GAMEOVER; return; }

    int pool[MAX_Q], n=0;
    for (int i=0; i<g->total; i++)
        if (!g->bank[i].seen) pool[n++]=i;

    if (n==0) { g->state = S_GAMEOVER; return; } /* all used */

    int pick = pool[rand()%n];
    g->bank[pick].seen = 1;
    g->cur     = pick;
    g->t_start = SDL_GetTicks();
}

/* start: init + pick first question */
void game_start(Game *g) {
    game_init(g);
    game_pick(g);
    g->state = S_PLAYING;
}

void game_answer(Game *g, int choice) {
    g->asked++;
    if (choice == g->bank[g->cur].ans) {
        g->last_ok = 1;
        g->score  += 10;
    } else {
        g->last_ok = 0;
        g->lives--;
    }
    g->rz_scale = 0.05f;
    g->rz_angle = 0;
    g->state = S_RESULT;
}

void game_tick(Game *g) {
    if (SDL_GetTicks() - g->t_start >= TIME_LIMIT) {
        g->last_ok = 0;
        g->lives--;
        g->asked++;
        g->rz_scale = 0.05f;
        g->rz_angle = 0;
        g->state = S_RESULT;
    }
}

/* ════════════════════════════════════════════════════════
   EVENTS
═════════════════════════════════════════════════════════ */
void ev_menu(Game *g, SDL_Event *e) {
    if (e->type == SDL_KEYDOWN) {
        if (e->key.keysym.sym==SDLK_RETURN || e->key.keysym.sym==SDLK_KP_ENTER)
            game_start(g);
        if (e->key.keysym.sym==SDLK_ESCAPE)
            { SDL_Event q; q.type=SDL_QUIT; SDL_PushEvent(&q); }
    }
    if (e->type==SDL_MOUSEBUTTONDOWN) {
        int mx=e->button.x, my=e->button.y;
        if (mx>=300&&mx<=500) {
            if (my>=290&&my<=345) game_start(g);
            if (my>=365&&my<=420) { SDL_Event q; q.type=SDL_QUIT; SDL_PushEvent(&q); }
        }
    }
}

void ev_playing(Game *g, SDL_Event *e) {
    if (e->type==SDL_KEYDOWN) {
        SDL_Keycode k=e->key.keysym.sym;
        if (k==SDLK_1||k==SDLK_KP_1) game_answer(g,0);
        if (k==SDLK_2||k==SDLK_KP_2) game_answer(g,1);
        if (k==SDLK_3||k==SDLK_KP_3) game_answer(g,2);
        if (k==SDLK_ESCAPE)           g->state=S_MENU;
    }
    if (e->type==SDL_MOUSEBUTTONDOWN) {
        int mx=e->button.x, my=e->button.y;
        if (mx>=55&&mx<=SCREEN_W-55)
            for (int i=0;i<3;i++)
                if (my>=185+i*110 && my<=185+i*110+90)
                    game_answer(g,i);
    }
}

void ev_result(Game *g, SDL_Event *e) {
    int go = 0;
    if (e->type==SDL_KEYDOWN &&
        (e->key.keysym.sym==SDLK_SPACE||
         e->key.keysym.sym==SDLK_RETURN||
         e->key.keysym.sym==SDLK_KP_ENTER)) go=1;
    if (e->type==SDL_MOUSEBUTTONDOWN) go=1;
    if (!go) return;

    if (g->lives<=0 || g->asked>=10) {
        g->rz_scale=0.05f; g->rz_angle=0;
        g->state=S_GAMEOVER;
    } else {
        game_pick(g);
        g->state=S_PLAYING;
    }
}

void ev_gameover(Game *g, SDL_Event *e) {
    if (e->type==SDL_KEYDOWN) {
        SDL_Keycode k=e->key.keysym.sym;
        if (k==SDLK_r||k==SDLK_RETURN) { game_start(g); }
        if (k==SDLK_m||k==SDLK_ESCAPE)   g->state=S_MENU;
    }
    if (e->type==SDL_MOUSEBUTTONDOWN) {
        int mx=e->button.x, my=e->button.y;
        if (mx>=300&&mx<=500) {
            if (my>=430&&my<=485) game_start(g);
            if (my>=505&&my<=560) g->state=S_MENU;
        }
    }
}
