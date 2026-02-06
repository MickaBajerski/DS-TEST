// idle_sweep_only.c
// Janela SDL2 com idle sweep RGB (Dark / Light)
// Clique do mouse alterna o tema
// Extraído diretamente do main_unico.c preservando o efeito original

#include <SDL2/SDL.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 600

// -------------------- util --------------------

static inline float lerp_f(float a, float b, float t) {
    return a + (b - a) * t;
}

static inline float smoothstep_f(float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

static Uint8 fcol_to_u8(float v) {
    int iv = (int)(v * 255.0f + 0.5f);
    if (iv < 0) iv = 0;
    if (iv > 255) iv = 255;
    return (Uint8)iv;
}

// -------------------- theme --------------------

typedef struct {
    float r,g,b,a;
} FColor;

typedef struct {
    const char* name;
    FColor accent;
    float idle_gain;
    float idle_intensity;
} Theme;

static Theme darkTheme = {
        "Dark",
        {0.43f, 0.70f, 1.0f, 1.0f},
        1.6f,
        0.7f
};

static Theme lightTheme = {
        "Light",
        {0.20f, 0.48f, 0.72f, 1.0f},
        3.0f,
        1.0f
};

static Theme currentTheme;

// -------------------- color animation --------------------

typedef struct {
    float sr, sg, sb;
    float tr, tg, tb;
    float t;
    float duration;
} ColorAnim;

#define NUM_COLOR_ANIMS 3
static ColorAnim colorAnims[NUM_COLOR_ANIMS];

static void pick_new_target(ColorAnim* ca) {
    ca->tr = 0.15f + ((float)rand() / RAND_MAX) * 0.85f;
    ca->tg = 0.15f + ((float)rand() / RAND_MAX) * 0.85f;
    ca->tb = 0.15f + ((float)rand() / RAND_MAX) * 0.85f;
    ca->t = 0.0f;
}

static void init_color_anims(float duration) {
    for (int i = 0; i < NUM_COLOR_ANIMS; i++) {
        colorAnims[i].sr = colorAnims[i].sg = colorAnims[i].sb = 0.5f;
        colorAnims[i].duration = duration;
        pick_new_target(&colorAnims[i]);
    }
}

static void update_color_anims(float delta) {
    for (int i = 0; i < NUM_COLOR_ANIMS; i++) {
        ColorAnim* ca = &colorAnims[i];
        ca->t += delta;
        if (ca->t >= ca->duration) {
            float leftover = ca->t - ca->duration;
            ca->sr = ca->tr;
            ca->sg = ca->tg;
            ca->sb = ca->tb;
            pick_new_target(ca);
            ca->t = leftover;
            if (ca->t > ca->duration) ca->t = ca->duration;
        }
    }
}

static void get_anim_color(const ColorAnim* ca, float out[3]) {
    float tt = ca->t / ca->duration;
    float e = smoothstep_f(tt);
    out[0] = lerp_f(ca->sr, ca->tr, e);
    out[1] = lerp_f(ca->sg, ca->tg, e);
    out[2] = lerp_f(ca->sb, ca->tb, e);
}

// -------------------- main --------------------

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    srand((unsigned)time(NULL));

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow(
            "Idle Sweep RGB",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            DEFAULT_WIDTH,
            DEFAULT_HEIGHT,
            SDL_WINDOW_RESIZABLE
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(
            window, -1, SDL_RENDERER_ACCELERATED
    );

    int win_w = DEFAULT_WIDTH;
    int win_h = DEFAULT_HEIGHT;

    SDL_Texture* texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING,
            win_w,
            win_h
    );

    Uint32* pixels = malloc(sizeof(Uint32) * win_w * win_h);

    currentTheme = darkTheme;
    init_color_anims(3.0f);

    Uint32 last_time = SDL_GetTicks();
    int running = 1;

    while (running) {
        Uint32 now = SDL_GetTicks();
        float delta = (now - last_time) / 1000.0f;
        if (delta > 0.1f) delta = 0.1f;
        last_time = now;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                currentTheme = (currentTheme.name == darkTheme.name)
                               ? lightTheme
                               : darkTheme;
            }
            if (e.type == SDL_WINDOWEVENT &&
                e.window.event == SDL_WINDOWEVENT_RESIZED) {
                win_w = e.window.data1;
                win_h = e.window.data2;
                free(pixels);
                SDL_DestroyTexture(texture);
                texture = SDL_CreateTexture(
                        renderer,
                        SDL_PIXELFORMAT_ARGB8888,
                        SDL_TEXTUREACCESS_STREAMING,
                        win_w,
                        win_h
                );
                pixels = malloc(sizeof(Uint32) * win_w * win_h);
            }
        }

        update_color_anims(delta);

        float animR[3], animG[3], animB[3];

        if (currentTheme.name == darkTheme.name) {
            get_anim_color(&colorAnims[2], animR);
            get_anim_color(&colorAnims[1], animG);
            get_anim_color(&colorAnims[0], animB);
        } else {
            get_anim_color(&colorAnims[0], animR);
            get_anim_color(&colorAnims[1], animG);
            get_anim_color(&colorAnims[2], animB);
        }

        float ar = currentTheme.accent.r;
        float ag = currentTheme.accent.g;
        float ab = currentTheme.accent.b;

        for (int y = 0; y < win_h; y++) {
            float fy = (float)y / (float)(win_h - 1);
            for (int x = 0; x < win_w; x++) {
                float fx = (float)x / (float)(win_w - 1);

                float v0 = 0.5f + 0.5f * sinf((fx + animR[0]*0.5f + fy*0.3f) * 6.2831853f + animR[1]*2.0f);
                float v1 = 0.5f + 0.5f * sinf((fx*1.2f + animG[1]*0.6f + fy*0.2f) * 6.2831853f + animG[2]*1.5f);
                float v2 = 0.5f + 0.5f * sinf((fx*0.8f + animB[2]*0.4f + fy*0.1f) * 6.2831853f + animB[0]*2.2f);

                float rr = (v0 * animR[0] + v1 * animG[0] + v2 * animB[0]) / 3.0f;
                float gg = (v0 * animR[1] + v1 * animG[1] + v2 * animB[1]) / 3.0f;
                float bb = (v0 * animR[2] + v1 * animG[2] + v2 * animB[2]) / 3.0f;

                rr = lerp_f(rr, ar, 0.18f) * currentTheme.idle_intensity;
                gg = lerp_f(gg, ag, 0.18f) * currentTheme.idle_intensity;
                bb = lerp_f(bb, ab, 0.18f) * currentTheme.idle_intensity;

                float pulse = (0.5f + 0.5f * sinf((fx + fy) * 12.0f + animR[0]*6.0f))
                              * 0.06f * currentTheme.idle_gain;

                rr += pulse;
                gg += pulse;
                bb += pulse;

                pixels[y * win_w + x] =
                        (255 << 24) |
                        (fcol_to_u8(rr) << 16) |
                        (fcol_to_u8(gg) << 8) |
                        (fcol_to_u8(bb));
            }
        }

        SDL_UpdateTexture(texture, NULL, pixels, win_w * sizeof(Uint32));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        SDL_Delay(8);
    }

    free(pixels);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
