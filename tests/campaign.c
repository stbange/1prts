#include "app.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define REQUIRE(expr)                                                                                 \
    do {                                                                                              \
        if (!(expr)) {                                                                                \
            fprintf(stderr, "Campaign unit line %d: %s\n", __LINE__, #expr);                          \
            exit(1);                                                                                  \
        }                                                                                             \
    } while (0)

static int entity(const Sim *s, const char *key) {
    for (int i = 0; i < s->places; i++)
        if (!strcmp(s->place[i].key, key))
            return i;
    return -1;
}
static void stable(V3 a, V3 b) {
    REQUIRE(fabsf(a.x - b.x) < .02f && fabsf(a.y - b.y) < .02f && fabsf(a.z - b.z) < .02f);
}
void campaign_cases(App *a) {
    xr_attach(NULL);
    REQUIRE(!strcmp(orra_status(), "loading"));
    xr_attach(a);
    REQUIRE(!orra_xr_pick(.5f, .5f));
    app_key(a, SDL_SCANCODE_M);
    app_key(a, SDL_SCANCODE_DOWN);
    app_draw(a);
    app_key(a, SDL_SCANCODE_RETURN);
    REQUIRE(!strcmp(a->sim.mission, "tide-window"));
    REQUIRE(!app_mission(a, "unknown") && !strcmp(a->sim.mission, "tide-window"));
    Sim *s = &a->sim;
    int ferry = entity(s, "kestrel"), crawler = entity(s, "mole");
    REQUIRE(ferry >= 0 && crawler >= 0);
    app_key(a, SDL_SCANCODE_E);
    REQUIRE(a->camera.aboard == ferry);
    app_key(a, SDL_SCANCODE_E);
    REQUIRE(a->camera.aboard < 0);
    app_key(a, SDL_SCANCODE_E);
    app_key(a, SDL_SCANCODE_C);
    REQUIRE(app_list_count(a) > 0);
    app_choose(a);
    REQUIRE(strstr(orra_status(), "assignment") && a->panel == CREW_JOB);
    app_choose(a);
    app_draw(a);
    app_key(a, SDL_SCANCODE_L);
    REQUIRE(strstr(orra_status(), "log"));
    app_draw(a);
    app_key(a, SDL_SCANCODE_L);
    app_key(a, SDL_SCANCODE_C);
    REQUIRE(strstr(orra_status(), "crews"));
    app_draw(a);
    app_choose(a);
    int selected = a->destination;
    REQUIRE(orra_xr_pick(.9f, .93f));
    REQUIRE(selected != a->destination);
    app_key(a, SDL_SCANCODE_LEFT);
    int age;
    REQUIRE(sim_crew_status(s, "kestrel", &age) == 1 && age == 0);
    REQUIRE(sim_step(s, 151));
    REQUIRE(sim_crew_status(s, "kestrel", &age) == 2 && age == 1);
    app_draw(a);
    app_key(a, SDL_SCANCODE_RIGHT);
    app_choose(a);
    app_key(a, SDL_SCANCODE_DOWN);
    app_choose(a);
    app_key(a, SDL_SCANCODE_DOWN);
    app_choose(a);
    app_key(a, SDL_SCANCODE_LEFT);
    app_key(a, SDL_SCANCODE_ESCAPE);
    app_key(a, SDL_SCANCODE_UP);
    app_key(a, SDL_SCANCODE_PAGEDOWN);
    app_key(a, SDL_SCANCODE_1);
    app_key(a, SDL_SCANCODE_ESCAPE);
    app_key(a, SDL_SCANCODE_L);
    app_draw(a);
    app_key(a, SDL_SCANCODE_L);
    app_key(a, SDL_SCANCODE_ESCAPE);
    REQUIRE(sim_order(s, "ramp") && sim_order(s, "slip") && sim_order(s, "install"));
    REQUIRE(sim_step(s, 185 * 30) && !strcmp(s->stage, "crossing"));
    REQUIRE(s->place[crawler].linked == ferry + 1);
    V3 origin;
    REQUIRE(world_object(s, crawler, &origin));
    Shape mole = world_asset_shape(s, crawler);
    a->camera.local = (V3){0, .84f, 16 + mole.d * .5f};
    app_key(a, SDL_SCANCODE_E);
    REQUIRE(a->camera.aboard == crawler);
    a->camera.local = (V3){.25f, .84f, 1};
    REQUIRE(sim_step(s, 1731));
    REQUIRE(world_object(s, crawler, &origin));
    V3 expected = to_world(a->camera.local, origin, world_angle(s, crawler));
    expected.y += 1.72f;
    stable(expected, camera_eye(&a->camera, s));
    V3 local = a->camera.local;
    for (int i = 0; i < 3; i++) {
        REQUIRE(sim_step(s, 200));
        stable(local, a->camera.local);
        REQUIRE(world_object(s, crawler, &origin));
        V3 actual = camera_eye(&a->camera, s);
        actual.y -= 1.72f;
        stable(local, to_local(actual, origin, world_angle(s, crawler)));
    }
    REQUIRE(!world_interact(s, &a->camera));
    a->camera.local.z = mole.d * .5f;
    REQUIRE(world_interact(s, &a->camera) && a->camera.aboard == ferry);
    a->camera.local = (V3){-19 * .29f, .84f, 0};
    camera_move(&a->camera, s, 0, .01f, 1.f / 30);
    REQUIRE(a->camera.local.y > 2);
    a->camera.local = (V3){0, .84f, 48};
    REQUIRE(!world_interact(s, &a->camera));
    a->camera = (Camera){.pos = {0, 0, -29.9f}, .aboard = -1};
    camera_move(&a->camera, s, 0, 1, 1);
    REQUIRE(a->camera.pos.z > -30);
    REQUIRE(sim_step(s, 151 + 380 * 30 - s->tick) && !strcmp(s->stage, "island"));
    a->camera.aboard = ferry;
    a->camera.local = (V3){0, 0, 50.5f};
    a->camera.yaw = world_angle(s, ferry);
    V3 before = camera_eye(&a->camera, s);
    camera_move(&a->camera, s, 0, -1, 1.f / 30);
    REQUIRE(a->camera.aboard == -1);
    V3 ashore = camera_eye(&a->camera, s);
    REQUIRE(hypotf(ashore.x - before.x, ashore.z - before.z) < .1f &&
            fabsf(ashore.y - before.y) < .02f);
    Camera bow = {.aboard = ferry, .local = {0, 0, -53.01f}};
    V3 bord = camera_eye(&bow, s);
    camera_move(&bow, s, 0, 1, 1.f / 30);
    REQUIRE(bow.aboard < 0 && fabsf(bord.y - camera_eye(&bow, s).y) < .02f);
    camera_move(&bow, s, 0, -1, 1.f / 30);
    REQUIRE(bow.aboard == ferry);
    REQUIRE(sim_step(s, 2100));
    stable(ashore, camera_eye(&a->camera, s));
    REQUIRE(app_mission(a, "high-water"));
    int tender = entity(s, "lark"), station = entity(s, "station");
    a->camera.pos = (V3){0, 0, 43};
    REQUIRE(world_interact(s, &a->camera));
    REQUIRE(a->camera.aboard == tender && sim_order(s, "module") && sim_step(s, 540));
    REQUIRE(sim_order(s, "lift") && sim_step(s, 1800));
    a->camera.local.z = 42;
    REQUIRE(!world_interact(s, &a->camera));
    REQUIRE(sim_order(s, "dock") && sim_step(s, 660));
    a->camera.local = (V3){0, .84f, -41.64f};
    REQUIRE(world_interact(s, &a->camera) && a->camera.aboard == station);
    REQUIRE(world_interact(s, &a->camera) && a->camera.aboard == tender);
    app_key(a, SDL_SCANCODE_TAB);
    app_key(a, SDL_SCANCODE_PAGEDOWN);
    app_draw(a);
    app_key(a, SDL_SCANCODE_ESCAPE);
    orra_xr_action(4);
    REQUIRE(a->panel == OPERATIONS);
    orra_xr_action(5);
    REQUIRE(a->panel == CREWS);
    orra_xr_action(6);
    a->manual_clock = true;
    int tick = s->tick;
    orra_clock(2);
    REQUIRE(s->tick == tick + 2);
    orra_clock(-1);
    a->manual_clock = false;
    orra_clock(100);
    REQUIRE(s->tick == tick + 2);
    REQUIRE(app_mission(a, "gate-six"));
    app_key(a, SDL_SCANCODE_V);
    REQUIRE(a->panel == CREW_JOB);
    app_key(a, SDL_SCANCODE_ESCAPE);
    app_key(a, SDL_SCANCODE_ESCAPE);
    a->camera.yaw = 0;
    a->camera.pos = (V3){0, 0, 0};
    REQUIRE(!orra_xr_mark(0, 1.72f, 0, 1, 0, 0));
    REQUIRE(orra_xr_mark(0, 1.72f, 0, 0, 0, -1));
    app_key(a, SDL_SCANCODE_TAB);
    Mat projection = mat_projection(1, 1), view = mat_view((V3){0, 1.7f, 0}, 0, 0);
    orra_xr_wrist(view.v);
    orra_xr_eye(projection.v, view.v, 0, 0, 100, 100, 0, 1.7f, 0);
    orra_xr_pick(.7f, 155.f / 400);
    orra_xr_pick(.6f, 105.f / 400);
    orra_xr_wrist(NULL);
    app_touch(a, .93f, .145f);
    app_draw(a);
    REQUIRE(strstr(orra_status(), "gate-six"));
}
