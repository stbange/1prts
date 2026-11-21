#include "app.h"
#include <stdio.h>
#include <string.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
static App *game;
void xr_attach(App *a) {
    game = a;
}
/* Read-only status also makes the canvas operation available to assistive tools. */
const char *orra_status(void) {
    static char status[340];
    if (!game)
        return "loading";
    snprintf(status, sizeof status, "%s|%s|%d|%d|%s|%d|%d", game->sim.result,
             game->panel == FIELD   ? "field"
             : game->panel == SLATE ? "orders"
                                    : "guide",
             game->sim.blue.alloy, game->sim.tick, game->sim.report, game->touch, game->selected);
    return status;
}
void orra_xr_mode(int on) {
    game->xr = on != 0;
    game->stick_x = game->stick_y = 0;
#ifdef __EMSCRIPTEN__
    if (on)
        emscripten_pause_main_loop();
    else
        emscripten_resume_main_loop();
#endif
}
void orra_xr_tick(double dt, float walk, float strafe, float turn) {
    game->stick_x = strafe;
    game->stick_y = walk;
    camera_look(&game->camera, turn, 0);
    app_frame(game, dt);
}
void orra_xr_eye(const float *projection, const float *view, int x, int y, int w, int h, float ox,
                 float oy, float oz) {
    Mat p, v;
    memcpy(p.v, projection, sizeof p.v);
    memcpy(v.v, view, sizeof v.v);
    V3 anchor = camera_eye(&game->camera, &game->sim);
    anchor.y -= 1.72f;
    Mat base = mat_view(anchor, game->camera.yaw, 0);
    Mat vp = mat_mul(p, mat_mul(v, base));
    V3 eye = to_world((V3){ox, oy, oz}, anchor, game->camera.yaw);
    render_view(&game->renderer, &game->sim, &game->camera, vp, eye, x, y, w, h);
    if (game->panel != FIELD || game->sim.relief_at) {
        int lw = 900, lh = (int)(900.f * h / w);
        if (game->panel == FIELD)
            ui_relief_notice(&game->renderer, &game->sim, false);
        else
            ui_draw(game, lw, lh);
        render_overlay(&game->renderer, lw, lh);
    }
}
void orra_xr_action(int action) {
    const SDL_Scancode actions[] = {SDL_SCANCODE_TAB, SDL_SCANCODE_DOWN, SDL_SCANCODE_RETURN,
                                    SDL_SCANCODE_E};
    if (action >= 0 && action < 4)
        app_key(game, actions[action]);
}
