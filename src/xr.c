#include "app.h"
#include <stdio.h>
#include <math.h>
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
    static char status[560];
    if (!game)
        return "loading";
    snprintf(status, sizeof status, "%s|%s|%d|%d|%s|%d|%d|%s|%d|%d|%.3f|%.3f|%.3f|%d|%d|%d|%d|%d",
             game->sim.result,
             game->panel == FIELD        ? "field"
             : game->panel == SLATE      ? "orders"
             : game->panel == OPERATIONS ? "operations"
             : game->panel == CREWS      ? "crews"
             : game->panel == CREW_JOB   ? "assignment"
             : game->panel == RADIO      ? "log"
                                         : "guide",
             game->sim.blue.alloy, game->sim.tick, game->sim.report, game->touch, game->selected,
             game->sim.mission, game->camera.aboard, game->sim.progress, game->camera.pos.x,
             game->camera.pos.z, game->camera.yaw, game->left_down, game->right_down,
             game->sim.hold_ticks, game->member, game->destination);
    return status;
}
void orra_clock(int ticks) {
    if (!game || !game->manual_clock || ticks < 0 || ticks > 108000)
        return;
    SDL_Event event;
    while (SDL_PollEvent(&event))
        app_event(game, &event);
    for (int i = 0; i < ticks; i++)
        app_tick(game);
}
void orra_xr_mode(int on) {
    game->xr = on != 0;
    game->wrist_valid = false;
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
void orra_xr_wrist(const float *pose) {
    game->wrist_valid = pose != NULL;
    if (pose)
        memcpy(game->wrist.v, pose, sizeof game->wrist.v);
}
int orra_xr_pick(float x, float y) {
    if (game->panel == FIELD || x < 0 || x > 1 || y < 0 || y > 1)
        return 0;
    /* Wrist coordinates use the slate's own logical extent, independent of canvas density. */
    if (game->panel == CREW_JOB && y * 400 >= 362)
        app_target(game, x > .5f ? 1 : -1);
    else if (y * 400 >= 123 && y * 400 < 363 && x > .13f) {
        int *selection = game->panel == SLATE ? &game->selected : &game->listing;
        int item = *selection / 10 * 10 + (int)((y * 400 - 123) / 24);
        if (item < app_list_count(game)) {
            *selection = item;
            app_choose(game);
        }
    } else if (y * 400 >= 96 && y * 400 < 123)
        app_key(game, SDL_SCANCODE_PAGEDOWN);
    return 1;
}
int orra_xr_mark(float x, float y, float z, float dx, float dy, float dz) {
    if (game->panel != FIELD || strcmp(game->sim.mission, "gate-six"))
        return 0;
    V3 anchor = camera_eye(&game->camera, &game->sim);
    anchor.y -= 1.72f;
    V3 origin = to_world((V3){x, y, z}, anchor, game->camera.yaw);
    V3 ray = to_world((V3){dx, dy, dz}, (V3){0}, game->camera.yaw);
    V3 to = {-origin.x, 1.72f - origin.y, -66 - origin.z};
    float distance = to.x * ray.x + to.y * ray.y + to.z * ray.z;
    if (distance <= 0 || hypotf(hypotf(to.x - distance * ray.x, to.z - distance * ray.z),
                                to.y - distance * ray.y) > 4.5f)
        return 0;
    for (int i = 0; i < game->sim.order_count; i++)
        if (!strcmp(game->sim.orders[i].id, "advance")) {
            app_action(game, i);
            return 1;
        }
    return 0;
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
        if (game->wrist_valid && game->panel != FIELD) {
            Mat projected = mat_mul(p, mat_mul(v, game->wrist));
            ui_wrist(game);
            render_slate(&game->renderer, 350, 400, &projected);
            return;
        }
        int lw = 900, lh = (int)(900.f * h / w);
        if (game->panel == FIELD)
            ui_relief_notice(&game->renderer, &game->sim, false);
        else
            ui_draw(game, lw, lh);
        render_overlay(&game->renderer, lw, lh);
    }
}
void orra_xr_action(int action) {
    const SDL_Scancode actions[] = {SDL_SCANCODE_TAB,    SDL_SCANCODE_DOWN, SDL_SCANCODE_RETURN,
                                    SDL_SCANCODE_E,      SDL_SCANCODE_M,    SDL_SCANCODE_C,
                                    SDL_SCANCODE_ESCAPE, SDL_SCANCODE_R,    SDL_SCANCODE_RIGHT,
                                    SDL_SCANCODE_L,      SDL_SCANCODE_V};
    if (action >= 0 && action < 11)
        app_key(game, actions[action]);
}
