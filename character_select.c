#include "header.h"

static CharacterDefinition characters[] = {
    {
        "Joueur 1",
        {
            {
                "assets/characters/joueur1/orange/idle.png",
                "assets/characters/joueur1/orange/walk.png",
                "assets/characters/joueur1/orange/jump.png",
                "assets/characters/joueur1/orange/fall.png",
                "assets/characters/joueur1/orange/fire.png",
                "assets/characters/joueur1/orange/die.png",
                "assets/characters/joueur1/orange/portrait.png",
                "Joueur 1",
                "Combinaison orange"
            },
            {
                "assets/characters/joueur1/blue/idle.png",
                "assets/characters/joueur1/blue/walk.png",
                "assets/characters/joueur1/blue/jump.png",
                "assets/characters/joueur1/blue/fall.png",
                "assets/characters/joueur1/blue/fire.png",
                "assets/characters/joueur1/blue/die.png",
                "assets/characters/joueur1/blue/portrait.png",
                "Joueur 1",
                "Combinaison bleue"
            }
        }
    },
    {
        "Joueur 2",
        {
            {
                "assets/characters/joueur2/violet/idle.png",
                "assets/characters/joueur2/violet/walk.png",
                "assets/characters/joueur2/violet/jump.png",
                "assets/characters/joueur2/violet/fall.png",
                "assets/characters/joueur2/violet/fire.png",
                "assets/characters/joueur2/violet/die.png",
                "assets/characters/joueur2/violet/portrait.png",
                "Joueur 2",
                "Armure violette"
            },
            {
                "assets/characters/joueur2/green/idle.png",
                "assets/characters/joueur2/green/walk.png",
                "assets/characters/joueur2/green/jump.png",
                "assets/characters/joueur2/green/fall.png",
                "assets/characters/joueur2/green/fire.png",
                "assets/characters/joueur2/green/die.png",
                "assets/characters/joueur2/green/portrait.png",
                "Joueur 2",
                "Armure verte"
            }
        }
    }
};

const CharacterDefinition *getCharacterDefinitions(int *count)
{
    if (count) *count = (int)(sizeof(characters) / sizeof(characters[0]));
    return characters;
}

static SDL_Texture *loadTexture(SDL_Renderer *renderer, const char *path)
{
    SDL_Surface *surface = IMG_Load(path);
    SDL_Texture *texture;
    if (!surface) return NULL;
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

static void drawCenteredText(SDL_Renderer *renderer, TTF_Font *font, const char *text, SDL_Rect area, SDL_Color color)
{
    SDL_Surface *surface;
    SDL_Texture *texture;
    SDL_Rect dst;
    if (!font || !text) return;
    surface = TTF_RenderUTF8_Blended(font, text, color);
    if (!surface) return;
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    dst = (SDL_Rect){area.x + (area.w - surface->w) / 2, area.y + (area.h - surface->h) / 2, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &dst);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

static void fillPanel(SDL_Renderer *renderer, SDL_Rect rect, SDL_Color fill, SDL_Color border, int selected)
{
    SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
    SDL_RenderDrawRect(renderer, &rect);
    if (selected)
    {
        SDL_Rect inner = {rect.x + 4, rect.y + 4, rect.w - 8, rect.h - 8};
        SDL_RenderDrawRect(renderer, &inner);
    }
}

typedef struct
{
    SDL_Texture *cardPanel;
    SDL_Texture *inputPanel;
    SDL_Texture *vsPanel;
    SDL_Texture *infoBar;
    SDL_Texture *button;
    SDL_Texture *buttonPrimary;
} UiAssets;

static void drawImagePanel(SDL_Renderer *renderer, SDL_Texture *texture, SDL_Rect rect, SDL_Color fill, SDL_Color border, int selected)
{
    if (texture)
    {
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        return;
    }
    fillPanel(renderer, rect, fill, border, selected);
}

static void loadUiAssets(SDL_Renderer *renderer, UiAssets *ui)
{
    memset(ui, 0, sizeof(*ui));
    ui->cardPanel = loadTexture(renderer, "assets/ui/card_panel.bmp");
    ui->inputPanel = loadTexture(renderer, "assets/ui/input_panel.bmp");
    ui->vsPanel = loadTexture(renderer, "assets/ui/vs_panel.bmp");
    ui->infoBar = loadTexture(renderer, "assets/ui/info_bar.bmp");
    ui->button = loadTexture(renderer, "assets/ui/button.bmp");
    ui->buttonPrimary = loadTexture(renderer, "assets/ui/button_primary.bmp");
}

static void destroyUiAssets(UiAssets *ui)
{
    if (ui->cardPanel) SDL_DestroyTexture(ui->cardPanel);
    if (ui->inputPanel) SDL_DestroyTexture(ui->inputPanel);
    if (ui->vsPanel) SDL_DestroyTexture(ui->vsPanel);
    if (ui->infoBar) SDL_DestroyTexture(ui->infoBar);
    if (ui->button) SDL_DestroyTexture(ui->button);
    if (ui->buttonPrimary) SDL_DestroyTexture(ui->buttonPrimary);
}

static void drawButton(SDL_Renderer *renderer, TTF_Font *font, UiAssets *ui, const char *label, SDL_Rect rect, int primary)
{
    SDL_Color fill = primary ? (SDL_Color){32, 98, 120, 230} : (SDL_Color){28, 32, 44, 230};
    SDL_Color border = primary ? (SDL_Color){255, 230, 90, 255} : (SDL_Color){110, 170, 190, 255};
    drawImagePanel(renderer, primary ? ui->buttonPrimary : ui->button, rect, fill, border, primary);
    drawCenteredText(renderer, font, label, rect, (SDL_Color){245, 245, 230, 255});
}

static int pointInRect(int x, int y, SDL_Rect rect)
{
    return x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h;
}

static void drawCard(SDL_Renderer *renderer, TTF_Font *font, UiAssets *ui, SDL_Texture *portrait, SDL_Rect rect, const char *playerLabel, const char *name, const char *outfit, const char *menuControls, const char *gameControls)
{
    SDL_Rect portraitDst = {rect.x + (rect.w - 160) / 2, rect.y + 86, 160, 160};
    drawImagePanel(renderer, ui->cardPanel, rect, (SDL_Color){18, 22, 34, 238}, (SDL_Color){255, 230, 90, 255}, 1);
    drawCenteredText(renderer, font, playerLabel, (SDL_Rect){rect.x, rect.y + 14, rect.w, 30}, (SDL_Color){255, 235, 110, 255});
    drawCenteredText(renderer, font, name, (SDL_Rect){rect.x, rect.y + 48, rect.w, 30}, (SDL_Color){240, 245, 245, 255});
    if (portrait) SDL_RenderCopy(renderer, portrait, NULL, &portraitDst);
    drawCenteredText(renderer, font, outfit, (SDL_Rect){rect.x + 10, rect.y + 256, rect.w - 20, 32}, (SDL_Color){220, 232, 240, 255});
    drawImagePanel(renderer, ui->inputPanel, (SDL_Rect){rect.x + 22, rect.y + 304, rect.w - 44, 74}, (SDL_Color){12, 16, 26, 235}, (SDL_Color){80, 145, 170, 255}, 0);
    drawCenteredText(renderer, font, "Input", (SDL_Rect){rect.x + 22, rect.y + 309, rect.w - 44, 20}, (SDL_Color){255, 235, 110, 255});
    drawCenteredText(renderer, font, menuControls, (SDL_Rect){rect.x + 22, rect.y + 334, rect.w - 44, 20}, (SDL_Color){220, 232, 240, 255});
    drawCenteredText(renderer, font, gameControls, (SDL_Rect){rect.x + 22, rect.y + 356, rect.w - 44, 20}, (SDL_Color){180, 214, 226, 255});
}

int runCharacterSelectMenu(SDL_Renderer *renderer, TTF_Font *font, CharacterSelection *selection)
{
    int running = 1;
    int confirmed = 0;
    int p1Char = selection ? selection->p1CharacterIndex : 0;
    int p1Outfit = selection ? selection->p1OutfitIndex : 0;
    int p2Char = selection ? selection->p2CharacterIndex : 1;
    int p2Outfit = selection ? selection->p2OutfitIndex : 0;
    int characterCount = 0;
    const CharacterDefinition *defs = getCharacterDefinitions(&characterCount);
    SDL_Texture *bg = loadTexture(renderer, "assets/ui/character_select_bg.png");
    SDL_Texture *portraits[2][2] = {{NULL, NULL}, {NULL, NULL}};
    UiAssets ui;
    SDL_Rect p1Card = {140, 138, 390, 410};
    SDL_Rect p2Card = {750, 138, 390, 410};
    SDL_Rect p1OutfitArea = {140, 386, 390, 162};
    SDL_Rect p2OutfitArea = {750, 386, 390, 162};
    SDL_Rect backButton = {310, 638, 170, 44};
    SDL_Rect validateButton = {555, 638, 170, 44};
    SDL_Rect startButton = {800, 638, 170, 44};
    SDL_Event e;

    loadUiAssets(renderer, &ui);
    for (int c = 0; c < characterCount; c++)
        for (int o = 0; o < 2; o++)
            portraits[c][o] = loadTexture(renderer, defs[c].outfits[o].portrait);

    while (running)
    {
        stellarMusicUpdateMenu();
        SDL_SetRenderDrawColor(renderer, 12, 15, 24, 255);
        SDL_RenderClear(renderer);
        if (bg) SDL_RenderCopy(renderer, bg, NULL, NULL);

        drawCenteredText(renderer, font, "Choix du personnage", (SDL_Rect){0, 34, SCREEN_W, 40}, (SDL_Color){255, 236, 120, 255});
        drawCenteredText(renderer, font, "Chaque joueur modifie uniquement sa propre fenetre de selection.", (SDL_Rect){0, 82, SCREEN_W, 28}, (SDL_Color){204, 222, 232, 255});

        drawCard(renderer, font, &ui, portraits[p1Char][p1Outfit], p1Card, "Joueur 1", defs[p1Char].name, defs[p1Char].outfits[p1Outfit].outfitName, "A/D personnage, W/S tenue", "Clic haut: perso, bas: tenue");
        drawCard(renderer, font, &ui, portraits[p2Char][p2Outfit], p2Card, "Joueur 2", defs[p2Char].name, defs[p2Char].outfits[p2Outfit].outfitName, "Fleches personnage/tenue", "Clic haut: perso, bas: tenue");

        drawImagePanel(renderer, ui.vsPanel, (SDL_Rect){548, 238, 184, 134}, (SDL_Color){14, 18, 28, 238}, (SDL_Color){90, 180, 205, 255}, 0);
        drawCenteredText(renderer, font, "VS", (SDL_Rect){548, 278, 184, 48}, (SDL_Color){255, 235, 110, 255});

        drawImagePanel(renderer, ui.infoBar, (SDL_Rect){120, 570, 1040, 54}, (SDL_Color){16, 20, 31, 235}, (SDL_Color){86, 140, 160, 255}, 0);
        drawCenteredText(renderer, font, "Entree / Espace ou clic Commencer : lancer    |    Echap ou Retour : quitter", (SDL_Rect){120, 583, 1040, 28}, (SDL_Color){220, 232, 240, 255});

        drawButton(renderer, font, &ui, "Retour", backButton, 0);
        drawButton(renderer, font, &ui, "Valider", validateButton, 1);
        drawButton(renderer, font, &ui, "Commencer", startButton, 1);

        SDL_RenderPresent(renderer);

        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT) running = 0;
            if (e.type == SDL_KEYDOWN)
            {
                SDL_Scancode key = e.key.keysym.scancode;
                if (key == SDL_SCANCODE_ESCAPE) running = 0;
                else if (key == SDL_SCANCODE_A || key == SDL_SCANCODE_D)
                    p1Char = (p1Char + 1) % characterCount;
                else if (key == SDL_SCANCODE_W || key == SDL_SCANCODE_S)
                    p1Outfit = (p1Outfit + 1) % 2;
                else if (key == SDL_SCANCODE_LEFT || key == SDL_SCANCODE_RIGHT)
                    p2Char = (p2Char + 1) % characterCount;
                else if (key == SDL_SCANCODE_UP || key == SDL_SCANCODE_DOWN)
                    p2Outfit = (p2Outfit + 1) % 2;
                else if (key == SDL_SCANCODE_RETURN || key == SDL_SCANCODE_SPACE)
                {
                    confirmed = 1;
                    running = 0;
                }
            }
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
            {
                int x = e.button.x;
                int y = e.button.y;

                if (pointInRect(x, y, backButton))
                {
                    running = 0;
                }
                else if (pointInRect(x, y, validateButton) || pointInRect(x, y, startButton))
                {
                    confirmed = 1;
                    running = 0;
                }
                else if (pointInRect(x, y, p1OutfitArea))
                {
                    p1Outfit = (p1Outfit + 1) % 2;
                }
                else if (pointInRect(x, y, p2OutfitArea))
                {
                    p2Outfit = (p2Outfit + 1) % 2;
                }
                else if (pointInRect(x, y, p1Card))
                {
                    p1Char = (p1Char + 1) % characterCount;
                }
                else if (pointInRect(x, y, p2Card))
                {
                    p2Char = (p2Char + 1) % characterCount;
                }
            }
        }

        SDL_Delay(16);
    }

    for (int c = 0; c < characterCount; c++)
        for (int o = 0; o < 2; o++)
            if (portraits[c][o]) SDL_DestroyTexture(portraits[c][o]);
    destroyUiAssets(&ui);
    if (bg) SDL_DestroyTexture(bg);

    if (confirmed && selection)
    {
        selection->p1CharacterIndex = p1Char;
        selection->p1OutfitIndex = p1Outfit;
        selection->p2CharacterIndex = p2Char;
        selection->p2OutfitIndex = p2Outfit;
    }
    return confirmed;
}
