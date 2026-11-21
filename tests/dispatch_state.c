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
    VERIFY(app_mission(a, "tide-window"));
    app_key(a, SDL_SCANCODE_E);
    VERIFY(!strcmp(s->place[a->camera.aboard].key, "kestrel"));
    VERIFY(sim_order(s, "ramp") && sim_order(s, "slip") && sim_order(s, "install"));
    VERIFY(sim_step(s, 5550));
    a->camera.local = (V3){0, .84f, 27};
    app_key(a, SDL_SCANCODE_E);
    VERIFY(a->camera.aboard == place(s, "mole"));
    VERIFY(sim_order(s, "crew:reed@rival-ferry"));
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
    int mole = place(s, "mole");
    a->camera.aboard = -1;
    V3 mid;
    VERIFY(world_object(s, mole, &mid));
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
    VERIFY(app_mission(a, "high-water"));
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
