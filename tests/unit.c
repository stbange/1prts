#include "app.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(e)                                                                                       \
    do {                                                                                               \
        if (!(e)) {                                                                                    \
            fprintf(stderr, "%s:%d failed: %s\n", __FILE__, __LINE__, #e);                             \
            exit(1);                                                                                   \
        }                                                                                              \
    } while (0)
static bool closef(float a, float b) {
    return fabsf(a - b) < .002f;
}
static void math_cases(void) {
    V3 loc = {1.3f, 2, -4}, anchor = {9, 3.5f, -11};
    V3 p = to_local(to_world(loc, anchor, .91f), anchor, .91f);
    CHECK(closef(p.x, loc.x) && closef(p.y, loc.y) && closef(p.z, loc.z));
    Mat v = mat_view(anchor, .6f, .3f);
    Mat projection = mat_projection(16.f / 9, 1.1f);
    Mat vp = mat_mul(projection, v);
    CHECK(vp.v[11] != 0 && projection.v[0] < projection.v[5]);
    CHECK(world_shape("something").w == 0 && world_shape("mole").boardable);
}
static int index_of(const Sim *s, const char *id) {
    for (int i = 0; i < s->places; i++)
        if (!strcmp(s->place[i].model, id))
            return i;
    return -1;
}
static void logic_cases(App *a) {
    Sim *s = &a->sim;
    CHECK(s->cats[UNITS].count == 39 && s->cats[BUILDINGS].count == 30);
    CHECK(s->blue.alloy == 8 && !sim_command(s, -1) && !sim_command(s, 100));
    CHECK(sim_command(s, 1));
    CHECK(!sim_command(s, 1));
    CHECK(s->blue.work_left == 120);
    CHECK(sim_step(s, 120) && s->blue.mill && s->blue.alloy == 4);
    CHECK(sim_command(s, 0));
    CHECK(sim_step(s, 300) && s->blue.alloy == 8);
    CHECK(sim_command(s, 2));
    sim_step(s, 90);
    CHECK(sim_command(s, 4));
    sim_step(s, 90);
    CHECK(s->blue.section);
    CHECK(!sim_command(s, 5));
    CHECK(sim_command(s, 6));
    sim_step(s, 300);
    CHECK(s->blue.walk == 300);
    CHECK(sim_command(s, 3));
    sim_step(s, 160);
    CHECK(sim_command(s, 5));
    sim_step(s, 160);
    CHECK(s->blue.armor);
    CHECK(sim_command(s, 7));
    for (int i = 0; i < s->places; i++) {
        V3 pos;
        world_object(s, i, &pos);
    }
    int mole = index_of(s, "mole");
    V3 base;
    CHECK(world_object(s, mole, &base));
    Camera *c = &a->camera;
    camera_reset(c);
    CHECK(world_nearest(s, c) == -1 && !world_interact(s, c));
    c->pos = (V3){base.x, 0, base.z + 8.3f};
    CHECK(world_interact(s, c));
    int seated = c->aboard;
    V3 first = camera_eye(c, s);
    camera_move(c, s, 1, 1, .15f);
    sim_step(s, 40);
    V3 second = camera_eye(c, s);
    CHECK(seated == mole && fabsf(first.z - second.z) > .1);
    CHECK(world_interact(s, c) && c->aboard == -1);
    camera_look(c, 14, 42);
    CHECK(closef(c->pitch, 1.3f));
    camera_reset(c);
    c->pos = (V3){-26, 0, -1};
    camera_move(c, s, 0, 1, .6f);
    CHECK(c->pos.z < -3.8f);
    c->pos = (V3){-20, 0, -2.9f};
    camera_move(c, s, 0, 1, .10f);
    CHECK(closef(c->pos.z, -2.9f));
    c->pos = (V3){70.5f, 0, 0};
    camera_move(c, s, 1, 0, 1);
    CHECK(closef(c->pos.x, 70.5f));
    sim_step(s, 2500);
    CHECK(!strcmp(s->result, "victory"));
    app_draw(a);
    app_key(a, SDL_SCANCODE_R);
    CHECK(!strcmp(s->result, "active"));
}
static void inputs(App *a) {
    SDL_Event e = {0};
    e.type = SDL_EVENT_KEY_DOWN;
    e.key.scancode = SDL_SCANCODE_W;
    app_event(a, &e);
    float z = a->camera.pos.z;
    app_tick(a);
    CHECK(a->camera.pos.z < z);
    e.type = SDL_EVENT_KEY_UP;
    app_event(a, &e);
    e.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    app_event(a, &e);
    e.type = SDL_EVENT_MOUSE_MOTION;
    e.motion.xrel = 10;
    app_event(a, &e);
    CHECK(a->captured);
    e.type = SDL_EVENT_WINDOW_FOCUS_LOST;
    app_event(a, &e);
    CHECK(!a->captured);
    app_key(a, SDL_SCANCODE_E);
    CHECK(a->message_ticks > 0);
    app_key(a, SDL_SCANCODE_TAB);
    app_key(a, SDL_SCANCODE_DOWN);
    app_key(a, SDL_SCANCODE_UP);
    app_key(a, SDL_SCANCODE_RETURN);
    CHECK(a->sim.blue.harvesting);
    app_draw(a);
    int w, h;
    ui_size(a, &w, &h);
    ui_tap(a, (w - 64.f) / w, 151.f / h);
    CHECK(a->selected == 1);
    app_key(a, SDL_SCANCODE_3);
    e.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    e.button.x = 0;
    e.button.y = 100;
    app_event(a, &e);
    app_key(a, SDL_SCANCODE_ESCAPE);
    app_key(a, SDL_SCANCODE_F1);
    for (int i = 0; i < CATEGORY_COUNT; i++) {
        app_key(a, SDL_SCANCODE_DOWN);
        app_draw(a);
        app_key(a, SDL_SCANCODE_UP);
        app_key(a, SDL_SCANCODE_RIGHT);
    }
    CHECK(a->category == 0 && a->item == 0);
    app_key(a, SDL_SCANCODE_LEFT);
    CHECK(a->category == 4);
    const float start = w - (w > 700 ? 415 : 340) - 16, pw = w > 700 ? 415.f : 340.f;
    for (int i = 0; i < 4; i++)
        ui_tap(a, (start + pw * (.1f + i * .26f)) / w, 258.f / h);
    CHECK(a->panel == FIELD);
    e = (SDL_Event){0};
    e.type = SDL_EVENT_FINGER_DOWN;
    e.tfinger.x = .2f;
    e.tfinger.y = .5f;
    e.tfinger.fingerID = 1;
    app_event(a, &e);
    e.type = SDL_EVENT_FINGER_MOTION;
    e.tfinger.x = .25f;
    app_event(a, &e);
    CHECK(a->stick_x > 0);
    e.type = SDL_EVENT_FINGER_DOWN;
    e.tfinger.x = .7f;
    e.tfinger.fingerID = 2;
    app_event(a, &e);
    e.type = SDL_EVENT_FINGER_MOTION;
    e.tfinger.dx = .1f;
    app_event(a, &e);
    e.type = SDL_EVENT_FINGER_UP;
    app_event(a, &e);
    e.tfinger.fingerID = 1;
    app_event(a, &e);
    CHECK(a->stick_x == 0 && !a->right_down);
    e.type = SDL_EVENT_FINGER_DOWN;
    e.tfinger.x = .9f;
    e.tfinger.y = .97f;
    app_event(a, &e);
    CHECK(a->panel == GUIDE);
    app_key(a, SDL_SCANCODE_ESCAPE);
    app_frame(a, .1);
    CHECK(a->running);
    app_key(a, SDL_SCANCODE_TAB);
    ui_tap(a, .9f, 80.f / h);
    CHECK(a->panel == FIELD);
    app_touch(a, .84f, .92f);
    app_touch(a, .96f, .92f);
    app_draw(a);
    e.type = SDL_EVENT_QUIT;
    app_event(a, &e);
    CHECK(!a->running);
    a->running = true;
}
void component_tests(App *, const char *);
void host_tests(const char *);
int main(int argc, char **argv) {
    CHECK(argc == 2);
    math_cases();
    Sim bad;
    CHECK(!sim_open(&bad, "/not-present-orra") && bad.error[0]);
    sim_close(&bad);
    App *a = calloc(1, sizeof *a);
    CHECK(a);
    if (!app_open(a, argv[1], true)) {
        fprintf(stderr, "%s %s %s\n", SDL_GetError(), a->sim.error, a->renderer.error);
        return 1;
    }
    logic_cases(a);
    inputs(a);
    component_tests(a, argv[1]);
    CHECK(!render_capture(&a->renderer, "/missing-orra-directory/a.ppm"));
    app_close(a);
    free(a);
    host_tests(argv[1]);
    puts("Native unit assertions passed");
    return 0;
}
