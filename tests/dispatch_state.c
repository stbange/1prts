#include "app.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERIFY(e)                                                                                    \
    do {                                                                                             \
        if (!(e)) {                                                                                  \
            fprintf(stderr, "Dispatch state %d: %s\n", __LINE__, #e);                                \
            exit(1);                                                                                 \
        }                                                                                            \
    } while (0)
static int place(const Sim *s, const char *key) {
    for (int i = 0; i < s->places; i++)
        if (!strcmp(key, s->place[i].key))
            return i;
    return -1;
}
static void choice(App *a, const char *key) {
    for (int i = 0; i < app_list_count(a); i++) {
        const char *command = app_field_choice(a, i, false);
        if (command && !strcmp(command, key)) {
            a->listing = i;
            app_choose(a);
            return;
        }
    }
    VERIFY(false);
}
static void bad_slot(const char *name, const char *body) {
    uint32_t check = 2166136261u;
    for (const unsigned char *c = (const unsigned char *)body; *c; c++)
        check = (check ^ *c) * 16777619u;
    char file[512];
    snprintf(file, sizeof file, "ORRA1 %08x\n%s", check, body);
    VERIFY(SDL_SaveFile(name, file, strlen(file)));
}
static void point_input(App *a, int mode) {
    if (mode == 2) {
        VERIFY(orra_xr_mark(0, 1.72f, 0, 0, sinf(a->camera.pitch), -cosf(a->camera.pitch)));
    } else {
        SDL_Event e = {0};
        if (mode == 0) {
            a->captured = true;
            e.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
            e.button.button = SDL_BUTTON_LEFT;
            app_event(a, &e);
        } else {
            e.type = SDL_EVENT_FINGER_DOWN;
            e.tfinger.x = .5f;
            e.tfinger.y = .5f;
            e.tfinger.fingerID = 9;
            app_event(a, &e);
            e.type = SDL_EVENT_FINGER_UP;
            app_event(a, &e);
        }
    }
}
static void slate_controls(App *a) {
    VERIFY(app_mission(a, "gate-six"));
    Sim *s = &a->sim;
    V3 lot, gate;
    VERIFY(world_object(s, place(s, "common-lot"), &lot));
    VERIFY(app_mark(a, (V3){lot.x, 4, lot.z}, (V3){0, -1, 0}) && a->panel == FIELD &&
           a->message_ticks > 0);
    VERIFY(!app_mark(a, (V3){350, 5, 150}, (V3){0, 1, 0}));
    VERIFY(!app_mark(a, (V3){350, 5, 150}, (V3){0, -1, 0}));
    a->panel = CREWS;
    VERIFY(!app_mark(a, lot, (V3){0, -1, 0}));
    a->panel = FIELD;
    VERIFY(!world_ground(s, (V3){429, 0, 0}));
    int gate_id = place(s, "gate");
    VERIFY(world_object(s, gate_id, &gate));
    float yaw = world_angle(s, gate_id);
    const V3 wall[] = {{12, 0, 0}, {0, 0, -4}, {5, 0, 1}};
    for (int i = 0; i < 3; i++)
        VERIFY(!world_ground(s, to_world(wall[i], gate, yaw)));
    s->objective[0] = 0;
    s->relief_at = 1;
    bool *stages[] = {&s->blue.mill,  &s->blue.harvesting, &s->blue.watch, &s->blue.section,
                      &s->blue.motor, &s->blue.armor,      &s->blue.guard, &s->blue.advance};
    for (size_t i = 0; i < sizeof stages / sizeof stages[0]; i++) {
        ui_draw(a, 960, 540);
        VERIFY(a->renderer.ui_count > 0);
        a->renderer.ui_count = 0;
        *stages[i] = true;
    }
    s->blue.walk = 300;
    ui_draw(a, 960, 540);
    a->renderer.ui_count = 0;
    int old_w = a->width, old_h = a->height, w, h;
    a->width = a->height = 1;
    ui_size(a, &w, &h);
    VERIFY(w == 10 && h == 10);
    a->width = 960;
    a->height = 540;
    ui_tap(a, 344.f / 960, 42.f / 540);
    VERIFY(a->panel == CHART);
    VERIFY(orra_xr_pick(.4f, .5f));
    ui_tap(a, 290.f / 960, 42.f / 540);
    VERIFY(a->panel == RADIO);
    ui_tap(a, 205.f / 960, 42.f / 540);
    VERIFY(a->panel == CREWS);
    a->listing = 0;
    app_choose(a);
    VERIFY(a->panel == CREW_JOB);
    ui_tap(a, 900.f / 960, 374.f / 540);
    a->panel = GUIDE;
    ui_wrist(a);
    a->renderer.ui_count = 0;
    a->panel = FIELD;
    ui_wrist(a);
    a->renderer.ui_count = 0;
    VERIFY(app_mission(a, "tide-window"));
    Camera step = {.pos = {110, 1, 80}, .aboard = -1};
    camera_move(&step, s, .5f, 1, .04f);
    VERIFY(step.pos.y < 1);
    a->panel = SLATE;
    ui_wrist(a);
    a->renderer.ui_count = 0;
    a->width = old_w;
    a->height = old_h;
}
static void equivalent_orders(App *a) {
    uint32_t reference = 0;
    for (int mode = 0; mode < 3; mode++) {
        VERIFY(app_mission(a, "gate-six"));
        Sim *s = &a->sim;
        V3 p;
        VERIFY(world_object(s, place(s, "mole"), &p));
        a->camera.pos = (V3){p.x, 0, p.z + 20};
        a->camera.yaw = 0;
        point_input(a, mode);
        VERIFY(a->panel == CREW_JOB && s->roster[a->member] == place(s, "mole"));
        choice(a, "point");
        VERIFY(world_object(s, place(s, "west-pad"), &p));
        a->camera.pos = (V3){p.x, 0, p.z + 12};
        a->camera.pitch = -.08f;
        point_input(a, mode);
        VERIFY(a->pointed && a->destination == place(s, "west-pad"));
        choice(a, "move");
        VERIFY(strstr(s->report, "acknowledged"));
        if (mode)
            VERIFY(reference == sim_digest(s));
        else
            reference = sim_digest(s);
    }
}
void dispatch_state_test(App *a) {
    Sim *s = &a->sim;
    const char *dir = "orra-unit-files/yard-save";
    VERIFY(SDL_CreateDirectory(dir));
    char slot[1400], previous[1400];
    snprintf(slot, sizeof slot, "%s/command.sav", dir);
    snprintf(previous, sizeof previous, "%s/previous.sav", dir);
    SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "ORRA_SAVE_DIR", dir, true);
    SDL_RemovePath(slot);
    SDL_RemovePath(previous);
    VERIFY(!app_resume(a));
    bad_slot(slot, "invalid");
    VERIFY(!app_resume(a));
    bad_slot(slot, "absent - - - -\n0 0 0 0 0 0 0 0\n0 0 0 0 0\n[]");
    VERIFY(!app_resume(a));
    bad_slot(slot, "gate-six - - - -\n0 0 0 0 0 0 0 0\n0 0 0 0 0\n[]");
    VERIFY(!app_resume(a));
    SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "ORRA_SAVE_DIR", slot, true);
    VERIFY(!app_save(a) && !app_resume(a));
    SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "ORRA_SAVE_DIR", dir, true);
    SDL_RemovePath(slot);
    VERIFY(SDL_CreateDirectory(slot));
    VERIFY(!app_save(a));
    SDL_RemovePath(slot);
    slate_controls(a);
    equivalent_orders(a);
    const char *verify_orders = "expect guard 0\nexpect dispatch mole@move@west-pad\n";
    const char *route = "orra-unit-files/check-driver.route";
    VERIFY(SDL_SaveFile(route, verify_orders, strlen(verify_orders)) && app_route(a, route));
    SDL_RemovePath(route);
    VERIFY(app_mission(a, "compact-bank") && a->camera.aboard == place(s, "shed"));
    SDL_strlcpy(a->pending_mission, "gate-six", sizeof a->pending_mission);
    orra_sector_ready(a->sector_request + 2, 1);
    orra_sector_ready(a->sector_request, 0);
    VERIFY(!*a->pending_mission && strstr(a->message, "sector"));
    SDL_strlcpy(a->pending_mission, "tide-window", sizeof a->pending_mission);
    orra_sector_ready(a->sector_request, 1);
    VERIFY(!strcmp(s->mission, "tide-window"));
    VERIFY(app_mission(a, "tide-window"));
    app_key(a, SDL_SCANCODE_E);
    VERIFY(!strcmp(s->place[a->camera.aboard].key, "kestrel"));
    VERIFY(sim_order(s, "ramp") && sim_order(s, "slip") && sim_order(s, "install"));
    VERIFY(sim_step(s, 5550));
    a->camera.local = (V3){0, .84f, 27};
    app_key(a, SDL_SCANCODE_E);
    VERIFY(a->camera.aboard == place(s, "mole"));
    VERIFY(sim_order(s, "crew:reed@rival-ferry"));
    V3 last;
    int age;
    VERIFY(!sim_contact(s, "unreported", &last, &age));
    VERIFY(sim_step(s, 2100) && sim_contact(s, "rival-ferry", &last, &age));
    app_key(a, SDL_SCANCODE_B);
    VERIFY(a->panel == CHART);
    app_draw(a);
    ui_wrist(a);
    const float port_scale = 240.f / 420;
    chart_pick(a, 350, 184 + last.x * port_scale, 342 + (last.z - 90) * port_scale);
    VERIFY(a->destination == place(s, "rival-ferry"));
    ui_wrist(a);
    chart_pick(a, 350, 0, 0);
    app_key(a, SDL_SCANCODE_B);
    uint32_t first = sim_digest(s);
    V3 eye = camera_eye(&a->camera, s);
    app_key(a, SDL_SCANCODE_F5);
    VERIFY(strstr(a->message, "saved"));
    VERIFY(sim_step(s, 195));
    uint32_t future = sim_digest(s);
    VERIFY(app_save(a) && app_mission(a, "gate-six"));
    app_key(a, SDL_SCANCODE_F9);
    VERIFY(sim_digest(s) == future && a->camera.aboard == place(s, "mole"));
    VERIFY(SDL_SaveFile(slot, "interrupted", 11));
    VERIFY(app_resume(a) && sim_digest(s) == first && strstr(a->message, "previous"));
    V3 restored = camera_eye(&a->camera, s);
    VERIFY(fabsf(restored.x - eye.x) < .02f && fabsf(restored.z - eye.z) < .02f);
    VERIFY(sim_step(s, 195) && sim_digest(s) == future);
    snprintf(a->ground_target, sizeof a->ground_target, "g-483-1141");
    VERIFY(app_save(a));
    a->ground_target[0] = 0;
    VERIFY(app_resume(a) && !strcmp(a->ground_target, "g-483-1141"));
    a->ground_target[0] = 0;
    a->save_pending = true;
    VERIFY(!app_save(a) && !app_resume(a));
    orra_saved(0);
    orra_saved(1);
    VERIFY(!app_continue(a));
    VERIFY(sim_step(s, 19000) && app_continue(a) && !strcmp(s->mission, "westbound"));
    a->panel = OPERATIONS;
    for (int i = 0; i < 3; i++) {
        a->panel = OPERATIONS;
        a->listing = s->operation_count + i;
        app_choose(a);
    }
    VERIFY(app_mission(a, "gate-six"));
    VERIFY(sim_order(s, "mill") && sim_order(s, "harvest") && sim_step(s, 300));
    VERIFY(sim_order(s, "motor") && sim_step(s, 300) && sim_order(s, "armor") && sim_step(s, 300));
    VERIFY(sim_order(s, "armor") && sim_step(s, 150) && place(s, "lintel-2") >= 0);
    app_draw(a);
    VERIFY(sim_reset(s));
    app_draw(a);
    int mole = place(s, "mole");
    a->camera.aboard = -1;
    V3 mid;
    VERIFY(world_object(s, mole, &mid));
    app_key(a, SDL_SCANCODE_B);
    chart_pick(a, 350, 184 + mid.x * .8f, 342 + (mid.z - 90) * .8f);
    VERIFY(a->panel == CREW_JOB && s->roster[a->member] == mole);
    choice(a, "chart");
    VERIFY(a->panel == CHART && a->aiming);
    int w, h;
    ui_size(a, &w, &h);
    V3 pad;
    VERIFY(world_object(s, place(s, "west-pad"), &pad));
    ui_tap(a, (w - 166 + pad.x * .8f) / w, (342 + (pad.z - 90) * .8f) / h);
    VERIFY(a->panel == CREW_JOB && a->destination == place(s, "west-pad"));
    app_draw(a);
    a->panel = FIELD;
    mid.y += 1;
    VERIFY(app_mark(a, (V3){mid.x, mid.y, mid.z + 20}, (V3){0, 0, -1}));
    VERIFY(a->panel == CREW_JOB && s->roster[a->member] == mole);
    choice(a, "group");
    choice(a, "point");
    VERIFY(a->aiming);
    VERIFY(app_mark(a, (V3){63, 3, 41}, (V3){0, -1, 0}));
    VERIFY(a->pointed && *a->ground_target && a->panel == CREW_JOB);
    choice(a, "move");
    VERIFY(strstr(s->report, "acknowledged"));
    choice(a, "point");
    app_key(a, SDL_SCANCODE_ESCAPE);
    VERIFY(!a->aiming);
    app_key(a, SDL_SCANCODE_ESCAPE);
    app_key(a, SDL_SCANCODE_ESCAPE);
    app_screen_point(a, .5f, .5f);
    VERIFY(!world_point(s, mid, (V3){0}, -1).ground);
    a->panel = CREW_JOB;
    a->ground_target[0] = 0;
    a->pointed = true;
    a->destination = place(s, "lintel");
    choice(a, "move");
    VERIFY(strstr(a->message, "left the sector"));
    a->destination = s->places;
    choice(a, "move");
    VERIFY(a->destination == s->roster[a->member] && strstr(a->message, "left the sector"));
    VERIFY(app_mission(a, "high-water"));
    a->panel = CHART;
    ui_wrist(a);
    a->panel = FIELD;
    VERIFY(!app_stage(a, "westbound", "[]") && !strcmp(s->mission, "high-water"));
    int station = place(s, "station");
    a->camera.aboard = station;
    a->camera.local = (V3){-4, .84f, 326};
    VERIFY(app_lift(a, false));
    app_key(a, SDL_SCANCODE_E);
    for (int i = 0; i < 123; i++)
        app_tick(a);
    VERIFY(fabsf(a->camera.local.y - 6.84f) < .02f);
    app_draw(a);
    app_key(a, SDL_SCANCODE_E);
    for (int i = 0; i < 121; i++)
        app_tick(a);
    VERIFY(fabsf(a->camera.local.y - .84f) < .02f);
    SDL_RemovePath(slot);
    SDL_RemovePath(previous);
    SDL_UnsetEnvironmentVariable(SDL_GetEnvironment(), "ORRA_SAVE_DIR");
}
