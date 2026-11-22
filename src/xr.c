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
    snprintf(status, sizeof status,
             "%s|%s|%d|%d|%s|%d|%d|%s|%d|%d|%.3f|%.3f|%.3f|%d|%d|%d|%d|%d|%d|%d|%s|%s|%s",
             game->sim.result,
             game->desk.page != DESK_CLOSED ? "desk"
             : game->panel == FIELD         ? "field"
             : game->panel == SLATE         ? "orders"
             : game->panel == OPERATIONS    ? "operations"
             : game->panel == CREWS         ? "crews"
             : game->panel == CREW_JOB      ? "assignment"
             : game->panel == CHART         ? "chart"
             : game->panel == RADIO         ? "log"
                                            : "guide",
             game->sim.blue.alloy, game->sim.tick, game->sim.report, game->touch, game->selected,
             game->sim.mission, game->camera.aboard, game->sim.progress, game->camera.pos.x,
             game->camera.pos.z, game->camera.yaw, game->left_down, game->right_down,
             game->sim.hold_ticks, game->member, game->destination, game->sim.order_count,
             game->save_pending,
             game->sim.crew_count ? game->sim.place[game->sim.roster[game->member]].key : "",
             *game->ground_target ? game->ground_target : game->sim.place[game->destination].key,
             game->message);
    return status;
}
void orra_sector_ready(int request, int success) {
    if (game)
        app_sector_ready(game, request, success);
}
void orra_saved(int success) {
    if (!game) return;
    bool manual = game->desk.write_slot >= 0 && game->desk.write_slot < 2;
    app_save_complete(game, success != 0);
    if (success && manual && !game->save_pending)
        SDL_strlcpy(game->message, "Operation saved in this browser.", sizeof game->message);
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
    if (game->desk.page != DESK_CLOSED) {
        strafe = walk = turn = 0;
    }
    if (game->desk.current.value[PREF_COMFORT])
        turn *= .5f;
    else if (game->desk.current.value[PREF_TURN])
        turn *= game->desk.current.value[PREF_TURN] == 1 ? .5f : 1.5f;
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
    if (game->desk.page != DESK_CLOSED && x >= 0 && x <= 1 && y >= 0 && y <= 1) {
        session_tap(game, x, y, 350, 400);
        return 1;
    }
    if (game->panel == FIELD || x < 0 || x > 1 || y < 0 || y > 1)
        return 0;
    if (game->panel == CHART) {
        chart_pick(game, 350, x * 350, y * 400);
        return 1;
    }
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
    if (game->desk.page != DESK_CLOSED)
        return 0;
    V3 base=camera_eye(&game->camera,&game->sim);
    base.y-=1.72f;
    return app_mark(game,to_world((V3){x,y,z},base,game->camera.yaw),
                    to_world((V3){dx,dy,dz},(V3){0},game->camera.yaw));
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
    if (game->desk.page != DESK_CLOSED || game->panel != FIELD || game->sim.relief_at) {
        if (game->wrist_valid && (game->desk.page != DESK_CLOSED || game->panel != FIELD)) {
            Mat projected = mat_mul(p, mat_mul(v, game->wrist));
            ui_wrist(game);
            render_slate(&game->renderer, 350, 400, &projected);
            return;
        }
        int lw = 900, lh = (int)(900.f * h / w);
        if (game->panel == FIELD && game->desk.page == DESK_CLOSED)
            ui_relief_notice(&game->renderer, &game->sim, false);
        else
            ui_draw(game, lw, lh);
        render_overlay(&game->renderer, lw, lh);
    }
}
int orra_xr_binding(int physical) {
    return game && physical >= 0 && physical < XR_BINDINGS ? game->desk.current.xr[physical] : -1;
}
void orra_xr_action(int physical) {
    int action = orra_xr_binding(physical);
    const SDL_Scancode actions[] = {SDL_SCANCODE_TAB,    SDL_SCANCODE_DOWN, SDL_SCANCODE_RETURN,
                                    SDL_SCANCODE_E,      SDL_SCANCODE_M,    SDL_SCANCODE_C,
                                    SDL_SCANCODE_ESCAPE, SDL_SCANCODE_R,    SDL_SCANCODE_RIGHT,
                                    SDL_SCANCODE_L,      SDL_SCANCODE_V,    SDL_SCANCODE_P};
    if (action >= 0 && action < XR_BINDINGS)
        app_key(game, actions[action]);
}

static bool list_panel(const App *a) {
    return a->panel == SLATE || a->panel == CREWS || a->panel == CREW_JOB || a->panel == OPERATIONS;
}
int orra_menu_count(void) {
    if (!game)
        return 0;
    if (game->desk.page != DESK_CLOSED)
        return session_count(game);
    if (game->panel == RADIO)
        return 8 + SDL_min(game->call_count, 12);
    if (game->panel == GUIDE)
        return 5;
    if (!list_panel(game))
        return 0;
    return app_list_count(game) + (game->panel == CREW_JOB ? 3 : 1);
}
const char *orra_menu_heading(void) {
    if (!game)
        return "";
    if (game->desk.page != DESK_CLOSED)
        return session_title(game);
    const char *names[] = {"",           "Orders",    "Field guide", "Operations", "Crew dispatch",
                           "Assignment", "Radio log", "Sector chart"};
    return lang_text(&game->renderer.lang, names[game->panel]);
}
static const char *row_text(int i) {
    static char row[400];
    if (!game || i < 0 || i >= orra_menu_count())
        return "";
    if (game->desk.page != DESK_CLOSED)
        return session_row(game, i, row, sizeof row);
    if (game->panel == RADIO && i < 7) {
        sim_log(&game->sim, i, row, sizeof row);
        return row;
    }
    if (game->panel == RADIO && i < orra_menu_count() - 1)
        return game->calls[(game->call_count - 1 - (i - 7)) % 12];
    const char *label = "Close";
    if (game->panel == GUIDE)
        label = (const char *[]){"Previous entry", "Next entry", "Previous category",
                                 "Next category", "Close"}[i];
    else if (list_panel(game)) {
        int n = app_list_count(game);
        if (i < n)
            label = app_list_label(game, i);
        else if (game->panel == CREW_JOB && i < n + 2)
            label = i == n ? "Previous target" : "Next target";
    }
    return lang_text(&game->renderer.lang, label);
}
const char *orra_menu_row(int i) {
    static char translated[500];
    const char *raw = row_text(i);
    return game ? lang_format(&game->renderer.lang, raw, translated, sizeof translated) : raw;
}
static const char *detail_text(void) {
    static char text[1400];
    if (!game)
        return "";
    if (game->desk.page != DESK_CLOSED) {
        session_description(game, text, sizeof text);
        return text;
    }
    if (game->panel == CREW_JOB)
        return game->ground_target[0] ? game->ground_target
                                      : game->sim.place[game->destination].name;
    if (game->panel == GUIDE) {
        const Entry *e = &game->sim.cats[game->category].entry[game->item];
        snprintf(text, sizeof text, "%s / %s", e->name, e->role);
        return text;
    }
    return lang_text(&game->renderer.lang, game->message_ticks ? game->message : game->sim.report);
}
const char *orra_menu_detail(void) {
    static char translated[1500];
    const char *raw = detail_text();
    return game ? lang_format(&game->renderer.lang, raw, translated, sizeof translated) : raw;
}
const char *orra_field_label(int sc) {
    static char label[128];
    for (int i = 0; game && i < session_commands(); i++) {
        SDL_Scancode key;
        const char *name = session_command(i, &key);
        if ((int)key == sc) {
            if (key == SDL_SCANCODE_E)
                name = "Interact";
            if (key == SDL_SCANCODE_H)
                name = "Help";
            if (key == SDL_SCANCODE_L)
                name = "Radio";
            return session_prompt(game, key, name, label, sizeof label);
        }
    }
    return "";
}
const char *orra_language_tag(void) {
    if (!game || !game->renderer.lang.available) return "en";
    return game->renderer.lang.tags[game->renderer.lang.selected];
}
int orra_menu_flags(void) {
    if (!game)
        return 0;
    return (game->desk.page != DESK_CLOSED) | (game->touch << 1) | (game->xr << 2) |
           (game->desk.current.value[PREF_NARRATE] << 3) | (game->desk.rebinding << 4) |
           (game->desk.current.value[PREF_HAPTIC] << 5) |
           (game->desk.current.value[PREF_SCALE] << 6);
}
void orra_menu_pick(int i) {
    if (!game || i < 0 || i >= orra_menu_count())
        return;
    if (game->desk.page != DESK_CLOSED) {
        session_pick(game, i);
        return;
    }
    if (game->panel == GUIDE && i < 4) {
        app_key(game, (SDL_Scancode[]){SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT,
                                       SDL_SCANCODE_RIGHT}[i]);
        return;
    }
    if (list_panel(game) && i < app_list_count(game)) {
        if (game->panel == SLATE)
            game->selected = i;
        else
            game->listing = i;
        app_choose(game);
        return;
    }
    if (game->panel == CREW_JOB && i < app_list_count(game) + 2) {
        app_target(game, i == app_list_count(game) ? -1 : 1);
        return;
    }
    if (game->panel != RADIO || i == orra_menu_count() - 1)
        app_key(game, SDL_SCANCODE_ESCAPE);
}
void orra_menu_key(int sc) {
    if (game && sc > 0 && sc < SDL_SCANCODE_COUNT)
        app_key(game, (SDL_Scancode)sc);
}
void orra_interrupt(void) {
    if (game)
        session_pause(game);
}
void orra_device(int touch) {
    if (game)
        game->touch = touch != 0;
}
void orra_fault(const char *category) {
    if (game)
        session_report(game, category);
}
