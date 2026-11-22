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
static const char *slot_files[] = {"command.sav", "reserve.sav", "checkpoint.sav"};
static const char *backup_files[] = {"previous.sav", "reserve-prev.sav", "checkpoint-prev.sav"};

bool app_data_path(const char *name, char *out, size_t capacity) {
#ifdef __EMSCRIPTEN__
    const char *dir = "/orra-saves";
    char *pref = NULL;
#else
    const char *dir = SDL_getenv("ORRA_SAVE_DIR");
    char *pref = dir ? NULL : SDL_GetPrefPath("OrraBasin", "1stprts");
    if (!dir)
        dir = pref;
#endif
    bool ok = dir && SDL_CreateDirectory(dir) &&
              snprintf(out, capacity, "%s/%s", dir, name) < (int)capacity;
    SDL_free(pref);
    return ok;
}
static bool paths(int index, char slot[1400], char previous[1400], char temporary[1400]) {
    if (index < 0 || index >= ORRA_SLOT_COUNT)
        return false;
    char scratch[100];
    snprintf(scratch, sizeof scratch, "command-%llx-%08x.tmp",
             (unsigned long long)SDL_GetPerformanceCounter(), (unsigned)SDL_rand_bits());
    return app_data_path(slot_files[index], slot, 1400) &&
           app_data_path(backup_files[index], previous, 1400) &&
           app_data_path(scratch, temporary, 1400);
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
char *app_bookmark(App *a) {
    char head[920];
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
        return NULL;
    }
    size_t n = strlen(state) + (size_t)header + 1;
    char *body = malloc(n);
    if (body)
        snprintf(body, n, "%s%s", head, state);
    free(state);
    return body;
}
bool app_slot_info(int index, char *label, size_t cap) {
    char path[1400], backup[1400], temp[1400], *body = NULL;
    if (!paths(index, path, backup, temp))
        return false;
    char *file = read_slot(path, &body);
    if (!file)
        file = read_slot(backup, &body);
    if (file) {
        snprintf(label, cap, "%.*s", (int)strcspn(body, " "), body);
        SDL_free(file);
        return true;
    }
    return false;
}
static bool write_record(App *a, int index, char *body, int tick) {
    char slot[1400], previous[1400], temporary[1400];
    if (!paths(index, slot, previous, temporary)) {
        free(body);
        notice(a, "Save folder unavailable. Check storage permissions.");
        return false;
    }
    size_t n = strlen(body) + 16;
    char *file = malloc(n);
    bool ok = false;
    if (file) {
        snprintf(file, n, "ORRA1 %08x\n%s", checksum(body), body);
        char *old_body;
        char *old = read_slot(slot, &old_body);
        ok = (!old || replace_file(previous, temporary, old)) && replace_file(slot, temporary, file);
        SDL_free(old);
        free(file);
    }
    free(body);
    if (!ok) {
        notice(a, "Could not write save. Check free storage.");
        return false;
    }
#ifdef __EMSCRIPTEN__
    a->save_pending = true;
    a->desk.write_slot = index;
    a->desk.write_tick = tick;
    notice(a, index == 2 ? "Writing checkpoint." : "Writing browser save.");
    EM_ASM({ Module.persistSlots($0); }, index);
#else
    a->desk.write_slot = index;
    a->desk.write_tick = tick;
    app_save_complete(a, true);
#endif
    return true;
}
bool app_write_slot(App *a, int index) {
    if (a->save_pending) {
        notice(a, "Save is still being written.");
        return false;
    }
    char *body = app_bookmark(a);
    if (!body) {
        notice(a, "The mission could not be recorded.");
        return false;
    }
    return write_record(a, index, body, a->sim.tick);
}
bool app_queue_save(App *a, int slot) {
    if (!a->save_pending)
        return app_write_slot(a, slot);
    if (a->desk.queued_save) {
        notice(a, "A manual save is already queued.");
        return false;
    }
    a->desk.queued_save = app_bookmark(a);
    if (!a->desk.queued_save)
        return false;
    a->desk.queued_slot = slot;
    a->desk.queued_tick = a->sim.tick;
    notice(a, "Manual save queued after checkpoint.");
    return true;
}
void app_save_complete(App *a, bool success) {
    Session *d = &a->desk;
    a->save_pending = false;
    if (success && d->write_slot >= 0) {
        d->recent = d->write_slot;
        if (d->write_slot != 2) {
            d->saved_tick = d->write_tick;
            d->dirty = a->sim.tick != d->saved_tick;
        }
        char path[1400], value[8];
        snprintf(value, sizeof value, "%d", d->recent);
        if (app_data_path("recent.txt", path, sizeof path))
            SDL_SaveFile(path, value, strlen(value));
#ifdef __EMSCRIPTEN__
        EM_ASM({ Module.storePreference("recent.txt"); });
#endif
    }
    notice(a, !success            ? "Storage write failed. Check free space or browser permissions."
              : d->write_slot < 0 ? "Slot cleared."
              : d->write_slot == 2 ? "Checkpoint committed."
                                   : "Operation saved.");
    session_check_slots(a);
    if (d->queued_save) {
        char *body = d->queued_save;
        d->queued_save = NULL;
        write_record(a, d->queued_slot, body, d->queued_tick);
    } else if (d->needs_checkpoint && d->enabled && !d->practice) {
        d->needs_checkpoint = false;
        app_write_slot(a, 2);
    }
}
bool app_save(App *a) { return app_write_slot(a, a->desk.slot); }
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
bool app_restore_bookmark(App *a, char *body) {
    char why[120] = "Invalid practice return point.";
    if (restore(a, body, why, sizeof why))
        return true;
    notice(a, why);
    return false;
}
bool app_read_slot(App *a, int index) {
    char slot[1400], previous[1400], temporary[1400],
        why[120] = "No readable save. Save an active operation first.";
    if (a->save_pending) {
        notice(a, "Wait for browser storage to finish.");
        return false;
    }
    if (!paths(index, slot, previous, temporary)) {
        notice(a, "Cannot open the save folder.");
        return false;
    }
#ifdef __EMSCRIPTEN__
    char mission[80];
    if (app_slot_info(index, mission, sizeof mission) &&
        !EM_ASM_INT(
            { return Module.requestSector(UTF8ToString($0), $1); }, mission, ++a->sector_request)) {
        SDL_strlcpy(a->pending_mission, mission, sizeof a->pending_mission);
        a->pending_slot_load = index + 1;
        session_menu(a, DESK_LOADING);
        return true;
    }
#endif
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
bool app_resume(App *a) { return app_read_slot(a, a->desk.slot); }
bool app_delete_slot(App *a, int index) {
    char slot[1400], backup[1400], tmp[1400];
    if (a->save_pending || !paths(index, slot, backup, tmp))
        return false;
    SDL_PathInfo info;
    if ((SDL_GetPathInfo(backup, &info) && !SDL_RemovePath(backup)) ||
        (SDL_GetPathInfo(slot, &info) && !SDL_RemovePath(slot)))
        return false;
#ifdef __EMSCRIPTEN__
    a->save_pending = true;
    a->desk.write_slot = -1;
    EM_ASM({ Module.persistSlots($0); }, index);
#endif
    return true;
}
bool app_continue(App *a) {
    char *next = sim_text(&a->sim, "next"), *ledger = sim_text(&a->sim, "ledger");
    bool ok = next && *next && ledger && app_stage(a, next, ledger);
    free(next);
    free(ledger);
    notice(a, ok ? "Campaign watch reported for duty." : "No linked operation available.");
    return ok;
}
