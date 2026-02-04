// main_unico.c
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>

#define WIDTH 800
#define HEIGHT 600

// Função para desenhar a barra superior
void drawMenuBar(SDL_Renderer* renderer, TTF_Font* font) {
    const char* menus[] = {"Cartucho", "Tela", "Sistema", "Áudio", "Configuração", "Ajuda"};
    int numMenus = 6;

    // barra superior
    SDL_Rect menuBar = {0, 0, WIDTH, 28};
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255); // cinza escuro
    SDL_RenderFillRect(renderer, &menuBar);

    SDL_Color black = {0, 0, 0, 255};


    // desenhar texto
    int x = 10;
    SDL_Color white = {220, 220, 220, 255};
    for (int i = 0; i < numMenus; i++) {
        SDL_Surface* surface = TTF_RenderUTF8_Solid(font, menus[i], black);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect dst = {x, 4, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &dst);
        x += surface->w + 20;
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Window* window = SDL_CreateWindow(
        "Idle - Gray Sweep RGB + Menu DS",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WIDTH, HEIGHT,
        SDL_WINDOW_SHOWN
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        WIDTH, HEIGHT
    );

    Uint32* pixels = malloc(sizeof(Uint32) * WIDTH * HEIGHT);

    // caminho relativo correto para a fonte
    TTF_Font* font = TTF_OpenFont("fonts/arial.ttf", 18);
    if (!font) {
        SDL_Log("Erro ao carregar fonte!");
        return 1;
    }

    int running = 1;
    SDL_Event event;
    int frame = 0;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = 0;
        }

        // gerar sweep RGB
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                int raw = (x + y - frame) % 128;
                if (raw < 0) raw += 128;

                int tri = abs(raw - 64);
                Uint8 v = tri / 4;

                Uint8 r = v, g = v, b = v;
                int colorPhase = (frame / 60) % 3;

                if (tri % 64 == 0) {
                    if (colorPhase == 0) g += v * 6; // verde
                    else if (colorPhase == 1) b += v * 6; // azul
                    else if (colorPhase == 2) r += v * 6; // vermelho
                }

                pixels[y * WIDTH + x] =
                    (0xFF << 24) | (r << 16) | (g << 8) | b;
            }
        }

        SDL_UpdateTexture(texture, NULL, pixels, WIDTH * sizeof(Uint32));

        SDL_RenderClear(renderer);

        // desenha sweep RGB
        SDL_RenderCopy(renderer, texture, NULL, NULL);

        // desenha barra superior
        drawMenuBar(renderer, font);

        SDL_RenderPresent(renderer);

        frame++;
        SDL_Delay(16);
    }

    TTF_CloseFont(font);
    free(pixels);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
