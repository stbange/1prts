#include "app.h"
#include <lauxlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(q)                                                                                       \
    do {                                                                                               \
        if (!(q)) {                                                                                    \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #q);                                    \
            exit(1);                                                                                   \
        }                                                                                              \
    } while (0)
static const char *fixture = "orra-unit-files";
static void put(const char *name, const char *body) {
    char path[512];
    snprintf(path, sizeof path, "%s/%s", fixture, name);
    FILE *f = fopen(path, "wb");
    CHECK(f);
    fputs(body, f);
    CHECK(fclose(f) == 0);
}
static void copy_asset(const char *root, const char *name) {
    char from[1400], to[512];
    snprintf(from, sizeof from, "%s/%s", root, name);
    snprintf(to, sizeof to, "%s/%s", fixture, name);
    CHECK(SDL_CopyFile(from, to));
}
static void variant(const char *root, const char *name, const char *edit) {
    char path[100], body[1700];
    snprintf(path, sizeof path, "scripts/%s.lua", name);
    snprintf(body, sizeof body, "local t=dofile([==[%s/%s]==])\n%s\nreturn t", root, path, edit);
    put(path, body);
}
static void invalid_sim(const char *reason) {
    Sim s;
    CHECK(!sim_open(&s, fixture));
    CHECK(strstr(s.error, reason));
    sim_close(&s);
}
static void content_errors(const char *root) {
    put("scripts/catalog.lua", "invalid %%");
    invalid_sim("catalog.lua");
    put("scripts/catalog.lua", "error('broken pack')");
    invalid_sim("broken pack");
    put("scripts/catalog.lua", "return 12");
    invalid_sim("return a table");
    variant(root, "catalog", "t.units={}");
    invalid_sim("catalog size");
    variant(root, "catalog", "t.units[1]=12");
    invalid_sim("entry must");
    variant(root, "catalog", "t.units[1].name=nil");
    invalid_sim("ID or name");
    variant(root, "catalog", "t.units[2].id=t.units[1].id");
    invalid_sim("Duplicate");
    copy_asset(root, "scripts/catalog.lua");
    put("scripts/site.lua", "return {}");
    invalid_sim("site size");
    put("scripts/site.lua", "return {1}");
    invalid_sim("site entry");
    copy_asset(root, "scripts/site.lua");
    variant(root, "gate_six", "t.hold_ticks=0");
    invalid_sim("mission rules");
    variant(root, "gate_six", "t.actions[1]=false");
    invalid_sim("mission order");
    variant(root, "gate_six", "t.new=function() return 4 end");
    invalid_sim("state table");
    variant(root, "gate_six", "t.new=function() return {} end");
    invalid_sim("faction state");
    variant(root, "gate_six", "t.new=function() return {blue={}} end");
    invalid_sim("faction queues");
    variant(root, "gate_six", "t.new=function() error('new failed') end");
    invalid_sim("new failed");
    variant(root, "gate_six", "t.command=function() error('job failed') end;t.step=t.command");
    Sim s;
    CHECK(sim_open(&s, fixture));
    CHECK(!sim_command(&s, 0));
    CHECK(!sim_step(&s, 1));
    CHECK(strstr(s.error, "job failed"));
    sim_close(&s);
}
static void bad_render(Sim *s, const char *msg) {
    Renderer r;
    CHECK(!render_open(&r, s));
    CHECK(strstr(r.error, msg));
    render_close(&r);
}
static void art_errors(App *a, const char *root) {
    Sim s = a->sim;
    snprintf(s.root, sizeof s.root, "%s", fixture);
    put("assets/font.txt", "A 12");
    bad_render(&s, "bad glyph");
    copy_asset(root, "assets/font.txt");
    put("assets/radio.txt", "short");
    bad_render(&s, "portrait");
    copy_asset(root, "assets/radio.txt");
    put("assets/shaders/world.vert.glsl", "this is invalid");
    bad_render(&s, "world.vert.glsl");
    const char *programs[] = {"world", "sky", "panel"};
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 2; j++) {
            char n[110];
            snprintf(n, sizeof n, "assets/shaders/%s.%s.glsl", programs[i], j ? "frag" : "vert");
            copy_asset(root, n);
        }
    put("assets/shaders/world.frag.glsl", "in vec4 world; out vec4 frag;void main(){frag=world;}");
    bad_render(&s, "world");
    copy_asset(root, "assets/shaders/world.frag.glsl");
    put("assets/models/terrain.mesh", "ORRAinvalid");
    bad_render(&s, "mesh header");
    snprintf(s.place[0].model, sizeof s.place[0].model, "does-not-exist");
    bad_render(&s, "missing mesh");
    s.places = 0;
    bad_render(&s, "terrain model");
    snprintf(s.root, sizeof s.root, "%s/no-directory", fixture);
    bad_render(&s, "missing font");
}
static void xr_cases(App *a) {
    xr_attach(a);
    orra_xr_mode(1);
    orra_xr_tick(.04, .2f, .3f, .1f);
    CHECK(a->xr && strstr(orra_status(), "active|"));
    Mat p = mat_projection(1, 1), v = mat_view((V3){0, 1.7f, 0}, 0, 0);
    a->panel = FIELD;
    orra_xr_eye(p.v, v.v, 0, 0, 64, 64, -.03f, 1.7f, 0);
    orra_xr_action(0);
    CHECK(strstr(orra_status(), "orders"));
    orra_xr_action(1);
    orra_xr_action(2);
    orra_xr_eye(p.v, v.v, 64, 0, 64, 64, .03f, 1.7f, 0);
    orra_xr_action(3);
    orra_xr_action(-1);
    orra_xr_mode(0);
    CHECK(!a->xr && a->stick_x == 0 && a->stick_y == 0);
    a->panel = FIELD;
}
void component_tests(App *a, const char *root) {
    const char *dirs[] = {"orra-unit-files", "orra-unit-files/scripts", "orra-unit-files/assets",
                          "orra-unit-files/assets/shaders", "orra-unit-files/assets/models"};
    for (size_t i = 0; i < sizeof dirs / sizeof *dirs; i++)
        CHECK(SDL_CreateDirectory(dirs[i]));
    content_errors(root);
    art_errors(a, root);
    xr_cases(a);
    a->camera.pos = (V3){-26, 0, -12};
    CHECK(world_terminal(&a->sim, &a->camera));
    app_draw(a);
    app_key(a, SDL_SCANCODE_E);
    CHECK(a->panel == SLATE);
    app_key(a, SDL_SCANCODE_ESCAPE);
    a->camera.pos = (V3){-8, 0, 10};
    app_draw(a);
    app_key(a, SDL_SCANCODE_E);
    app_draw(a);
    app_key(a, SDL_SCANCODE_E);
    put("parser.route",
        "# miniature parser case\n\nkey 1\nhold W 1\nlook 4 3\nwait 1\nexpect result active\nexpect "
        "aboard 0\nexpect armor 0\nexpect section 0\nexpect alloy 8\nshot component\n");
    sim_reset(&a->sim);
    CHECK(app_route(a, "orra-unit-files/parser.route"));
    put("parser.route", "expect no-such-value 0");
    CHECK(!app_route(a, "orra-unit-files/parser.route"));
    put("parser.route", "bogus");
    CHECK(!app_route(a, "orra-unit-files/parser.route"));
    CHECK(!app_route(a, "no-such-route.orra"));
    app_draw(a);
    CHECK(render_capture(&a->renderer, "orra-unit-files/colour.ppm"));
    a->sim.blue.hold = a->sim.hold_ticks;
    snprintf(a->sim.result, sizeof a->sim.result, "victory");
    app_touch(a, .5f, .5f);
    CHECK(!strcmp(a->sim.result, "active"));
}
static int quit_later(void *unused) {
    (void)unused;
    SDL_Delay(500);
    SDL_Event e = {0};
    e.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&e);
    return 0;
}
void host_tests(const char *root) {
    char *usage[] = {"orra", "--unknown", NULL};
    CHECK(orra_run(2, usage) == 2);
    char *broken[] = {"orra", "--data", "orra-unit-files/nonexistent", NULL};
    CHECK(orra_run(3, broken) == 1);
    put("parser.route", "expect alloy 8");
    char *route[] = {"orra", "--data", (char *)root, "--replay", "orra-unit-files/parser.route", NULL};
    CHECK(orra_run(5, route) == 0);
    char *campaign[]={"orra","--data",(char *)root,"--mission","white-mile","--replay","orra-unit-files/parser.route",NULL};
    CHECK(orra_run(7,campaign)==0);campaign[4]="missing";CHECK(orra_run(7,campaign)==1);
    char *normal[] = {"orra", "--data", (char *)root, NULL};
    SDL_Thread *thread = SDL_CreateThread(quit_later, "quit", NULL);
    CHECK(thread);
    CHECK(orra_run(3, normal) == 0);
    SDL_WaitThread(thread, NULL);
}
