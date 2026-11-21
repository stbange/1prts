/* SDL/GL instrument for the real command, tick and drawing paths. No score thresholds live here. */
#include "app.h"
#include <SDL3/SDL_opengl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static App game;
static const char *out;
static Uint64 boot;
static void(APIENTRY *finish_gpu)(void);
static void(APIENTRY *read_pixels)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void *);
static double ms(void) { return (double)(SDL_GetTicksNS() - boot) * 1e-6; }
static void require(bool ok, const char *message) {
    if (!ok) {
        fprintf(stderr, "%s %s\n", message, SDL_GetError());
        exit(1);
    }
}
static void observe(int tick, int id, bool accepted) {
    printf("input,%d,%.4f,%s,%d\n", tick, ms(), game.sim.orders[id].id, accepted);
}
static void key(SDL_Scancode code, bool down) {
    SDL_Event event = {0};
    event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.scancode = code;
    app_event(&game, &event);
}
static void tap(SDL_Scancode code) {
    key(code, true);
    key(code, false);
}
static void shot(const char *name) {
    char path[1200];
    snprintf(path, sizeof path, "%s/%s.ppm", out, name);
    app_draw(&game);
    require(render_capture(&game.renderer, path), "framebuffer capture");
}
static unsigned radio_hash(void) {
    /* At the pinned 960 x 540 resolution the lower left radio is 355 x 48. */
    unsigned char pixels[300 * 28 * 4];
    read_pixels(68, 17, 300, 28, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    unsigned hash = 2166136261u;
    for (size_t i = 0; i < sizeof pixels; i++)
        hash = (hash ^ pixels[i]) * 16777619u;
    return hash;
}
static void realtime(void) {
    shot("boot");
    double deadline = ms(), previous = deadline, command_time = 0;
    unsigned old_hash = 0;
    int command_frame = -1;
    double accumulated = 0;
    for (int frame = 0; frame < 150; frame++) {
        if (frame % 15 == 5) {
            old_hash = radio_hash();
            command_time = ms();
            tap(SDL_SCANCODE_1);
            command_frame = frame;
        }
        double now = ms(), elapsed = now - previous;
        previous = now;
        accumulated += elapsed;
        while (accumulated >= 1000. / 30) {
            app_tick(&game);
            accumulated -= 1000. / 30;
        }
        double draw = ms();
        app_draw(&game);
        finish_gpu();
        double ready = ms();
        if (command_frame >= 0 && radio_hash() != old_hash) {
            printf("visual,%d,%d,%.4f\n", command_frame, frame, ms() - command_time);
            command_frame = -1;
        }
        SDL_GL_SwapWindow(game.window);
        printf("frame,%d,%.4f,%.4f\n", frame, ready - draw, elapsed);
        deadline += 1000. / 60;
        double wait = deadline - ms();
        if (wait > 0)
            SDL_DelayPrecise((Uint64)(wait * 1e6));
        else
            deadline = ms();
    }
}
static void motion(void) {
    camera_reset(&game.camera);
    float previous = game.camera.pos.z, speed = 0;
    float previous_x = game.camera.pos.x, speed_x = 0;
    for (int tick = 0; tick < 18; tick++) {
        if (tick == 0 || tick == 9)
            key(SDL_SCANCODE_W, tick == 0);
        app_tick(&game);
        float next = (previous - game.camera.pos.z) * 30;
        float next_x = (game.camera.pos.x - previous_x) * 30;
        double nearest = 1e9;
        int hostiles = 0;
        V3 eye = camera_eye(&game.camera, &game.sim);
        for (int i = 0; i < game.sim.places; i++) {
            V3 p;
            if (!strncmp(game.sim.place[i].condition, "enemy", 5) &&
                world_object(&game.sim, i, &p)) {
                hostiles++;
                nearest = fmin(nearest, hypot(p.x - eye.x, p.z - eye.z));
            }
        }
        printf("motion,%d,%.5f,%.5f,%.5f,%.5f,%d,%.2f,%.5f,%.5f\n", tick, eye.x, eye.z, next,
               (next - speed) * 30, hostiles, hostiles ? nearest : -1., next_x, (next_x-speed_x)*30);
        previous = game.camera.pos.z;
        speed = next;
        previous_x = game.camera.pos.x;
        speed_x = next_x;
    }
}
static V3 object(const char *condition, bool *present) {
    for (int i = 0; i < game.sim.places; i++) {
        if (!strcmp(game.sim.place[i].condition, condition)) {
            V3 pos;
            *present = world_object(&game.sim, i, &pos);
            return pos;
        }
    }
    fprintf(stderr, "Missing site entity %s\n", condition);
    exit(1);
}
static void feedback(const char *label, SDL_Scancode sc, const char *cause) {
    bool present_before, present_after;
    V3 old = object(cause, &present_before);
    unsigned sounds = game.audio_cues;
    app_draw(&game);
    unsigned radio = radio_hash();
    if (sc != SDL_SCANCODE_UNKNOWN)
        tap(sc);
    app_tick(&game);
    app_draw(&game);
    V3 next = object(cause, &present_after);
    bool world = present_before != present_after ||
                 (present_after && (fabsf(next.x - old.x) + fabsf(next.z - old.z) > .001f));
    printf("feedback,%s,%d,%d,%d\n", label, radio_hash() != radio, game.audio_cues > sounds, world);
}
int main(int argc, char **argv) {
    require(argc == 3, "usage: fun_probe GAME_DIRECTORY OUTPUT_DIRECTORY");
    boot = SDL_GetTicksNS();
    out = argv[2];
    require(app_open(&game, argv[1], true), "startup");
    require(SDL_SetWindowSize(game.window, 960, 540), "window");
    finish_gpu = (void(APIENTRY *)(void))SDL_GL_GetProcAddress("glFinish");
    read_pixels = (void(APIENTRY *)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum,
                                    void *))SDL_GL_GetProcAddress("glReadPixels");
    require(finish_gpu && read_pixels, "OpenGL instrumentation");
    game.observe_order = observe;
    app_draw(&game);
    finish_gpu();
    double interactive = ms();
    printf("lifecycle,boot,0\nlifecycle,first_interactive,%.4f\nlifecycle,run_start,%.4f\n",
           interactive, interactive);
    realtime();
    motion();
    require(sim_reset(&game.sim), "reset");
    camera_reset(&game.camera);
    feedback("mill_order", SDL_SCANCODE_2, "fitters");
    feedback("hauler_order", SDL_SCANCODE_1, "haul");
    while (game.sim.tick < 119)
        app_tick(&game);
    feedback("mill_complete", SDL_SCANCODE_UNKNOWN, "mill");
    require(sim_reset(&game.sim), "idle reset");
    /* Sample recognition on the approach before the carrier occludes the infantry. */
    while (game.sim.red.walk < 100 && game.sim.tick < 7200 && !strcmp(game.sim.result, "active"))
        app_tick(&game);
    require(game.sim.red.walk >= 100, "rival approached within the episode");
    bool visible;
    V3 section = object("enemysection", &visible);
    require(visible, "rival infantry present");
    game.camera.pos = (V3){section.x - 4, 0, section.z + 5};
    game.camera.yaw = .675f;
    game.camera.pitch = -.11f;
    shot("enemy");
    game.sim.red.section = false;
    shot("background");
    game.sim.red.section = true;
    game.camera.pos = (V3){15, 0, -43};
    game.camera.yaw = -.53f;
    game.camera.pitch = .025f;
    while (game.sim.red.hold < game.sim.hold_ticks - 1 && game.sim.tick < 7200 &&
           !strcmp(game.sim.result, "active"))
        app_tick(&game);
    require(game.sim.red.hold == game.sim.hold_ticks - 1, "peak threat");
    shot("peak");
    feedback("gate_loss", SDL_SCANCODE_UNKNOWN, "redbase");
    require(!strcmp(game.sim.result, "defeat"), "unattended rival");
    double over = ms();
    shot("game-over");
    double press = ms();
    tap(SDL_SCANCODE_R);
    app_draw(&game);
    finish_gpu();
    require(!strcmp(game.sim.result, "active") && game.sim.tick == 0, "restart");
    printf("lifecycle,game_over,%.4f\nlifecycle,restart_ready,%.4f\nretry,%.4f\n", over, ms(),
           ms() - press);
    app_close(&game);
    return 0;
}
