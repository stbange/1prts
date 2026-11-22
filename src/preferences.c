#include "app.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

typedef struct {
    const char *title, *choices;
    int initial;
} Setting;
static const Setting settings[PREF_COUNT] = {
    {"Window mode", "Windowed|Borderless|Fullscreen", 0},
    {"Resolution", "Current|960 x 540|1280 x 720|1920 x 1080", 0},
    {"V-sync", "Off|On", 1},
    {"Frame cap", "Display|30|60|120", 0},
    {"Graphics quality", "Full|Low shadows", 0},
    {"Brightness", "Dim|Standard|Bright|Very bright", 1},
    {"Master volume", "0%|25%|50%|75%|100%", 4},
    {"Music volume", "0%|25%|50%|75%|100%", 4},
    {"Effects volume", "0%|25%|50%|75%|100%", 4},
    {"Radio volume", "0%|25%|50%|75%|100%", 4},
    {"Mute", "Off|On", 0},
    {"Interface size", "100%|125%|150%", 0},
    {"Caption size", "100%|125%|150%", 0},
    {"Caption background", "Translucent|Solid", 0},
    {"Simulation pace", "Standard|Assisted: half speed", 0},
    {"Reduced motion and flashes", "Off|On", 0},
    {"VR turning", "30 degree snap|15 degree snap|45 degree snap", 0},
    {"Look sensitivity", "Low|Standard|High", 1},
    {"Invert look Y", "Off|On", 0},
    {"Movement keys", "Hold|Toggle", 0},
    {"Controller haptics", "Off|On", 1},
    {"Menu narration", "Off|On", 0},
    {"desk.language", "English", 0}};
typedef struct {
    SDL_Scancode key;
    const char *title;
} Binding;
static const Binding actions[] = {{SDL_SCANCODE_W, "Walk forward"},
                                  {SDL_SCANCODE_S, "Walk backward"},
                                  {SDL_SCANCODE_A, "Walk left"},
                                  {SDL_SCANCODE_D, "Walk right"},
                                  {SDL_SCANCODE_LSHIFT, "Run"},
                                  {SDL_SCANCODE_LEFT, "Left / previous target"},
                                  {SDL_SCANCODE_RIGHT, "Right / next target"},
                                  {SDL_SCANCODE_UP, "Up / look up"},
                                  {SDL_SCANCODE_DOWN, "Down / look down"},
                                  {SDL_SCANCODE_RETURN, "Confirm"},
                                  {SDL_SCANCODE_ESCAPE, "Back / pause"},
                                  {SDL_SCANCODE_P, "Pause"},
                                  {SDL_SCANCODE_E, "Use hatch or desk"},
                                  {SDL_SCANCODE_TAB, "Orders"},
                                  {SDL_SCANCODE_C, "Crews"},
                                  {SDL_SCANCODE_V, "Address crew"},
                                  {SDL_SCANCODE_B, "Chart"},
                                  {SDL_SCANCODE_M, "Operations"},
                                  {SDL_SCANCODE_L, "Radio log"},
                                  {SDL_SCANCODE_F1, "Field guide"},
                                  {SDL_SCANCODE_H, "Briefing and help"},
                                  {SDL_SCANCODE_R, "Retry"},
                                  {SDL_SCANCODE_F5, "Save"},
                                  {SDL_SCANCODE_F9, "Load"},
                                  {SDL_SCANCODE_PAGEDOWN, "Next page"},
                                  {SDL_SCANCODE_1, "Order 1"},
                                  {SDL_SCANCODE_2, "Order 2"},
                                  {SDL_SCANCODE_3, "Order 3"},
                                  {SDL_SCANCODE_4, "Order 4"},
                                  {SDL_SCANCODE_5, "Order 5"},
                                  {SDL_SCANCODE_6, "Order 6"},
                                  {SDL_SCANCODE_7, "Order 7"},
                                  {SDL_SCANCODE_8, "Order 8"},
                                  {SDL_SCANCODE_9, "Order 9"},
                                  {SDL_SCANCODE_0, "Order 10"}};
int session_commands(void) { return (int)(sizeof actions / sizeof actions[0]); }
const char *session_command(int i, SDL_Scancode *key) {
    *key = actions[i].key;
    return actions[i].title;
}
void session_defaults(Preferences *p) {
    for (int i = 0; i < XR_BINDINGS; i++)
        p->xr[i] = i;
    for (int i = 0; i < PREF_COUNT; i++)
        p->value[i] = settings[i].initial;
    for (int i = 0; i < SDL_SCANCODE_COUNT; i++)
        p->binding[i] = (SDL_Scancode)i;
}
static int choices(const App *a, int id) {
    if (id == PREF_LANGUAGE)
        return SDL_max(1, a->renderer.lang.available);
    int total = 1;
    for (const char *s = settings[id].choices; *s; s++)
        total += *s == '|';
    return total;
}
static bool managed(int id) {
#ifdef __EMSCRIPTEN__
    return id == PREF_WINDOW || id == PREF_RESOLUTION || id == PREF_VSYNC;
#else
    (void)id;
    return false;
#endif
}
const char *session_pref_label(const App *a, int i, char *out, size_t n) {
    const Language *l = &a->renderer.lang;
    int v = a->desk.edit.value[i];
    const char *value = settings[i].choices;
    if (managed(i))
        value = "Browser managed";
    else if (i == PREF_LANGUAGE && l->available)
        value = l->names[v];
    else
        while (v-- > 0)
            value = strchr(value, '|') + 1;
    char copy[90];
    snprintf(copy, sizeof copy, "%.*s", (int)strcspn(value, "|"), value);
    snprintf(out, n, "%s: %s", lang_text(l, settings[i].title), lang_text(l, copy));
    return out;
}
void session_adjust(App *a, int d) {
    int i = a->desk.row;
    if (i >= 0 && i < PREF_COUNT && !managed(i)) {
        int n = choices(a, i);
        a->desk.edit.value[i] = (a->desk.edit.value[i] + d + n) % n;
    }
}
void session_settings(App *a, bool write) {
    char path[1400], temp[1400];
    if (!app_data_path("preferences.txt", path, sizeof path) ||
        !app_data_path("preferences.tmp", temp, sizeof temp))
        return;
    if (write) {
        FILE *f = fopen(temp, "w");
        if (!f)
            return;
        for (int i = 0; i < PREF_COUNT; i++)
            fprintf(f, "p %d %d\n", i, a->desk.current.value[i]);
        for (int i = 0; i < XR_BINDINGS; i++)
            fprintf(f, "x %d %d\n", i, a->desk.current.xr[i]);
        for (int i = 0; i < session_commands(); i++) {
            int k = actions[i].key;
            fprintf(f, "k %d %d\n", k, a->desk.current.binding[k]);
        }
        if (fclose(f) == 0 && SDL_RenamePath(temp, path)) {
#ifdef __EMSCRIPTEN__
            EM_ASM({ Module.storePreference("preferences.txt"); });
#endif
        } else
            SDL_RemovePath(temp);
        return;
    }
    FILE *f = fopen(path, "r");
    if (!f)
        return;
    int id, v;
    char kind;
    while (fscanf(f, " %c %d %d", &kind, &id, &v) == 3) {
        if (kind == 'x' && id >= 0 && id < XR_BINDINGS && v >= 0 && v < XR_BINDINGS)
            a->desk.current.xr[id] = v;
        if (kind == 'p' && id >= 0 && id < PREF_COUNT && v >= 0 && v < choices(a, id))
            a->desk.current.value[id] = v;
        if (kind == 'k' && id > 0 && id < SDL_SCANCODE_COUNT && v > 0 && v < SDL_SCANCODE_COUNT)
            a->desk.current.binding[id] = (SDL_Scancode)v;
    }
    fclose(f);
}
void session_apply(App *a) {
    int *v = a->desk.current.value;
    sound_levels(a);
    a->renderer.brightness = (float)(v[PREF_BRIGHT] - 1);
    a->renderer.quality = v[PREF_QUALITY];
    if (a->renderer.lang.selected != v[PREF_LANGUAGE])
        lang_select(&a->renderer.lang, v[PREF_LANGUAGE]);
    if (!managed(PREF_WINDOW) && a->window) {
        SDL_SetWindowFullscreen(a->window, v[PREF_WINDOW] == 2);
        SDL_SetWindowBordered(a->window, v[PREF_WINDOW] == 0);
        const int w[] = {0, 960, 1280, 1920}, h[] = {0, 540, 720, 1080};
        int res = v[PREF_RESOLUTION];
        if (res && v[PREF_WINDOW] != 2)
            SDL_SetWindowSize(a->window, w[res], h[res]);
        SDL_GL_SetSwapInterval(v[PREF_VSYNC]);
    }
}
float session_gain(const App *a, PrefId id) {
    const int *v = a->desk.current.value;
    return v[PREF_MUTE] ? 0.f : v[PREF_MASTER] * v[id] / 16.f;
}
SDL_Scancode session_key(const App *a, SDL_Scancode physical) {
    for (int i = 0; i < session_commands(); i++) {
        SDL_Scancode k = actions[i].key;
        if (a->desk.current.binding[k] == physical)
            return k;
    }
    for (int i = 0; i < session_commands(); i++)
        if (actions[i].key == physical)
            return SDL_SCANCODE_UNKNOWN;
    return physical;
}
void session_rebind(App *a, SDL_Scancode physical) {
    Session *d = &a->desk;
    d->rebinding = false;
    if (physical == SDL_SCANCODE_ESCAPE)
        return;
    for (int i = 0; i < session_commands(); i++)
        if (i != d->row && d->edit.binding[actions[i].key] == physical) {
            snprintf(d->detail, sizeof d->detail, "Already assigned to %s.", actions[i].title);
            return;
        }
    d->edit.binding[actions[d->row].key] = physical;
    snprintf(d->detail, sizeof d->detail, "Assigned %s. Apply changes in Settings.",
             SDL_GetScancodeName(physical));
}
const char *session_prompt(App *a, SDL_Scancode key, const char *label, char *out, size_t n) {
    if (a->touch || a->xr)
        snprintf(out, n, "%s", lang_text(&a->renderer.lang, label));
    else
        snprintf(out, n, "[%s] %s", SDL_GetScancodeName(a->desk.current.binding[key]),
                 lang_text(&a->renderer.lang, label));
    return out;
}

const char *session_xr_action(int id) {
    static const char *names[] = {"Orders",      "Next row",  "Confirm",      "Interact",
                                  "Operations",  "Crews",     "Back",         "Retry",
                                  "Next target", "Radio log", "Address crew", "Pause"};
    return names[id];
}
const char *session_xr_control(int id) {
    static const char *controls[] = {
        "XR left trigger", "XR left grip",        "XR right trigger", "XR right grip",
        "XR left X",       "XR left Y",           "XR left menu",     "XR right stick press",
        "XR right B",      "XR left stick press", "XR right menu",    "XR both grips"};
    return controls[id];
}
void session_xr_adjust(Session *d, int physical, int delta) {
    int previous = d->edit.xr[physical], next = (previous + delta + XR_BINDINGS) % XR_BINDINGS;
    for (int i = 0; i < XR_BINDINGS; i++)
        if (d->edit.xr[i] == next) {
            d->edit.xr[i] = previous;
            break;
        }
    d->edit.xr[physical] = next;
}
