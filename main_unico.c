// main_unico.c
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>

#define WIDTH 800
#define HEIGHT 600
#define MENU_HEIGHT 28

int menuSelecionado = -1; // -1 = nenhum menu aberto

// caixas dos menus (preenchidas por computeMenuBoxes)
SDL_Rect menuBoxes[6];

// textos dos menus
const char* menus[] = {"Cartucho", "Tela", "Sistema", "Áudio", "Configuração", "Ajuda"};
const int numMenus = 6;

// dropdown items para cada menu
const char* dropdownItems0[] = {"Inserir Cartucho", "Ejetar", "Info"};
const char* dropdownItems1[] = {"Resolução", "Fullscreen", "Escala"};
const char* dropdownItems2[] = {"Reiniciar", "Salvar Estado", "Carregar Estado"};
const char* dropdownItems3[] = {"Volume", "Mute", "Mixer"};
const char* dropdownItems4[] = {"Vídeo", "Áudio", "Controles", "Sistema"};
const char* dropdownItems5[] = {"Documentação", "Sobre", "Sair"};

const char** allDropdowns[] = {
        dropdownItems0, dropdownItems1, dropdownItems2,
        dropdownItems3, dropdownItems4, dropdownItems5
};

const int dropdownCounts[] = {
        sizeof(dropdownItems0)/sizeof(dropdownItems0[0]),
        sizeof(dropdownItems1)/sizeof(dropdownItems1[0]),
        sizeof(dropdownItems2)/sizeof(dropdownItems2[0]),
        sizeof(dropdownItems3)/sizeof(dropdownItems3[0]),
        sizeof(dropdownItems4)/sizeof(dropdownItems4[0]),
        sizeof(dropdownItems5)/sizeof(dropdownItems5[0])
};

// calcula as caixas dos menus (usa a fonte para medir largura)
void computeMenuBoxes(TTF_Font* font) {
    int x = 10;
    for (int i = 0; i < numMenus; ++i) {
        int w, h;
        TTF_SizeUTF8(font, menus[i], &w, &h);
        menuBoxes[i].x = x;
        menuBoxes[i].y = 0;
        menuBoxes[i].w = w + 40; // padding horizontal
        menuBoxes[i].h = MENU_HEIGHT;
        x += menuBoxes[i].w;
    }
}

// desenha a barra superior e os textos; também faz hover highlight
void drawMenuBar(SDL_Renderer* renderer, TTF_Font* font) {
    SDL_Rect menuBar = {0, 0, WIDTH, MENU_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderFillRect(renderer, &menuBar);

    SDL_Color black = {0, 0, 0, 255};
    int mx, my;
    SDL_GetMouseState(&mx, &my);

    for (int i = 0; i < numMenus; ++i) {
        int x = menuBoxes[i].x;
        int y = menuBoxes[i].y;
        int w = menuBoxes[i].w;
        int h = menuBoxes[i].h;

        // hover highlight (quando mouse sobre a caixa)
        if (mx >= x && mx <= x + w && my >= y && my <= y + h) {
            SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
            SDL_Rect r = {x, y, w, h};
            SDL_RenderFillRect(renderer, &r);
        }

        // renderiza texto centralizado verticalmente
        SDL_Surface* surface = TTF_RenderUTF8_Solid(font, menus[i], black);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        int textY = y + (h - surface->h) / 2;
        SDL_Rect dst = {x + 8, textY, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &dst);

        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }
}

// desenha um dropdown com largura e altura baseados em parâmetros
void drawDropdown(SDL_Renderer* renderer, TTF_Font* font, const char* items[], int numItems, int x, int y, int width) {
    SDL_Color black = {0, 0, 0, 255};
    SDL_Color white = {255, 255, 255, 255};
    int itemHeight = 24;

    int mx, my;
    SDL_GetMouseState(&mx, &my);

    for (int i = 0; i < numItems; i++) {
        SDL_Rect rect = {x, y + i * itemHeight, width, itemHeight};

        // hover highlight para cada item
        int isHover = (mx >= rect.x && mx <= rect.x + rect.w && my >= rect.y && my <= rect.y + rect.h);

        if (isHover) {
            SDL_SetRenderDrawColor(renderer, 120, 120, 120, 255); // destaque do item
        } else {
            SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255); // fundo normal
        }
        SDL_RenderFillRect(renderer, &rect);

        // renderiza texto; cor diferente quando hover
        SDL_Color textColor = isHover ? white : black;
        SDL_Surface* surface = TTF_RenderUTF8_Solid(font, items[i], textColor);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

        SDL_Rect dst = {x + 8, y + 4 + i * itemHeight, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &dst);

        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }
}

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init error: %s", SDL_GetError());
        return 1;
    }
    if (TTF_Init() != 0) {
        SDL_Log("TTF_Init error: %s", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
            "Idle - Gray Sweep RGB + Menu DS",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            WIDTH, HEIGHT,
            SDL_WINDOW_SHOWN
    );
    if (!window) {
        SDL_Log("CreateWindow error: %s", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_Log("CreateRenderer error: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Texture* texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING,
            WIDTH, HEIGHT
    );
    if (!texture) {
        SDL_Log("CreateTexture error: %s", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    Uint32* pixels = malloc(sizeof(Uint32) * WIDTH * HEIGHT);
    if (!pixels) {
        SDL_Log("malloc failed");
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    TTF_Font* font = TTF_OpenFont("fonts/arial.ttf", 18);
    if (!font) {
        SDL_Log("Erro ao carregar fonte!");
        free(pixels);
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    int running = 1;
    SDL_Event event;
    int frame = 0;

    // loop principal
    while (running) {
        // atualiza caixas dos menus (necessário para detectar cliques)
        computeMenuBoxes(font);

        // trata eventos
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                int mx = event.button.x;
                int my = event.button.y;

                // clique dentro da barra de menus?
                if (my >= 0 && my < MENU_HEIGHT) {
                    int clickedMenu = -1;
                    for (int i = 0; i < numMenus; ++i) {
                        SDL_Rect m = menuBoxes[i];
                        if (mx >= m.x && mx <= m.x + m.w) {
                            clickedMenu = i;
                            break;
                        }
                    }
                    if (clickedMenu >= 0) {
                        // toggle: se já aberto, fecha; senão abre
                        if (menuSelecionado == clickedMenu) menuSelecionado = -1;
                        else menuSelecionado = clickedMenu;
                    }
                } else {
                    // clique fora da barra: verificar se clicou dentro do dropdown aberto
                    if (menuSelecionado != -1) {
                        int dx = menuBoxes[menuSelecionado].x;
                        int dy = MENU_HEIGHT;
                        int itemH = 24;
                        int count = dropdownCounts[menuSelecionado];
                        int width = menuBoxes[menuSelecionado].w;
                        if (width < 160) width = 160;
                        SDL_Rect dropRect = {dx, dy, width, itemH * count};

                        if (mx >= dropRect.x && mx <= dropRect.x + dropRect.w &&
                            my >= dropRect.y && my <= dropRect.y + dropRect.h) {
                            // clicou em uma opção do dropdown
                            int idx = (my - dropRect.y) / itemH;
                            if (idx >= 0 && idx < count) {
                                const char* escolha = allDropdowns[menuSelecionado][idx];
                                SDL_Log("Menu %d selecionou: %s", menuSelecionado, escolha);
                                // ação simples: fechar o dropdown após seleção
                                menuSelecionado = -1;
                            }
                        } else {
                            // clique fora do dropdown: fecha
                            menuSelecionado = -1;
                        }
                    }
                }
            }
        }

        // gerar sweep RGB (mesma lógica anterior)
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                int raw = (x + y - frame) % 128;
                if (raw < 0) raw += 128;

                int tri = abs(raw - 64);
                Uint8 v = tri / 4;

                Uint8 r = v, g = v, b = v;
                int colorPhase = (frame / 60) % 3;

                if (tri % 64 == 0) {
                    if (colorPhase == 0) g += v * 6;
                    else if (colorPhase == 1) b += v * 6;
                    else if (colorPhase == 2) r += v * 6;
                }

                pixels[y * WIDTH + x] = (0xFF << 24) | (r << 16) | (g << 8) | b;
            }
        }

        SDL_UpdateTexture(texture, NULL, pixels, WIDTH * sizeof(Uint32));
        SDL_RenderClear(renderer);

        // desenha sweep RGB
        SDL_RenderCopy(renderer, texture, NULL, NULL);

        // desenha barra superior
        drawMenuBar(renderer, font);

        // desenha dropdown se algum menu estiver selecionado
        if (menuSelecionado >= 0 && menuSelecionado < numMenus) {
            int dx = menuBoxes[menuSelecionado].x;
            int dy = MENU_HEIGHT;
            int count = dropdownCounts[menuSelecionado];
            int width = menuBoxes[menuSelecionado].w;
            if (width < 160) width = 160;
            drawDropdown(renderer, font, allDropdowns[menuSelecionado], count, dx, dy, width);
        }

        SDL_RenderPresent(renderer);

        frame++;
        SDL_Delay(16);
    }

    // limpeza final
    TTF_CloseFont(font);
    free(pixels);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
