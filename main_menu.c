#include "main_menu.h"

#define MAIN_MENU_W 1900
#define MAIN_MENU_HEIGHT 1000
#define MAIN_MENU_BUTTONS 6
#define MAIN_MENU_BG "assets/main_menu/Background.png"
#define MAIN_MENU_LOGO "assets/main_menu/Logo.png"
#define MAIN_MENU_START_IMG "assets/main_menu/StartID.png"
#define MAIN_MENU_START_HOVER "assets/main_menu/Start_hover.png"
#define MAIN_MENU_SETTINGS_IMG "assets/main_menu/settings ID.png"
#define MAIN_MENU_SETTINGS_HOVER "assets/main_menu/Settings_hover.png"
#define MAIN_MENU_SCORE_IMG "assets/main_menu/score ids.png"
#define MAIN_MENU_SCORE_HOVER "assets/main_menu/Score_hoverd.png"
#define MAIN_MENU_BACKSTORY_IMG "assets/main_menu/backstory ID.png"
#define MAIN_MENU_BACKSTORY_HOVER "assets/main_menu/Backstory_hoverd.png"
#define MAIN_MENU_EXIT_IMG "assets/main_menu/exit ID.png"
#define MAIN_MENU_GAME2_IMG "assets/main_menu/game2.png"
#define MAIN_MENU_GAME2_HOVER "assets/main_menu/game2_h.png"
#define MAIN_MENU_HOVER_SOUND "assets/main_menu/Hover.mp3"
#define MAIN_MENU_DEBUG 0

typedef struct
{
    SDL_Texture *texture;
    int w;
    int h;
    SDL_Rect visible;
} MainMenuImage;

typedef struct
{
    MainMenuImage normal;
    MainMenuImage hover;
    SDL_Rect visibleRect;
    int hovered;
    int down;
} MainMenuButton;

typedef struct
{
    MainMenuImage background;
    MainMenuImage logo;
    MainMenuButton buttons[MAIN_MENU_BUTTONS];
    Mix_Chunk *hoverSound;
} MainMenu;

static SDL_Rect main_menu_visible_bounds(SDL_Surface *surface)
{
    SDL_Rect bounds;
    int x;
    int y;
    int minX;
    int minY;
    int maxX;
    int maxY;
    int found = 0;
    Uint32 pixel;
    Uint8 r;
    Uint8 g;
    Uint8 b;
    Uint8 a;
    Uint32 *pixels;

    bounds = (SDL_Rect){0, 0, surface->w, surface->h};
    minX = surface->w;
    minY = surface->h;
    maxX = 0;
    maxY = 0;

    if (SDL_MUSTLOCK(surface))
        SDL_LockSurface(surface);

    pixels = (Uint32 *)surface->pixels;
    for (y = 0; y < surface->h; y++)
    {
        for (x = 0; x < surface->w; x++)
        {
            pixel = pixels[y * surface->w + x];
            SDL_GetRGBA(pixel, surface->format, &r, &g, &b, &a);
            if (a > 8)
            {
                if (x < minX)
                    minX = x;
                if (y < minY)
                    minY = y;
                if (x > maxX)
                    maxX = x;
                if (y > maxY)
                    maxY = y;
                found = 1;
            }
        }
    }

    if (SDL_MUSTLOCK(surface))
        SDL_UnlockSurface(surface);

    if (found)
        bounds = (SDL_Rect){minX, minY, maxX - minX + 1, maxY - minY + 1};

    return bounds;
}

static MainMenuImage main_menu_image(SDL_Renderer *renderer, const char *path)
{
    MainMenuImage image;
    SDL_Surface *loaded;
    SDL_Surface *surface;

    memset(&image, 0, sizeof(image));
    loaded = IMG_Load(path);
    if (loaded == NULL)
        return image;

    surface = SDL_ConvertSurfaceFormat(loaded, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(loaded);
    if (surface == NULL)
        return image;

    image.w = surface->w;
    image.h = surface->h;
    image.visible = main_menu_visible_bounds(surface);
    image.texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return image;
}

static void main_menu_destroy_image(MainMenuImage *image)
{
    if (image->texture != NULL)
        SDL_DestroyTexture(image->texture);
    memset(image, 0, sizeof(*image));
}

static SDL_Rect main_menu_scaled_visible_rect(SDL_Rect base, int hovered, int down)
{
    SDL_Rect rect = base;
    int scaleNum = 100;
    int scaleDen = 100;
    int nw;
    int nh;

    if (down)
        scaleNum = 105;
    else if (hovered)
        scaleNum = 98;

    nw = base.w * scaleNum / scaleDen;
    nh = base.h * scaleNum / scaleDen;
    rect.x = base.x + (base.w - nw) / 2;
    rect.y = base.y + (base.h - nh) / 2;
    rect.w = nw;
    rect.h = nh;
    return rect;
}

static int main_menu_inside(int x, int y, SDL_Rect rect)
{
    return x >= rect.x && x <= rect.x + rect.w &&
           y >= rect.y && y <= rect.y + rect.h;
}

static void main_menu_init_button(MainMenuButton *button, SDL_Renderer *renderer,
                                  const char *normal, const char *hover)
{
    button->normal = main_menu_image(renderer, normal);
    button->hover = main_menu_image(renderer, hover);
    if (button->hover.texture == NULL)
        button->hover = main_menu_image(renderer, normal);
    if (button->normal.texture == NULL)
        button->normal.visible = (SDL_Rect){0, 0, 200, 100};
    button->visibleRect = (SDL_Rect){0, 0, button->normal.visible.w, button->normal.visible.h};
    button->hovered = 0;
    button->down = 0;
}

static int main_menu_fit_dimension(int sourceW, int sourceH,
                                   int maxW, int maxH,
                                   int *targetW, int *targetH)
{
    int scaleW;
    int scaleH;
    int scale;

    if (sourceW <= 0 || sourceH <= 0)
    {
        *targetW = maxW;
        *targetH = maxH;
        return 0;
    }

    scaleW = maxW * 1000 / sourceW;
    scaleH = maxH * 1000 / sourceH;
    scale = scaleW < scaleH ? scaleW : scaleH;
    if (scale > 1000)
        scale = 1000;
    if (scale <= 0)
        scale = 1;
    *targetW = sourceW * scale / 1000;
    *targetH = sourceH * scale / 1000;
    if (*targetW <= 0)
        *targetW = 1;
    if (*targetH <= 0)
        *targetH = 1;
    return scale;
}

static void main_menu_layout(MainMenu *menu, int windowW, int windowH)
{
    int leftX = windowW * 7 / 100;
    int topY = windowH * 12 / 100;
    int bottomReserve = windowH * 18 / 100;
    int gap = windowH * 42 / 1000;
    int columnW = windowW * 20 / 100;
    int maxButtonH = windowH * 12 / 100;
    int availableH;
    int totalH = 0;
    int targetW[4];
    int targetH[4];
    int y;
    int i;

    if (leftX < 90)
        leftX = 90;
    if (gap < 34)
        gap = 34;
    if (columnW > 360)
        columnW = 360;
    if (maxButtonH > 126)
        maxButtonH = 126;
    availableH = windowH - topY - bottomReserve;

    for (i = 0; i < 4; i++)
    {
        main_menu_fit_dimension(menu->buttons[i].normal.visible.w,
                                menu->buttons[i].normal.visible.h,
                                columnW, maxButtonH,
                                &targetW[i], &targetH[i]);
        totalH += targetH[i];
    }

    if (targetW[1] > 0 && menu->buttons[0].normal.visible.w > 0)
    {
        totalH -= targetH[0];
        targetW[0] = targetW[1];
        targetH[0] = menu->buttons[0].normal.visible.h * targetW[0] /
                     menu->buttons[0].normal.visible.w;
        totalH += targetH[0];
    }

    totalH += gap * 3;
    if (totalH > availableH)
    {
        int scaledGap = gap * availableH / totalH;
        int scale = (availableH - scaledGap * 3) * 1000 / (totalH - gap * 3);
        if (scaledGap < 24)
            scaledGap = 24;
        if (scale <= 0)
            scale = 1;
        gap = scaledGap;
        for (i = 0; i < 4; i++)
        {
            targetW[i] = targetW[i] * scale / 1000;
            targetH[i] = targetH[i] * scale / 1000;
            if (targetW[i] <= 0)
                targetW[i] = 1;
            if (targetH[i] <= 0)
                targetH[i] = 1;
        }
    }

    y = topY;
    for (i = 0; i < 4; i++)
    {
        menu->buttons[i].visibleRect.x = leftX;
        menu->buttons[i].visibleRect.y = y;
        menu->buttons[i].visibleRect.w = targetW[i];
        menu->buttons[i].visibleRect.h = targetH[i];
        y += targetH[i] + gap;
    }

    main_menu_fit_dimension(menu->buttons[4].normal.visible.w,
                            menu->buttons[4].normal.visible.h,
                            windowW * 14 / 100,
                            windowH * 9 / 100,
                            &menu->buttons[4].visibleRect.w,
                            &menu->buttons[4].visibleRect.h);
    menu->buttons[4].visibleRect.x = windowW - menu->buttons[4].visibleRect.w - windowW * 6 / 100;
    menu->buttons[4].visibleRect.y = windowH - menu->buttons[4].visibleRect.h - windowH * 7 / 100;

    main_menu_fit_dimension(menu->buttons[5].normal.visible.w,
                            menu->buttons[5].normal.visible.h,
                            windowW * 18 / 100,
                            windowH * 10 / 100,
                            &menu->buttons[5].visibleRect.w,
                            &menu->buttons[5].visibleRect.h);
    menu->buttons[5].visibleRect.x = (windowW - menu->buttons[5].visibleRect.w) / 2;
    menu->buttons[5].visibleRect.y = windowH - menu->buttons[5].visibleRect.h - windowH * 6 / 100;
}

static void main_menu_init(MainMenu *menu, SDL_Renderer *renderer)
{
    memset(menu, 0, sizeof(*menu));
    menu->background = main_menu_image(renderer, MAIN_MENU_BG);
    menu->logo = main_menu_image(renderer, MAIN_MENU_LOGO);
    main_menu_init_button(&menu->buttons[0], renderer, MAIN_MENU_START_IMG, MAIN_MENU_START_HOVER);
    main_menu_init_button(&menu->buttons[1], renderer, MAIN_MENU_SETTINGS_IMG, MAIN_MENU_SETTINGS_HOVER);
    main_menu_init_button(&menu->buttons[2], renderer, MAIN_MENU_SCORE_IMG, MAIN_MENU_SCORE_HOVER);
    main_menu_init_button(&menu->buttons[3], renderer, MAIN_MENU_BACKSTORY_IMG, MAIN_MENU_BACKSTORY_HOVER);
    main_menu_init_button(&menu->buttons[4], renderer, MAIN_MENU_EXIT_IMG, MAIN_MENU_EXIT_IMG);
    main_menu_init_button(&menu->buttons[5], renderer, MAIN_MENU_GAME2_IMG, MAIN_MENU_GAME2_HOVER);
    main_menu_layout(menu, MAIN_MENU_W, MAIN_MENU_HEIGHT);
    menu->hoverSound = Mix_LoadWAV(MAIN_MENU_HOVER_SOUND);
    stellarMusicStartMenu();
}

static void main_menu_destroy(MainMenu *menu)
{
    int i;

    main_menu_destroy_image(&menu->background);
    main_menu_destroy_image(&menu->logo);
    for (i = 0; i < MAIN_MENU_BUTTONS; i++)
    {
        main_menu_destroy_image(&menu->buttons[i].normal);
        main_menu_destroy_image(&menu->buttons[i].hover);
    }
    if (menu->hoverSound != NULL)
        Mix_FreeChunk(menu->hoverSound);
}

static void main_menu_mouse_logical(SDL_Renderer *renderer, int *x, int *y)
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

static void main_menu_event_logical(SDL_Renderer *renderer, const SDL_Event *event, int *x, int *y)
{
    float lx;
    float ly;

    SDL_RenderWindowToLogical(renderer, event->button.x, event->button.y, &lx, &ly);
    *x = (int)lx;
    *y = (int)ly;
}

static void main_menu_update_hover(MainMenu *menu, SDL_Renderer *renderer)
{
    int x;
    int y;
    int prev;
    int i;

    main_menu_mouse_logical(renderer, &x, &y);
    for (i = 0; i < MAIN_MENU_BUTTONS; i++)
    {
        prev = menu->buttons[i].hovered;
        menu->buttons[i].hovered = main_menu_inside(x, y, menu->buttons[i].visibleRect);
        if (menu->buttons[i].hovered && !prev && menu->hoverSound != NULL)
            Mix_PlayChannel(-1, menu->hoverSound, 0);
    }
}

static void main_menu_render(MainMenu *menu, SDL_Renderer *renderer)
{
    SDL_Rect logical = {0, 0, MAIN_MENU_W, MAIN_MENU_HEIGHT};
    SDL_Rect logoRect;
    SDL_Rect logoFull;
    int lw;
    int lh;
    int i;

    SDL_RenderSetLogicalSize(renderer, MAIN_MENU_W, MAIN_MENU_HEIGHT);
    main_menu_layout(menu, MAIN_MENU_W, MAIN_MENU_HEIGHT);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    if (menu->background.texture != NULL)
        SDL_RenderCopy(renderer, menu->background.texture, NULL, &logical);

    for (i = 0; i < MAIN_MENU_BUTTONS; i++)
    {
        MainMenuImage *image = menu->buttons[i].hovered ? &menu->buttons[i].hover : &menu->buttons[i].normal;
        SDL_Rect visibleRect = main_menu_scaled_visible_rect(menu->buttons[i].visibleRect,
                                                             menu->buttons[i].hovered,
                                                             menu->buttons[i].down);
        SDL_Rect fullRect;

        if (image->texture == NULL)
            image = &menu->buttons[i].normal;
        if (image->texture == NULL || image->visible.w <= 0 || image->visible.h <= 0)
            continue;

        fullRect.w = image->w * visibleRect.w / image->visible.w;
        fullRect.h = image->h * visibleRect.h / image->visible.h;
        if (fullRect.w <= 0)
            fullRect.w = 1;
        if (fullRect.h <= 0)
            fullRect.h = 1;
        fullRect.x = visibleRect.x - image->visible.x * fullRect.w / image->w;
        fullRect.y = visibleRect.y - image->visible.y * fullRect.h / image->h;
        SDL_RenderCopy(renderer, image->texture, NULL, &fullRect);

#if MAIN_MENU_DEBUG
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &fullRect);
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderDrawRect(renderer, &visibleRect);
#endif
    }

    if (menu->logo.texture != NULL)
    {
        main_menu_fit_dimension(menu->logo.visible.w,
                                menu->logo.visible.h,
                                MAIN_MENU_W * 15 / 100,
                                MAIN_MENU_HEIGHT * 24 / 100,
                                &lw, &lh);
        logoRect = (SDL_Rect){MAIN_MENU_W - lw - MAIN_MENU_W * 5 / 100,
                              MAIN_MENU_HEIGHT * 6 / 100, lw, lh};
        logoFull.w = menu->logo.w * logoRect.w / menu->logo.visible.w;
        logoFull.h = menu->logo.h * logoRect.h / menu->logo.visible.h;
        logoFull.x = logoRect.x - menu->logo.visible.x * logoFull.w / menu->logo.w;
        logoFull.y = logoRect.y - menu->logo.visible.y * logoFull.h / menu->logo.h;
        SDL_RenderCopy(renderer, menu->logo.texture, NULL, &logoFull);
#if MAIN_MENU_DEBUG
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &logoFull);
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderDrawRect(renderer, &logoRect);
#endif
    }
}

static void main_menu_fade(SDL_Renderer *renderer, MainMenu *menu, int fadeOut)
{
    const Uint32 duration = 420;
    Uint32 start = SDL_GetTicks();
    Uint32 elapsed = 0;
    SDL_Rect full = {0, 0, MAIN_MENU_W, MAIN_MENU_HEIGHT};

    while (elapsed < duration)
    {
        Uint8 alpha;
        elapsed = SDL_GetTicks() - start;
        if (elapsed > duration) elapsed = duration;
        alpha = fadeOut ? (Uint8)(255u * elapsed / duration)
                        : (Uint8)(255u - 255u * elapsed / duration);
        main_menu_render(menu, renderer);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, alpha);
        SDL_RenderFillRect(renderer, &full);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
}

int run_main_menu(SDL_Renderer *renderer)
{
    MainMenu menu;
    SDL_Event event;
    int running = 1;
    int result = MAIN_MENU_QUIT;
    int i;

    main_menu_init(&menu, renderer);
    SDL_RenderSetLogicalSize(renderer, MAIN_MENU_W, MAIN_MENU_HEIGHT);
    main_menu_fade(renderer, &menu, 0);
    while (running)
    {
        stellarMusicUpdateMenu();
        main_menu_update_hover(&menu, renderer);
        main_menu_render(&menu, renderer);
        SDL_RenderPresent(renderer);
        while (SDL_PollEvent(&event))
        {
            int mx;
            int my;
            if (event.type == SDL_QUIT)
                running = 0;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
                running = 0;
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
            {
                (void)event;
                main_menu_mouse_logical(renderer, &mx, &my);
                for (i = 0; i < MAIN_MENU_BUTTONS; i++)
                    menu.buttons[i].down = main_menu_inside(mx, my, menu.buttons[i].visibleRect);
            }
            if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT)
            {
                main_menu_mouse_logical(renderer, &mx, &my);
                for (i = 0; i < MAIN_MENU_BUTTONS; i++)
                {
                    if (menu.buttons[i].down &&
                        main_menu_inside(mx, my, menu.buttons[i].visibleRect))
                    {
                        if (i == 0)
                        {
                            result = MAIN_MENU_START;
                            main_menu_fade(renderer, &menu, 1);
                            running = 0;
                        }
                        if (i == 5)
                        {
                            result = MAIN_MENU_QUIZ;
                            main_menu_fade(renderer, &menu, 1);
                            running = 0;
                        }
                        if (i == 4)
                            running = 0;
                    }
                    menu.buttons[i].down = 0;
                }
            }
        }
        SDL_Delay(16);
    }
    main_menu_destroy(&menu);
    SDL_RenderSetLogicalSize(renderer, SCREEN_W, SCREEN_H);
    return result;
}

static void draw_mode_text(SDL_Renderer *renderer, TTF_Font *font,
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

int prompt_game_mode(SDL_Renderer *renderer, TTF_Font *font)
{
    MainMenuImage background = main_menu_image(renderer, MAIN_MENU_BG);
    Mix_Chunk *hover = Mix_LoadWAV(MAIN_MENU_HOVER_SOUND);
    SDL_Event event;
    SDL_Rect single = {SCREEN_W / 2 - 360, SCREEN_H / 2 - 20, 300, 86};
    SDL_Rect multi = {SCREEN_W / 2 + 60, SCREEN_H / 2 - 20, 300, 86};
    int singleHover = 0;
    int multiHover = 0;
    int oldSingle;
    int oldMulti;
    int x;
    int y;
    int result = 0;

    while (result == 0)
    {
        stellarMusicUpdateMenu();
        main_menu_mouse_logical(renderer, &x, &y);
        oldSingle = singleHover;
        oldMulti = multiHover;
        singleHover = main_menu_inside(x, y, single);
        multiHover = main_menu_inside(x, y, multi);
        if (hover != NULL && ((singleHover && !oldSingle) || (multiHover && !oldMulti)))
            Mix_PlayChannel(-1, hover, 0);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        if (background.texture != NULL)
            SDL_RenderCopy(renderer, background.texture, NULL, NULL);
        draw_mode_text(renderer, font, "Choose Game Mode", SCREEN_W / 2 - 170, SCREEN_H / 2 - 150,
                       (SDL_Color){255, 220, 80, 255});
        SDL_SetRenderDrawColor(renderer, singleHover ? 80 : 30, singleHover ? 100 : 45, singleHover ? 145 : 75, 230);
        SDL_RenderFillRect(renderer, &single);
        SDL_SetRenderDrawColor(renderer, multiHover ? 80 : 30, multiHover ? 100 : 45, multiHover ? 145 : 75, 230);
        SDL_RenderFillRect(renderer, &multi);
        SDL_SetRenderDrawColor(renderer, 230, 230, 245, 255);
        SDL_RenderDrawRect(renderer, &single);
        SDL_RenderDrawRect(renderer, &multi);
        draw_mode_text(renderer, font, "Singleplayer", single.x + 42, single.y + 25,
                       (SDL_Color){255, 255, 255, 255});
        draw_mode_text(renderer, font, "Multiplayer", multi.x + 56, multi.y + 25,
                       (SDL_Color){255, 255, 255, 255});
        SDL_RenderPresent(renderer);

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                result = -1;
            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    result = -1;
                if (event.key.keysym.sym == SDLK_1 || event.key.keysym.sym == SDLK_s)
                    result = GAME_MODE_SINGLE;
                if (event.key.keysym.sym == SDLK_2 || event.key.keysym.sym == SDLK_m)
                    result = GAME_MODE_MULTI;
            }
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
            {
                int mx;
                int my;
                main_menu_event_logical(renderer, &event, &mx, &my);
                if (main_menu_inside(mx, my, single))
                    result = GAME_MODE_SINGLE;
                if (main_menu_inside(mx, my, multi))
                    result = GAME_MODE_MULTI;
            }
        }
        SDL_Delay(16);
    }

    main_menu_destroy_image(&background);
    if (hover != NULL)
        Mix_FreeChunk(hover);
    return result;
}
