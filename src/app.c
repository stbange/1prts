#include "app.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static void tell(App *a, const char *msg) {
    snprintf(a->message, sizeof a->message, "%s", msg);
    a->message_ticks = 95;
}
static void capture_mouse(App *a, bool on) {
    a->captured = on;
    SDL_SetWindowRelativeMouseMode(a->window, on);
}
bool app_open(App *a, const char *root, bool hidden) {
    memset(a, 0, sizeof *a);
    camera_reset(&a->camera);
    if (!SDL_Init(SDL_INIT_VIDEO))
        return false;
#ifdef __EMSCRIPTEN__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef __APPLE__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
#endif
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    a->width = 1280;
    a->height = 720;
    a->window = SDL_CreateWindow("1stprts | Orra Basin", a->width, a->height,
                                 SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                                     SDL_WINDOW_HIGH_PIXEL_DENSITY | (hidden ? SDL_WINDOW_HIDDEN : 0));
    if (!a->window || !(a->context = SDL_GL_CreateContext(a->window)))
        return false;
    if (!sim_open(&a->sim, root) || !render_open(&a->renderer, &a->sim))
        return false;
    SDL_GL_SetSwapInterval(hidden ? 0 : 1);
    if (SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        const char *names[] = {"ack", "blocked"};
        for (int i = 0; i < 2; i++) {
            char path[1300];
            SDL_AudioSpec spec;
            snprintf(path, sizeof path, "%s/assets/sounds/%s.wav", root, names[i]);
            if (SDL_LoadWAV(path, &spec, &a->tone[i], &a->tone_size[i]) && !a->audio)
                a->audio =
                    SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
        }
        if (a->audio)
            SDL_ResumeAudioStreamDevice(a->audio);
    }
    a->running = true;
    return true;
}
void app_close(App *a) {
    if (a->audio)
        SDL_DestroyAudioStream(a->audio);
    for (int i = 0; i < 2; i++)
        SDL_free(a->tone[i]);
    render_close(&a->renderer);
    sim_close(&a->sim);
    if (a->context)
        SDL_GL_DestroyContext(a->context);
    if (a->window)
        SDL_DestroyWindow(a->window);
    SDL_Quit();
}
static void play_cue(App *a, int tone) {
    if (a->audio && a->tone[tone] &&
        SDL_PutAudioStreamData(a->audio, a->tone[tone], (int)a->tone_size[tone]))
        a->audio_cues++;
}
void app_action(App *a, int index) {
    bool accepted = sim_command(&a->sim, index);
    play_cue(a, accepted ? 0 : 1);
    if (a->observe_order)
        a->observe_order(a->sim.tick, index, accepted);
    a->message_ticks = 0;
}
void app_key(App *a, SDL_Scancode k) {
    if (k == SDL_SCANCODE_TAB || k == SDL_SCANCODE_F1 || k == SDL_SCANCODE_G) {
        Panel target = k == SDL_SCANCODE_TAB ? SLATE : GUIDE;
        a->panel = a->panel == target ? FIELD : target;
        capture_mouse(a, false);
        return;
    }
    if (k == SDL_SCANCODE_ESCAPE) {
        if (a->panel != FIELD)
            a->panel = FIELD;
        capture_mouse(a, false);
        return;
    }
    if (k == SDL_SCANCODE_R && strcmp(a->sim.result, "active")) {
        sim_reset(&a->sim);
        camera_reset(&a->camera);
        a->panel = FIELD;
        a->message_ticks = 0;
        return;
    }
    if (k == SDL_SCANCODE_E) {
        bool boarded = world_interact(&a->sim, &a->camera);
        if (!boarded && world_terminal(&a->sim, &a->camera)) {
            a->panel = SLATE;
            capture_mouse(a, false);
            return;
        }
        tell(a, boarded ? (a->camera.aboard >= 0 ? "Aboard. The vehicle crew has control."
                                                 : "Back on foot.")
                        : "Approach the rear hatch to board.");
        return;
    }
    if (a->panel == GUIDE) {
        if (k == SDL_SCANCODE_LEFT || k == SDL_SCANCODE_RIGHT) {
            a->category =
                (a->category + (k == SDL_SCANCODE_RIGHT ? 1 : CATEGORY_COUNT - 1)) % CATEGORY_COUNT;
            a->item = 0;
        } else if (k == SDL_SCANCODE_UP || k == SDL_SCANCODE_DOWN) {
            int count = a->sim.cats[a->category].count;
            a->item = (a->item + (k == SDL_SCANCODE_DOWN ? 1 : count - 1)) % count;
        }
        return;
    }
    if (k >= SDL_SCANCODE_1 && k <= SDL_SCANCODE_0) {
        a->selected = (int)k - SDL_SCANCODE_1;
        app_action(a, a->selected);
        return;
    }
    if (a->panel == SLATE) {
        if (k == SDL_SCANCODE_UP)
            a->selected = (a->selected + ORDER_COUNT - 1) % ORDER_COUNT;
        if (k == SDL_SCANCODE_DOWN)
            a->selected = (a->selected + 1) % ORDER_COUNT;
        if (k == SDL_SCANCODE_RETURN)
            app_action(a, a->selected);
    }
}
void app_touch(App *a, float x, float y) {
    ui_tap(a, x, y);
}
void app_event(App *a, const SDL_Event *e) {
    switch (e->type) {
    case SDL_EVENT_QUIT:
        a->running = false;
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        memset(a->keys, 0, sizeof a->keys);
        a->left_down = a->right_down = false;
        a->stick_x = a->stick_y = 0;
        capture_mouse(a, false);
        break;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
        SDL_Scancode sc = e->key.scancode;
        if (sc > SDL_SCANCODE_UNKNOWN && sc < SDL_SCANCODE_COUNT) {
            a->keys[sc] = e->type == SDL_EVENT_KEY_DOWN;
            if (a->keys[sc] && !e->key.repeat)
                app_key(a, sc);
        }
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (e->button.which == SDL_TOUCH_MOUSEID)
            break;
        if (a->panel != FIELD) {
            int w, h;
            SDL_GetWindowSize(a->window, &w, &h);
            ui_tap(a, e->button.x / w, e->button.y / h);
        } else
            capture_mouse(a, true);
        break;
    case SDL_EVENT_MOUSE_MOTION:
        if (a->captured && a->panel == FIELD)
            camera_look(&a->camera, e->motion.xrel * .0022f, -e->motion.yrel * .0022f);
        break;
    case SDL_EVENT_FINGER_DOWN: {
        a->touch = true;
        float x = e->tfinger.x, y = e->tfinger.y;
        if (a->panel != FIELD || (x > .58f && y > .84f) || strcmp(a->sim.result, "active")) {
            app_touch(a, x, y);
            break;
        }
        if (x < .45f) {
            a->left_down = true;
            a->left_id = e->tfinger.fingerID;
            a->left_x = x;
            a->left_y = y;
        } else {
            a->right_down = true;
            a->right_id = e->tfinger.fingerID;
        }
        break;
    }
    case SDL_EVENT_FINGER_MOTION:
        if (a->left_down && a->left_id == e->tfinger.fingerID) {
            a->stick_x = (e->tfinger.x - a->left_x) * 9;
            a->stick_y = (a->left_y - e->tfinger.y) * 9;
        } else if (a->right_down && a->right_id == e->tfinger.fingerID)
            camera_look(&a->camera, e->tfinger.dx * 3.2f, -e->tfinger.dy * 2.6f);
        break;
    case SDL_EVENT_FINGER_UP:
    case SDL_EVENT_FINGER_CANCELED:
        if (a->left_down && a->left_id == e->tfinger.fingerID) {
            a->left_down = false;
            a->stick_x = a->stick_y = 0;
        }
        if (a->right_down && a->right_id == e->tfinger.fingerID)
            a->right_down = false;
        break;
    default:
        break;
    }
}
void app_tick(App *a) {
    if (a->message_ticks > 0)
        a->message_ticks--;
    if (a->panel == FIELD) {
        camera_look(&a->camera, (a->keys[SDL_SCANCODE_RIGHT] - a->keys[SDL_SCANCODE_LEFT]) * .034f,
                    (a->keys[SDL_SCANCODE_UP] - a->keys[SDL_SCANCODE_DOWN]) * .025f);
        camera_move(&a->camera, &a->sim, a->keys[SDL_SCANCODE_D] - a->keys[SDL_SCANCODE_A] + a->stick_x,
                    a->keys[SDL_SCANCODE_W] - a->keys[SDL_SCANCODE_S] + a->stick_y,
                    (a->keys[SDL_SCANCODE_LSHIFT] ? 1.7f : 1.f) / 30.f);
    }
    if (!sim_step(&a->sim, 1)) {
        SDL_Log("Mission error: %s", a->sim.error);
        a->running = false;
    }
}
void app_draw(App *a) {
    SDL_GetWindowSizeInPixels(a->window, &a->width, &a->height);
    if (a->width <= 0 || a->height <= 0)
        return;
    V3 eye = camera_eye(&a->camera, &a->sim);
    Mat vp = mat_mul(mat_projection((float)a->width / a->height, 1.04f),
                     mat_view(eye, a->camera.yaw, a->camera.pitch));
    render_view(&a->renderer, &a->sim, &a->camera, vp, eye, 0, 0, a->width, a->height);
    int w, h;
    ui_size(a, &w, &h);
    ui_draw(a, w, h);
    render_overlay(&a->renderer, w, h);
}
void app_frame(App *a, double dt) {
    SDL_Event event;
    while (SDL_PollEvent(&event))
        app_event(a, &event);
    a->accumulator += fmin(fmax(dt, 0), .25);
    while (a->accumulator >= 1. / 30.) {
        app_tick(a);
        a->accumulator -= 1. / 30.;
    }
    if (!a->xr) {
        app_draw(a);
        SDL_GL_SwapWindow(a->window);
    }
}
