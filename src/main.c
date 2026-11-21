#include "app.h"
#include <stdio.h>
#include <string.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif
static App game;
static Uint64 previous;
static void frame(void) {
    Uint64 now = SDL_GetTicksNS();
    double elapsed = (double)(now - previous) * 1.e-9;
    previous = now;
#ifdef __EMSCRIPTEN__
    double w, h;
    emscripten_get_element_css_size("#canvas", &w, &h);
    int sw, sh;
    SDL_GetWindowSize(game.window, &sw, &sh);
    if (w > 1 && h > 1 && (sw != (int)w || sh != (int)h))
        SDL_SetWindowSize(game.window, (int)w, (int)h);
#endif
    app_frame(&game, elapsed);
}
int orra_run(int argc, char **argv) {
#ifdef __EMSCRIPTEN__
    const char *root = ".";
#else
    const char *root = SDL_GetBasePath();
#endif
    const char *route = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--help")) {
            puts("1stprts | Gate Six\nWASD move, mouse or arrows look, Tab orders, E board, F1 field "
                 "guide.");
            return 0;
        }
        if (!strcmp(argv[i], "--data") && i + 1 < argc)
            root = argv[++i];
        else if (!strcmp(argv[i], "--replay") && i + 1 < argc)
            route = argv[++i];
        else {
            fprintf(stderr, "Usage: 1stprts [--data directory] [--replay route]\n");
            return 2;
        }
    }
    if (!app_open(&game, root ? root : ".", route != NULL)) {
        fprintf(stderr, "Startup: %s %s %s\n", game.sim.error, game.renderer.error, SDL_GetError());
        app_close(&game);
        return 1;
    }
    xr_attach(&game);
    if (route) {
        bool ok = app_route(&game, route);
        app_close(&game);
        return ok ? 0 : 1;
    }
    previous = SDL_GetTicksNS();
#ifdef __EMSCRIPTEN__
    game.touch = EM_ASM_INT(
        { return (navigator.maxTouchPoints > 0 && matchMedia("(pointer:coarse)").matches) ? 1 : 0; });
    emscripten_set_main_loop(frame, 0, 1);
#else
    while (game.running)
        frame();
    app_close(&game);
#endif
    return 0;
}
