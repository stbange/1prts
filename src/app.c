#include "app.h"
#include <math.h>
#include <stdlib.h>
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
static void enter_sector(App *a) {
    camera_reset(&a->camera);
    a->camera.pos = a->sim.spawn;
    a->camera.yaw = a->sim.spawn_yaw;
    a->panel = FIELD;
    a->selected = a->listing = a->category = a->item = a->message_ticks = 0;
    a->stride_x = a->stride_y = a->stick_x = a->stick_y = 0;
    a->left_down = a->right_down = false;
    memset(a->keys, 0, sizeof a->keys);
    a->crew_count = 0;
    a->voice_at = -1000;
    for (int i = 0; i < a->sim.places; i++)
        if (a->sim.place[i].role[0])
            a->roster[a->crew_count++] = i;
}
bool app_mission(App *a, const char *id) {
    Sim *next = calloc(1, sizeof *next);
    Renderer *view = calloc(1, sizeof *view);
    bool ok = next && view && sim_open_mission(next, a->sim.root, id) && render_open(view, next);
    if (ok) {
        sim_close(&a->sim);
        render_close(&a->renderer);
        a->sim = *next;
        a->renderer = *view;
        enter_sector(a);
    } else {
        tell(a, next && next->error[0]   ? next->error
                : view && view->error[0] ? view->error
                                         : "Unable to open operation");
        if (next)
            sim_close(next);
        if (view)
            render_close(view);
    }
    free(view);
    free(next);
    return ok;
}
const Order *app_crew_order(const App *a, int choice) {
    const char *key = a->sim.place[a->roster[a->member]].key;
    for (int i = 0; i < a->sim.order_count; i++)
        if (!strcmp(a->sim.orders[i].actor, key) && choice-- == 0)
            return &a->sim.orders[i];
    return NULL;
}
int app_list_count(const App *a) {
    if (a->panel == CREW_JOB) {
        int n = 0;
        while (app_crew_order(a, n))
            n++;
        return n + 3;
    }
    return a->panel == OPERATIONS ? a->sim.operation_count
           : a->panel == CREWS    ? a->crew_count
                                  : a->sim.order_count;
}
void app_target(App *a, int direction) {
    for (int i = 0; i < a->sim.places; i++) {
        a->destination = (a->destination + direction + a->sim.places) % a->sim.places;
        const Place *p = &a->sim.place[a->destination];
        if (p->key[0] && (!strcmp(p->key, "rival-wake") || p->role[0]))
            return;
    }
}
static void crew_card(App *a, int member) {
    a->member = member;
    a->listing = 0;
    a->destination = a->roster[member];
    a->panel = CREW_JOB;
}
void app_choose(App *a) {
    if (a->panel == OPERATIONS && a->listing < a->sim.operation_count)
        app_mission(a, a->sim.operations[a->listing].id);
    else if (a->panel == CREWS && a->listing < a->crew_count)
        crew_card(a, a->listing);
    else if (a->panel == CREW_JOB) {
        const char *key = a->sim.place[a->roster[a->member]].key;
        const Order *job = app_crew_order(a, a->listing - 3);
        char order[100];
        if (job)
            snprintf(order, sizeof order, "%s", job->id);
        else if (a->listing == 1)
            snprintf(order, sizeof order, "release:%s", key);
        else
            snprintf(order, sizeof order, a->listing == 2 ? "work:%s@%s" : "crew:%s@%s", key,
                     a->sim.place[a->destination].key);
        if (!sim_order(&a->sim, order))
            tell(a, "Crew unavailable for that assignment.");
        else
            a->message_ticks = 0;
    } else if (a->panel == SLATE)
        app_action(a, a->selected);
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
    if (hidden && SDL_getenv("ORRA_COMPACT_REPLAY")) {
        a->width = 288;
        a->height = 162;
    }
    a->window = SDL_CreateWindow("1stprts | Orra Basin", a->width, a->height,
                                 SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                                     SDL_WINDOW_HIGH_PIXEL_DENSITY | (hidden ? SDL_WINDOW_HIDDEN : 0));
    if (!a->window || !(a->context = SDL_GL_CreateContext(a->window)))
        return false;
    if (!sim_open(&a->sim, root) || !render_open(&a->renderer, &a->sim))
        return false;
    SDL_GL_SetSwapInterval(hidden ? 0 : 1);
    if (SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        const char *names[] = {"ack",     "blocked", "weir",    "sounding",
                               "terrace", "salt",    "lantern", "meridian"};
        for (int i = 0; i < 8; i++) {
            char path[1300];
            SDL_AudioSpec spec;
            snprintf(path, sizeof path, "%s/assets/sounds/%s.wav", root, names[i]);
            if (a->sim.sounds[i][0])
                snprintf(path, sizeof path, "%s/%s", root, a->sim.sounds[i]);
            if (SDL_LoadWAV(path, &spec, &a->tone[i], &a->tone_size[i]) && !a->audio)
                a->audio =
                    SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
        }
        if (a->audio)
            SDL_ResumeAudioStreamDevice(a->audio);
    }
    enter_sector(a);
    a->running = true;
    return true;
}
void app_close(App *a) {
    if (a->audio)
        SDL_DestroyAudioStream(a->audio);
    for (int i = 0; i < 8; i++)
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
    if (accepted && a->sim.tick - a->voice_at >= 180) {
        const char *calls[] = {"WEIR", "SOUNDING", "TERRACE", "SALT", "LANTERN", "MERIDIAN"};
        for (int i = 0; i < 6; i++)
            if (!strncmp(a->sim.faction, calls[i], strlen(calls[i])))
                play_cue(a, i + 2);
        a->voice_at = a->sim.tick;
    }
    if (a->observe_order)
        a->observe_order(a->sim.tick, index, accepted);
    a->message_ticks = 0;
}
void app_key(App *a, SDL_Scancode k) {
    if (k == SDL_SCANCODE_L) {
        a->panel = a->panel == RADIO ? FIELD : RADIO;
        capture_mouse(a, false);
        return;
    }
    if (k == SDL_SCANCODE_V && a->crew_count) {
        V3 eye = camera_eye(&a->camera, &a->sim);
        int best = -1;
        float distance = 450;
        for (int j = 0; j < a->crew_count; j++) {
            V3 p;
            int i = a->roster[j];
            if (!world_object(&a->sim, i, &p))
                continue;
            V3 v = to_local(p, eye, a->camera.yaw);
            float d = hypotf(v.x, v.z);
            Shape shape = world_asset_shape(&a->sim, i);
            if (v.z < 0 && fabsf(v.x) < fmaxf(1, shape.w * .46f) + d * .04f && d < distance) {
                best = j;
                distance = d;
            }
        }
        if (best >= 0) {
            crew_card(a, best);
            capture_mouse(a, false);
        }
        return;
    }
    if (a->panel == CREW_JOB) {
        if (k == SDL_SCANCODE_LEFT || k == SDL_SCANCODE_RIGHT) {
            app_target(a, k == SDL_SCANCODE_LEFT ? -1 : 1);
            return;
        }
        if (k == SDL_SCANCODE_ESCAPE) {
            a->panel = CREWS;
            a->listing = a->member;
            return;
        }
    }
    if (k == SDL_SCANCODE_M || k == SDL_SCANCODE_F2 || k == SDL_SCANCODE_C) {
        Panel target = k == SDL_SCANCODE_C ? CREWS : OPERATIONS;
        a->panel = a->panel == target ? FIELD : target;
        a->listing = 0;
        capture_mouse(a, false);
        return;
    }
    if (a->panel == OPERATIONS || a->panel == CREWS || a->panel == CREW_JOB) {
        int count = app_list_count(a);
        if (count && (k == SDL_SCANCODE_DOWN || k == SDL_SCANCODE_UP || k == SDL_SCANCODE_PAGEDOWN)) {
            if (k == SDL_SCANCODE_PAGEDOWN)
                a->listing = ((a->listing / 10 + 1) * 10) % ((count + 9) / 10 * 10);
            else
                a->listing = (a->listing + (k == SDL_SCANCODE_UP ? -1 : 1) + count) % count;
            return;
        }
        if (k == SDL_SCANCODE_RETURN) {
            app_choose(a);
            return;
        }
        if (k >= SDL_SCANCODE_1 && k <= SDL_SCANCODE_0) {
            int target = a->listing / 10 * 10 + k - SDL_SCANCODE_1;
            if (target < count) {
                a->listing = target;
                app_choose(a);
            }
            return;
        }
    }
    if (a->panel == SLATE && k == SDL_SCANCODE_PAGEDOWN) {
        a->selected = ((a->selected / 10 + 1) * 10) % ((a->sim.order_count + 9) / 10 * 10);
        return;
    }
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
        enter_sector(a);
        a->panel = FIELD;
        a->message_ticks = 0;
        a->stride_x = a->stride_y = 0;
        return;
    }
    if (k == SDL_SCANCODE_E) {
        bool boarded = world_interact(&a->sim, &a->camera);
        if (boarded)
            a->stride_x = a->stride_y = 0;
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
        int target = (a->panel == SLATE ? a->selected / 10 * 10 : 0) + (int)k - SDL_SCANCODE_1;
        if (target < a->sim.order_count) {
            a->selected = target;
            app_action(a, target);
        }
        return;
    }
    if (a->panel == SLATE) {
        if (k == SDL_SCANCODE_UP)
            a->selected = (a->selected + a->sim.order_count - 1) % a->sim.order_count;
        if (k == SDL_SCANCODE_DOWN)
            a->selected = (a->selected + 1) % a->sim.order_count;
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
        a->stride_x = a->stride_y = 0;
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
        if (a->panel != FIELD || y < .115f || (x > .58f && y > .84f) ||
            strcmp(a->sim.result, "active")) {
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
static float stride(float previous, float target) {
    float next = previous + .62f * (target - previous);
    return fabsf(target - next) < .01f ? target : next;
}
static int finished(const Side *s) {
    return s->mill + s->watch + s->motor + s->section + s->armor;
}
void app_tick(App *a) {
    if (a->message_ticks > 0)
        a->message_ticks--;
    if (a->panel == FIELD) {
        camera_look(&a->camera, (a->keys[SDL_SCANCODE_RIGHT] - a->keys[SDL_SCANCODE_LEFT]) * .034f,
                    (a->keys[SDL_SCANCODE_UP] - a->keys[SDL_SCANCODE_DOWN]) * .025f);
        a->stride_x = stride(a->stride_x, a->keys[SDL_SCANCODE_D] - a->keys[SDL_SCANCODE_A] + a->stick_x);
        a->stride_y = stride(a->stride_y, a->keys[SDL_SCANCODE_W] - a->keys[SDL_SCANCODE_S] + a->stick_y);
        camera_move(&a->camera, &a->sim, a->stride_x, a->stride_y,
                    (a->keys[SDL_SCANCODE_LSHIFT] ? 1.7f : 1.f) / 30.f);
    } else {
        a->stride_x = a->stride_y = 0;
    }
    int previous_jobs = finished(&a->sim.blue), warning = a->sim.relief_at;
    bool active = !strcmp(a->sim.result, "active");
    float old_yaw = world_angle(&a->sim, a->camera.aboard);
    if (!sim_step(&a->sim, 1)) {
        SDL_Log("Mission error: %s", a->sim.error);
        a->running = false;
    } else if (active && strcmp(a->sim.result, "active"))
        play_cue(a, !strcmp(a->sim.result, "defeat"));
    else if (!warning && a->sim.relief_at)
        play_cue(a, 1);
    else if (previous_jobs < finished(&a->sim.blue))
        play_cue(a, 0);
    a->camera.yaw += world_angle(&a->sim, a->camera.aboard) - old_yaw;
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
void app_advance(App *a, double dt) {
    a->accumulator += fmin(fmax(dt, 0), .25);
    while (a->accumulator + 1.e-12 >= 1. / 30.) {
        app_tick(a);
        a->accumulator = fmax(0, a->accumulator - 1. / 30.);
    }
}
void app_frame(App *a, double dt) {
    SDL_Event event;
    while (SDL_PollEvent(&event))
        app_event(a, &event);
    if (!a->manual_clock)
        app_advance(a, dt);
    if (!a->xr) {
        app_draw(a);
        SDL_GL_SwapWindow(a->window);
    }
}
