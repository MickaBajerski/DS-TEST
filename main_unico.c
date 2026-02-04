// main_unico.c
/* TODO: ajustar edge-case de dropdowns encostados nas bordas quando janela muito estreita.
   Prioridade baixa; manter comportamento atual até revisão visual. */

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 600
#define MENU_HEIGHT 28

// Constantes centralizadas
static const int RIGHT_RESERVED = 120;
static const int MENU_PADDING = 40;
static const int MENU_MIN_WIDTH = 40;
static const int DROPDOWN_MIN_WIDTH = 160;
static const int EDGE_MARGIN = 8;
static const int DROPDOWN_ITEM_HEIGHT = 24;
static const int VOLUME_SUB_WIDTH = 120;
static const int CLOSE_BTN_SIZE = 24

; // small formatting helper

int menuSelecionado = -1; // -1 = nenhum menu aberto

SDL_Rect menuBoxes[6];

const char* menus[] = {"Cartucho", "Tela", "Sistema", "Áudio", "Configuração", "Ajuda"};
const int numMenus = 6;

const char* dropdownItems0[] = {"Inserir Cartucho", "Ejetar", "Info", "Sair"};
const char* dropdownItems1[] = {"Resolução", "Fullscreen", "Escala"};
const char* dropdownItems2[] = {"Reiniciar", "Salvar Estado", "Carregar Estado"};
const char* dropdownItems3[] = {"Volume", "Mute", "Mixer"};
const char* dropdownItems4[] = {"Vídeo", "Áudio", "Controles", "Sistema"};
const char* dropdownItems5[] = {"Documentação", "Sobre"};

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

const char* volumeItems[] = {
        "0%", "10%", "20%", "30%", "40%", "50%", "60%", "70%", "80%", "90%", "100%"
};
const int volumeCount = sizeof(volumeItems)/sizeof(volumeItems[0]);

int volumeDropdownOpen = 0;
int currentVolume = 100;
int muted = 0;

typedef struct {
    int open;
    char title[128];
    SDL_Rect rect;
    SDL_Rect closeBtn;
} Modal;

Modal modal = {0};

// variáveis de janela/recursos
int win_w = DEFAULT_WIDTH;
int win_h = DEFAULT_HEIGHT;
SDL_Texture* texture = NULL;
Uint32* pixels = NULL;

// HiDPI / drawable size
int drawable_w = DEFAULT_WIDTH;
int drawable_h = DEFAULT_HEIGHT;

// utilitários
static int clamp_int(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

static int calc_draw_x(int desired_x, int width) {
    int right_limit = win_w - EDGE_MARGIN;
    if (desired_x + width > right_limit) desired_x = right_limit - width;
    return clamp_int(desired_x, EDGE_MARGIN, right_limit - width);
}

// recria texture e buffer usando tamanho drawable (HiDPI aware)
int recreateTextureAndBufferFromWindow(SDL_Window* window, SDL_Renderer* renderer) {
    if (!window || !renderer) return 0;

    // obter tamanho da janela (UI coords) e tamanho do drawable (pixels)
    SDL_GetWindowSize(window, &win_w, &win_h);
    if (SDL_GetRendererOutputSize(renderer, &drawable_w, &drawable_h) != 0) {
        // fallback: use window size
        drawable_w = win_w;
        drawable_h = win_h;
    }

    if (drawable_w <= 0 || drawable_h <= 0) return 0;

    if (texture) { SDL_DestroyTexture(texture); texture = NULL; }
    if (pixels) { free(pixels); pixels = NULL; }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, drawable_w, drawable_h);
    if (!texture) {
        SDL_Log("CreateTexture failed: %s", SDL_GetError());
        return 0;
    }

    pixels = malloc(sizeof(Uint32) * drawable_w * drawable_h);
    if (!pixels) {
        SDL_Log("malloc failed for pixels");
        SDL_DestroyTexture(texture);
        texture = NULL;
        return 0;
    }

    return 1;
}

// computeMenuBoxes: windowed uses fixed spacing; fullscreen uses responsive.
// Both modes reserve space at right for volume indicator and clamp menus to available area.
void computeMenuBoxes(TTF_Font* font, int is_fullscreen) {
    int margin_left = 10;
    int margin_right = 12;
    int right_reserved = RIGHT_RESERVED;
    int available_width = win_w - right_reserved - margin_left - margin_right;
    if (available_width < 200) available_width = win_w - margin_left - margin_right;

    if (!is_fullscreen) {
        int x = margin_left;
        for (int i = 0; i < numMenus; ++i) {
            int w = 0, h = 0;
            if (TTF_SizeUTF8(font, menus[i], &w, &h) != 0) {
                w = 50; // fallback
            }
            int boxw = w + MENU_PADDING;
            if (x + boxw > available_width) {
                boxw = available_width - x;
                if (boxw < MENU_MIN_WIDTH) boxw = MENU_MIN_WIDTH;
            }
            menuBoxes[i].x = x;
            menuBoxes[i].y = 0;
            menuBoxes[i].w = boxw;
            menuBoxes[i].h = MENU_HEIGHT;
            x += menuBoxes[i].w;
        }
        return;
    }

    int totalTextW = 0;
    int textWidths[numMenus];
    int textH;
    for (int i = 0; i < numMenus; ++i) {
        if (TTF_SizeUTF8(font, menus[i], &textWidths[i], &textH) != 0) textWidths[i] = 50;
        totalTextW += textWidths[i];
    }

    int spacing = 40;
    if (numMenus > 1) {
        spacing = (available_width - totalTextW) / (numMenus - 1);
        if (spacing > 60) spacing = 60;
        if (spacing < 8) spacing = 8;
    }

    int x = margin_left;
    for (int i = 0; i < numMenus; ++i) {
        int boxw = textWidths[i] + 16;
        if (x + boxw > available_width) {
            boxw = available_width - x;
            if (boxw < MENU_MIN_WIDTH) boxw = MENU_MIN_WIDTH;
        }
        menuBoxes[i].x = x;
        menuBoxes[i].y = 0;
        menuBoxes[i].w = boxw;
        menuBoxes[i].h = MENU_HEIGHT;
        x += menuBoxes[i].w + spacing;
    }
}

void drawMenuBar(SDL_Renderer* renderer, TTF_Font* font) {
    SDL_Rect menuBar = {0, 0, win_w, MENU_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderFillRect(renderer, &menuBar);

    SDL_Color black = {0,0,0,255};
    int mx, my;
    SDL_GetMouseState(&mx, &my);

    for (int i = 0; i < numMenus; ++i) {
        int x = menuBoxes[i].x;
        int y = menuBoxes[i].y;
        int w = menuBoxes[i].w;
        int h = menuBoxes[i].h;

        if (mx >= x && mx <= x + w && my >= y && my <= y + h) {
            SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
            SDL_Rect r = {x, y, w, h};
            SDL_RenderFillRect(renderer, &r);
        }

        SDL_Surface* surface = TTF_RenderUTF8_Solid(font, menus[i], black);
        if (!surface) {
            SDL_Log("TTF_RenderUTF8_Solid failed: %s", TTF_GetError());
            continue;
        }
        SDL_Texture* textureText = SDL_CreateTextureFromSurface(renderer, surface);
        if (!textureText) {
            SDL_Log("CreateTextureFromSurface failed: %s", SDL_GetError());
            SDL_FreeSurface(surface);
            continue;
        }
        int textY = y + (h - surface->h) / 2;
        SDL_Rect dst = {x + 8, textY, surface->w, surface->h};
        SDL_RenderCopy(renderer, textureText, NULL, &dst);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(textureText);
    }
}

// drawDropdown: adjusts x so dropdown never draws off-screen; clamps left/right.
void drawDropdown(SDL_Renderer* renderer, TTF_Font* font, const char* items[], int numItems, int x, int y, int width) {
    if (!items || numItems <= 0) return;
    SDL_Color black = {0,0,0,255};
    SDL_Color white = {255,255,255,255};
    int itemHeight = DROPDOWN_ITEM_HEIGHT;

    // adjust x to keep dropdown inside window
    x = calc_draw_x(x, width);

    int mx, my;
    SDL_GetMouseState(&mx, &my);

    for (int i = 0; i < numItems; ++i) {
        SDL_Rect rect = {x, y + i * itemHeight, width, itemHeight};
        int isHover = (mx >= rect.x && mx <= rect.x + rect.w && my >= rect.y && my <= rect.y + rect.h);
        if (isHover) SDL_SetRenderDrawColor(renderer, 120,120,120,255);
        else SDL_SetRenderDrawColor(renderer, 80,80,80,255);
        SDL_RenderFillRect(renderer, &rect);

        SDL_Color textColor = isHover ? white : black;
        SDL_Surface* surface = TTF_RenderUTF8_Solid(font, items[i], textColor);
        if (!surface) {
            SDL_Log("TTF_RenderUTF8_Solid failed for dropdown item: %s", TTF_GetError());
            continue;
        }
        SDL_Texture* textureText = SDL_CreateTextureFromSurface(renderer, surface);
        if (!textureText) {
            SDL_Log("CreateTextureFromSurface failed: %s", SDL_GetError());
            SDL_FreeSurface(surface);
            continue;
        }
        SDL_Rect dst = {x + 8, y + 4 + i * itemHeight, surface->w, surface->h};
        SDL_RenderCopy(renderer, textureText, NULL, &dst);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(textureText);
    }
}

void drawModal(SDL_Renderer* renderer, TTF_Font* font, Modal* m) {
    if (!m->open) return;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0,0,0,120);
    SDL_Rect overlay = {0,0,win_w,win_h};
    SDL_RenderFillRect(renderer, &overlay);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    SDL_SetRenderDrawColor(renderer, 220,220,220,255);
    SDL_RenderFillRect(renderer, &m->rect);
    SDL_SetRenderDrawColor(renderer, 80,80,80,255);
    SDL_RenderDrawRect(renderer, &m->rect);

    SDL_Color black = {0,0,0,255};
    SDL_Surface* sTitle = TTF_RenderUTF8_Solid(font, m->title, black);
    if (sTitle) {
        SDL_Texture* tTitle = SDL_CreateTextureFromSurface(renderer, sTitle);
        if (tTitle) {
            int titleY = m->rect.y + 8;
            SDL_Rect dstTitle = {m->rect.x + 12, titleY, sTitle->w, sTitle->h};
            SDL_RenderCopy(renderer, tTitle, NULL, &dstTitle);
            SDL_DestroyTexture(tTitle);
        } else {
            SDL_Log("CreateTextureFromSurface failed for modal title: %s", SDL_GetError());
        }
        SDL_FreeSurface(sTitle);
    } else {
        SDL_Log("TTF_RenderUTF8_Solid failed for modal title: %s", TTF_GetError());
    }

    SDL_SetRenderDrawColor(renderer, 200,80,80,255);
    SDL_RenderFillRect(renderer, &m->closeBtn);
    SDL_SetRenderDrawColor(renderer, 255,255,255,255);
    int cx = m->closeBtn.x + 4;
    int cy = m->closeBtn.y + 4;
    int cw = m->closeBtn.w - 8;
    int ch = m->closeBtn.h - 8;
    SDL_RenderDrawLine(renderer, cx, cy, cx + cw, cy + ch);
    SDL_RenderDrawLine(renderer, cx + cw, cy, cx, cy + ch);

    const char* placeholder = "Conteúdo da janela (substituir depois)";
    SDL_Surface* sCont = TTF_RenderUTF8_Solid(font, placeholder, black);
    if (sCont) {
        SDL_Texture* tCont = SDL_CreateTextureFromSurface(renderer, sCont);
        if (tCont) {
            SDL_Rect dstCont = {m->rect.x + 12, m->rect.y + 40, sCont->w, sCont->h};
            SDL_RenderCopy(renderer, tCont, NULL, &dstCont);
            SDL_DestroyTexture(tCont);
        }
        SDL_FreeSurface(sCont);
    }
}

void openModalWithTitle(Modal* m, const char* title) {
    m->open = 1;
    strncpy(m->title, title, sizeof(m->title)-1);
    m->title[sizeof(m->title)-1] = '\0';
    int w = win_w * 60 / 100;
    int h = win_h * 35 / 100;
    if (w < 320) w = 320;
    if (h < 160) h = 160;
    m->rect.x = (win_w - w) / 2;
    m->rect.y = (win_h - h) / 2;
    m->rect.w = w; m->rect.h = h;
    m->closeBtn.w = CLOSE_BTN_SIZE; m->closeBtn.h = CLOSE_BTN_SIZE;
    m->closeBtn.x = m->rect.x + m->rect.w - m->closeBtn.w - 8;
    m->closeBtn.y = m->rect.y + 8;
}

int isPointInRect(int px, int py, SDL_Rect* r) {
    return (px >= r->x && px <= r->x + r->w && py >= r->y && py <= r->y + r->h);
}

void toggleFullscreen(SDL_Window* window) {
    Uint32 flags = SDL_GetWindowFlags(window);
    if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
        SDL_SetWindowFullscreen(window, 0);
        SDL_Log("Fullscreen off");
    } else {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        SDL_Log("Fullscreen on");
    }
}

void drawVolumeIndicator(SDL_Renderer* renderer, TTF_Font* font) {
    char buf[64];
    if (muted) snprintf(buf, sizeof(buf), "Muted");
    else snprintf(buf, sizeof(buf), "Vol: %d%%", currentVolume);
    SDL_Color black = {0,0,0,255};
    SDL_Surface* s = TTF_RenderUTF8_Solid(font, buf, black);
    if (!s) {
        SDL_Log("TTF_RenderUTF8_Solid failed for volume indicator: %s", TTF_GetError());
        return;
    }
    SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, s);
    if (!t) {
        SDL_Log("CreateTextureFromSurface failed for volume indicator: %s", SDL_GetError());
        SDL_FreeSurface(s);
        return;
    }
    int right_margin = 12;
    SDL_Rect dst = {win_w - s->w - right_margin, 6, s->w, s->h};
    SDL_RenderCopy(renderer, t, NULL, &dst);
    SDL_FreeSurface(s);
    SDL_DestroyTexture(t);
}

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { SDL_Log("SDL_Init error: %s", SDL_GetError()); return 1; }
    if (TTF_Init() != 0) { SDL_Log("TTF_Init error: %s", TTF_GetError()); SDL_Quit(); return 1; }

    SDL_Window* window = SDL_CreateWindow("Idle - Gray Sweep RGB + Menu DS",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, DEFAULT_WIDTH, DEFAULT_HEIGHT,
                                          SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) { SDL_Log("CreateWindow error: %s", SDL_GetError()); TTF_Quit(); SDL_Quit(); return 1; }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) { SDL_Log("CreateRenderer error: %s", SDL_GetError()); SDL_DestroyWindow(window); TTF_Quit(); SDL_Quit(); return 1; }

    if (!recreateTextureAndBufferFromWindow(window, renderer)) {
        SDL_Log("Falha ao criar texture/buffer"); SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); TTF_Quit(); SDL_Quit(); return 1;
    }

    TTF_Font* font = TTF_OpenFont("fonts/arial.ttf", 18);
    if (!font) { SDL_Log("Erro ao carregar fonte: %s", TTF_GetError()); free(pixels); SDL_DestroyTexture(texture); SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); TTF_Quit(); SDL_Quit(); return 1; }

    int running = 1;
    SDL_Event event;
    int frame = 0;

    // track fullscreen and last known size to avoid recreate loop
    int prev_fullscreen = 0;
    int last_w = win_w;
    int last_h = win_h;
    int need_recreate = 0;

    while (running) {
        Uint32 flags = SDL_GetWindowFlags(window);
        int is_fullscreen = (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) ? 1 : 0;

        // compute menu boxes using fullscreen flag
        computeMenuBoxes(font, is_fullscreen);

        // process events; mark need_recreate when size changes
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) { running = 0; }
            else if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    int new_w = event.window.data1, new_h = event.window.data2;
                    if (new_w != last_w || new_h != last_h) {
                        last_w = new_w; last_h = new_h;
                        need_recreate = 1;
                    }
                }
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) { if (modal.open) modal.open = 0; else running = 0; }
            } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                int mx = event.button.x, my = event.button.y;

                if (modal.open) {
                    if (isPointInRect(mx, my, &modal.closeBtn)) modal.open = 0;
                    else if (!isPointInRect(mx, my, &modal.rect)) modal.open = 0;
                    continue;
                }

                if (volumeDropdownOpen) {
                    int dx = menuBoxes[3].x;
                    int dy = MENU_HEIGHT;
                    int width = menuBoxes[3].w; if (width < DROPDOWN_MIN_WIDTH) width = DROPDOWN_MIN_WIDTH;
                    int vdx = dx + width;
                    int vdy = dy;
                    int vwidth = VOLUME_SUB_WIDTH;
                    if (vdx + vwidth > win_w - EDGE_MARGIN) vdx = dx - vwidth;
                    if (vdx < EDGE_MARGIN) vdx = EDGE_MARGIN;
                    SDL_Rect vRect = {vdx, vdy, vwidth, DROPDOWN_ITEM_HEIGHT * volumeCount};

                    if (mx >= vRect.x && mx <= vRect.x + vRect.w && my >= vRect.y && my <= vRect.y + vRect.h) {
                        int idx = (my - vRect.y) / DROPDOWN_ITEM_HEIGHT;
                        if (idx >= 0 && idx < volumeCount) { currentVolume = idx * 10; muted = 0; SDL_Log("Volume set to %d%%", currentVolume); volumeDropdownOpen = 0; }
                        continue;
                    } else { volumeDropdownOpen = 0; }
                }

                if (my >= 0 && my < MENU_HEIGHT) {
                    int clickedMenu = -1;
                    for (int i = 0; i < numMenus; ++i) {
                        SDL_Rect m = menuBoxes[i];
                        if (mx >= m.x && mx <= m.x + m.w) { clickedMenu = i; break; }
                    }
                    if (clickedMenu >= 0) {
                        if (menuSelecionado == clickedMenu) { menuSelecionado = -1; volumeDropdownOpen = 0; }
                        else { menuSelecionado = clickedMenu; volumeDropdownOpen = 0; }
                    }
                } else {
                    if (menuSelecionado != -1) {
                        int dx = menuBoxes[menuSelecionado].x;
                        int dy = MENU_HEIGHT;
                        int itemH = DROPDOWN_ITEM_HEIGHT;
                        int count = dropdownCounts[menuSelecionado];
                        int width = menuBoxes[menuSelecionado].w; if (width < DROPDOWN_MIN_WIDTH) width = DROPDOWN_MIN_WIDTH;

                        int draw_dx = calc_draw_x(dx, width);

                        SDL_Rect dropRect = {draw_dx, dy, width, itemH * count};

                        if (count > 0 && mx >= dropRect.x && mx <= dropRect.x + dropRect.w && my >= dropRect.y && my <= dropRect.y + dropRect.h) {
                            int idx = (my - dropRect.y) / itemH;
                            if (idx >= 0 && idx < count) {
                                const char* escolha = allDropdowns[menuSelecionado][idx];

                                if (menuSelecionado == 0) {
                                    if (strcmp(escolha, "Sair") == 0) running = 0;
                                    else if (strcmp(escolha, "Ejetar") == 0) SDL_Log("Ação: Ejetar cartucho");
                                    else openModalWithTitle(&modal, escolha);
                                } else if (menuSelecionado == 1) {
                                    if (strcmp(escolha, "Fullscreen") == 0) {
                                        toggleFullscreen(window);
                                    } else openModalWithTitle(&modal, escolha);
                                } else if (menuSelecionado == 2) {
                                    if (strcmp(escolha, "Reiniciar") == 0) SDL_Log("Ação: Reiniciar sistema (placeholder)");
                                    else if (strcmp(escolha, "Salvar Estado") == 0) SDL_Log("Ação: Salvar estado (placeholder)");
                                    else if (strcmp(escolha, "Carregar Estado") == 0) SDL_Log("Ação: Carregar estado (placeholder)");
                                } else if (menuSelecionado == 3) {
                                    if (strcmp(escolha, "Volume") == 0) volumeDropdownOpen = 1;
                                    else if (strcmp(escolha, "Mute") == 0) { muted = !muted; SDL_Log("Mute toggled: %d", muted); }
                                    else openModalWithTitle(&modal, escolha);
                                } else if (menuSelecionado == 4) openModalWithTitle(&modal, escolha);
                                else if (menuSelecionado == 5) openModalWithTitle(&modal, escolha);

                                if (!(menuSelecionado == 3 && strcmp(escolha, "Volume") == 0)) menuSelecionado = -1;
                            }
                        } else {
                            menuSelecionado = -1;
                            volumeDropdownOpen = 0;
                        }
                    }
                }
            }
        }

        // detect fullscreen change and mark recreate only if size actually changed
        {
            Uint32 flags_now = SDL_GetWindowFlags(window);
            int now_fullscreen = (flags_now & SDL_WINDOW_FULLSCREEN_DESKTOP) ? 1 : 0;
            if (now_fullscreen != prev_fullscreen) {
                int w, h; SDL_GetWindowSize(window, &w, &h);
                if (w != last_w || h != last_h) {
                    last_w = w; last_h = h;
                    need_recreate = 1;
                }
                prev_fullscreen = now_fullscreen;
            }
        }

        // perform recreate once per loop if needed
        if (need_recreate) {
            if (!recreateTextureAndBufferFromWindow(window, renderer)) {
                SDL_Log("Falha ao recriar texture/buffer");
            }
            need_recreate = 0;
        }

        // gerar sweep RGB (preenche drawable buffer)
        for (int y = 0; y < drawable_h; y++) {
            for (int x = 0; x < drawable_w; x++) {
                int raw = (x + y - frame) % 128; if (raw < 0) raw += 128;
                int tri = abs(raw - 64); Uint8 v = tri / 4;
                Uint8 r = v, g = v, b = v;
                int colorPhase = (frame / 60) % 3;
                if (tri % 64 == 0) {
                    if (colorPhase == 0) g += v * 6;
                    else if (colorPhase == 1) b += v * 6;
                    else if (colorPhase == 2) r += v * 6;
                }
                pixels[y * drawable_w + x] = (0xFF << 24) | (r << 16) | (g << 8) | b;
            }
        }

        // atualizar texture com drawable buffer
        SDL_UpdateTexture(texture, NULL, pixels, drawable_w * sizeof(Uint32));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);

        drawMenuBar(renderer, font);

        if (menuSelecionado >= 0 && menuSelecionado < numMenus) {
            int dx = menuBoxes[menuSelecionado].x;
            int dy = MENU_HEIGHT;
            int count = dropdownCounts[menuSelecionado];
            int width = menuBoxes[menuSelecionado].w; if (width < DROPDOWN_MIN_WIDTH) width = DROPDOWN_MIN_WIDTH;

            int draw_dx = calc_draw_x(dx, width);
            drawDropdown(renderer, font, allDropdowns[menuSelecionado], count, draw_dx, dy, width);

            if (menuSelecionado == 3 && volumeDropdownOpen) {
                int vdx = draw_dx + width;
                int vdy = dy;
                int vwidth = VOLUME_SUB_WIDTH;
                if (vdx + vwidth > win_w - EDGE_MARGIN) vdx = draw_dx - vwidth;
                if (vdx < EDGE_MARGIN) vdx = EDGE_MARGIN;
                drawDropdown(renderer, font, volumeItems, volumeCount, vdx, vdy, vwidth);
            }
        }

        drawModal(renderer, font, &modal);

        drawVolumeIndicator(renderer, font);

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
