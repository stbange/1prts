#include "app.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static void notice(App *a, const char *text) {
    snprintf(a->message, sizeof a->message, "%s", text);
    a->message_ticks = 180;
}
static bool paths(char slot[1400], char previous[1400], char temporary[1400]) {
#ifdef __EMSCRIPTEN__
    const char *dir = "/orra-saves";
    char *pref = NULL;
#else
    const char *dir = SDL_getenv("ORRA_SAVE_DIR");
    char *pref = dir ? NULL : SDL_GetPrefPath("OrraBasin", "1stprts");
    if (!dir)
        dir = pref;
#endif
    bool ok = dir && strlen(dir) < 1360 && SDL_CreateDirectory(dir);
    if (ok) {
        snprintf(slot, 1400, "%s/command.sav", dir);
        snprintf(previous, 1400, "%s/previous.sav", dir);
        snprintf(temporary, 1400, "%s/command-%llx-%08x.tmp", dir,
                 (unsigned long long)SDL_GetPerformanceCounter(), (unsigned)SDL_rand_bits());
    }
    SDL_free(pref);
    return ok;
}
static uint32_t checksum(const char *text) {
    uint32_t h = 2166136261u;
    for (; *text; text++)
        h = (h ^ (unsigned char)*text) * 16777619u;
    return h;
}
static char *read_slot(const char *name, char **body) {
    SDL_PathInfo info;
    if (!SDL_GetPathInfo(name, &info) || info.size < 16 || info.size > 32 * 1024 * 1024)
        return NULL;
    size_t size;
    char *file = SDL_LoadFile(name, &size);
    if (!file)
        return NULL;
    unsigned hash;
    int n = 0;
    if (sscanf(file, "ORRA1 %8x%n", &hash, &n) == 1 && n == 14 && file[n] == '\n' &&
        strlen(file) == size && checksum(file + n + 1) == hash) {
        *body = file + n + 1;
        return file;
    }
    SDL_free(file);
    return NULL;
}
static bool replace_file(const char *path, const char *temporary, const char *text) {
    bool ok = SDL_SaveFile(temporary, text, strlen(text)) && SDL_RenamePath(temporary, path);
    if (!ok)
        SDL_RemovePath(temporary);
    return ok;
}
bool app_save(App *a) {
    char slot[1400], previous[1400], temporary[1400], head[920];
    if (a->save_pending) {
        notice(a, "Save is still being written.");
        return false;
    }
    if (!paths(slot, previous, temporary)) {
        notice(a, "Save folder unavailable. Check storage permissions.");
        return false;
    }
    Sim *s = &a->sim;
    Camera *c = &a->camera;
    const char *parent = c->aboard < 0 ? "-" : s->place[c->aboard].key;
    const char *member = s->crew_count ? s->place[s->roster[a->member]].key : "-";
    const char *dest = s->place[a->destination].key;
    int header =
        snprintf(head, sizeof head,
                 "%s %s %s %s %s\n%.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g\n%d %d %d %d %d\n",
                 s->mission, parent, member, dest, *a->ground_target ? a->ground_target : "-",
                 c->pos.x, c->pos.y, c->pos.z, c->yaw, c->pitch, c->local.x, c->local.y, c->local.z,
                 a->panel, a->selected, a->listing, a->aiming, a->pointed);
    char *state = sim_text(s, "save");
    if (!state || header < 0 || (size_t)header >= sizeof head) {
        free(state);
        notice(a, "The mission could not be recorded.");
        return false;
    }
    size_t n = strlen(state) + (size_t)header + 16;
    char *file = malloc(n);
    bool ok = false;
    if (file) {
        memcpy(file + 15, head, (size_t)header);
        strcpy(file + 15 + header, state);
        char check[16];
        snprintf(check, sizeof check, "ORRA1 %08x\n", checksum(file + 15));
        memcpy(file, check, 15);
        char *old_body;
        char *old = read_slot(slot, &old_body);
        ok = (!old || replace_file(previous, temporary, old)) && replace_file(slot, temporary, file);
        SDL_free(old);
        free(file);
    }
    free(state);
    if (!ok) {
        notice(a, "Could not write save. Check free storage.");
        return false;
    }
#ifdef __EMSCRIPTEN__
    a->save_pending = true;
    notice(a, "Writing browser save.");
    EM_ASM({ Module.persistSlots(); });
#else
    notice(a, "Operation saved.");
#endif
    return true;
}
static int instance(const Sim *s, const char *key) {
    for (int i = 0; i < s->places; i++)
        if (!strcmp(s->place[i].key, key))
            return i;
    return -1;
}
static bool restore(App *a, char *body, char *why, size_t length) {
    char mission[32], parent[64], member[64], target[64], ground[64];
    Camera c = {0};
    int panel, selected, listing, aiming, pointed, used = 0;
    if (sscanf(body, "%31s %63s %63s %63s %63s %f %f %f %f %f %f %f %f %d %d %d %d %d%n", mission,
               parent, member, target, ground, &c.pos.x, &c.pos.y, &c.pos.z, &c.yaw, &c.pitch,
               &c.local.x, &c.local.y, &c.local.z, &panel, &selected, &listing, &aiming, &pointed,
               &used) != 18 ||
        !isfinite(c.pos.x + c.pos.y + c.pos.z + c.yaw + c.pitch + c.local.x + c.local.y +
                  c.local.z) ||
        panel < FIELD || panel > CHART || selected < 0 || listing < 0 || body[used] != '\n')
        return false;
    Sim *s = calloc(1, sizeof *s);
    Renderer *r = calloc(1, sizeof *r);
    bool ok = s && r && sim_open_mission(s, a->sim.root, mission) &&
              sim_import(s, "restore", body + used + 1);
    int who = -1, dest = -1;
    if (ok) {
        c.aboard = instance(s, parent);
        dest = instance(s, target);
        for (int i = 0; i < s->crew_count; i++)
            if (!strcmp(s->place[s->roster[i]].key, member))
                who = i;
        ok = (c.aboard >= 0 || !strcmp(parent, "-")) && (who >= 0 || !s->crew_count) && dest >= 0 &&
             selected < s->order_count && render_open(r, s);
    }
    if (ok) {
        app_adopt(a, s, r);
        a->camera = c;
        a->member = who < 0 ? 0 : who;
        a->destination = dest;
        a->panel = (Panel)panel;
        a->selected = selected;
        a->listing = listing < app_list_count(a) ? listing : 0;
        a->aiming = aiming != 0;
        a->pointed = pointed != 0;
        if (strcmp(ground, "-"))
            snprintf(a->ground_target, sizeof a->ground_target, "%s", ground);
        a->accumulator = 0;
    } else {
        snprintf(why, length, "%s",
                 s && s->error[0]   ? s->error
                 : r && r->error[0] ? r->error
                                    : "Invalid saved crew or commander pose.");
        if (s)
            sim_close(s);
        if (r)
            render_close(r);
    }
    free(s);
    free(r);
    return ok;
}
bool app_resume(App *a) {
    char slot[1400], previous[1400], temporary[1400],
        why[120] = "No readable save. Save an active operation first.";
    if (a->save_pending) {
        notice(a, "Wait for browser storage to finish.");
        return false;
    }
    if (!paths(slot, previous, temporary)) {
        notice(a, "Cannot open the save folder.");
        return false;
    }
    const char *names[] = {slot, previous};
    for (int i = 0; i < 2; i++) {
        char *body = NULL, *file = read_slot(names[i], &body);
        bool ok = file && restore(a, body, why, sizeof why);
        SDL_free(file);
        if (ok) {
            notice(a, i ? "Recovered previous save." : "Operation resumed.");
            return true;
        }
    }
    notice(a, why);
    return false;
}
bool app_continue(App *a) {
    char *next = sim_text(&a->sim, "next"), *ledger = sim_text(&a->sim, "ledger");
    bool ok = next && *next && ledger && app_stage(a, next, ledger);
    free(next);
    free(ledger);
    notice(a, ok ? "Campaign watch reported for duty." : "No linked operation available.");
    return ok;
}
