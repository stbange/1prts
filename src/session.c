#include "app.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static const char *const title_rows[] = {"desk.new",      "desk.continue", "desk.load",
                                         "desk.practice", "desk.settings", "desk.credits",
                                         "desk.quit"};
static const char *const pause_rows[] = {
    "desk.resume",   "desk.save",    "desk.load", "desk.brief",   "desk.practice", "desk.record",
    "desk.settings", "desk.restart", "desk.home", "desk.credits", "desk.quit"};
static const char *const credit_rows[] = {"desk.notes",
                                          "ui.original-art-and-voices",
                                          "ui.mit-code-license",
                                          "ui.cc-by-4-0-art-license",
                                          "ui.sdl-license",
                                          "ui.lua-license",
                                          "desk.copy",
                                          "desk.back"};
static const char *const documents[] = {
    "assets/release.txt",     "assets/CREDITS.md", "LICENSE",
    "LICENSES/CC-BY-4.0.txt", "LICENSES/SDL3.txt", "LICENSES/Lua.txt"};
static const char *const names[] = {"",
                                    "desk.title",
                                    "desk.pause",
                                    "ui.confirm-action",
                                    "desk.save",
                                    "desk.load",
                                    "desk.settings",
                                    "desk.controls",
                                    "desk.brief",
                                    "desk.record",
                                    "desk.credits",
                                    "ui.release-document",
                                    "desk.help",
                                    "ui.operation-unavailable",
                                    "Loading operation"};
static const char *const slots[] = {"Watch A", "Watch B", "Checkpoint"};
static const char *tr(const App *a, const char *key) { return lang_text(&a->renderer.lang, key); }

void session_speak(App *a, const char *text) {
    if (!a->desk.current.value[PREF_NARRATE] || !text || !*text)
        return;
#ifdef __EMSCRIPTEN__
    EM_ASM({ Module.speak ?.(UTF8ToString($0)); }, text);
#else
    const char *platform = SDL_GetPlatform();
    const char *const mac[] = {"say", "--", text, NULL};
    const char *const linux_voice[] = {"spd-say", "--", text, NULL};
    const char *const ng[] = {"espeak-ng", "--", text, NULL};
    const char *const fallback[] = {"espeak", "--", text, NULL};
    const char *const windows[] = {
        "powershell",
        "-NoProfile",
        "-NonInteractive",
        "-Command",
        "Add-Type -AssemblyName System.Speech; (New-Object "
        "System.Speech.Synthesis.SpeechSynthesizer).Speak([Console]::In.ReadToEnd())",
        NULL};
    bool win = !strcmp(platform, "Windows");
    SDL_Process *p = SDL_CreateProcess(win                          ? windows
                                       : !strcmp(platform, "macOS") ? mac
                                                                    : linux_voice,
                                       win);
    if (!p && !win && strcmp(platform, "macOS")) {
        p = SDL_CreateProcess(ng, false);
        if (!p)
            p = SDL_CreateProcess(fallback, false);
    }
    if (!p)
        SDL_strlcpy(a->desk.detail, "System speech unavailable. Use the text on the slate.",
                    sizeof a->desk.detail);
    if (p && win) {
        SDL_IOStream *input = SDL_GetProcessInput(p);
        if (input) {
            SDL_WriteIO(input, text, strlen(text));
            SDL_CloseIO(input);
        }
    }
    if (p)
        SDL_DestroyProcess(p);
#endif
}
void session_check_slots(App *a) {
    for (int i = 0; i < ORRA_SLOT_COUNT; i++) {
        a->desk.slot_title[i][0] = 0;
        a->desk.occupied[i] = app_slot_info(i, a->desk.slot_title[i], sizeof a->desk.slot_title[i]);
    }
}
static void release_input(App *a) {
    memset(a->keys, 0, sizeof a->keys);
    a->left_down = a->right_down = a->captured = false;
    a->stride_x = a->stride_y = a->stick_x = a->stick_y = 0;
    a->accumulator = 0;
    SDL_SetWindowRelativeMouseMode(a->window, false);
}
void session_menu(App *a, DeskPage page) {
    Session *d = &a->desk;
    release_input(a);
    d->page = page;
    d->row = 0;
    d->rebinding = false;
    d->detail[0] = 0;
    if (page == DESK_TITLE || page == DESK_LOAD || page == DESK_SAVES)
        session_check_slots(a);
    if (page == DESK_SETTINGS)
        d->edit = d->current;
    session_speak(a, session_title(a));
}
void session_pause(App *a) {
    if (!a->desk.enabled)
        return;
    release_input(a);
    if (a->desk.page == DESK_CLOSED)
        session_menu(a, DESK_PAUSE);
}
void session_report(App *a, const char *category) {
    char path[1400];
    snprintf(a->desk.diagnostic, sizeof a->desk.diagnostic, "Build %s / %s / %s / %.220s",
             ORRA_BUILD_ID, SDL_GetPlatform(), a->sim.mission, category);
    if (app_data_path("failure.txt", path, sizeof path))
        SDL_SaveFile(path, a->desk.diagnostic, strlen(a->desk.diagnostic));
#ifdef __EMSCRIPTEN__
    EM_ASM({ Module.storePreference("failure.txt"); });
#endif
}
void session_init(App *a, bool desk) {
    Session *d = &a->desk;
    d->enabled = true;
    char recent_file[1400];
    if (app_data_path("recent.txt", recent_file, sizeof recent_file)) {
        char *last = SDL_LoadFile(recent_file, NULL);
        if (last) {
            d->recent = SDL_clamp(atoi(last), 0, 2);
            SDL_free(last);
        }
    }
    session_settings(a, false);
    session_apply(a);
    d->playing = !desk;
    char path[1400];
    if (app_data_path("failure.txt", path, sizeof path)) {
        char *old = SDL_LoadFile(path, NULL);
        if (old) {
            SDL_strlcpy(d->diagnostic, old, sizeof d->diagnostic);
            SDL_free(old);
        }
    }
    if (desk)
        session_menu(a, DESK_TITLE);
}
void session_end(App *a) {
    free(a->desk.queued_save);
    a->desk.queued_save = NULL;
    free(a->desk.practice);
    a->desk.practice = NULL;
    SDL_free(a->desk.document);
    a->desk.document = NULL;
}
void session_entered(App *a) {
    Session *d = &a->desk;
    if (!d->enabled)
        return;
    d->playing = true;
    d->dirty = false;
    d->saved_tick = a->sim.tick;
    session_menu(a, DESK_CLOSED);
    if (d->practice)
        return;
    d->needs_checkpoint = a->save_pending;
    if (!a->save_pending)
        app_write_slot(a, 2);
}
const char *session_title(const App *a) { return tr(a, names[a->desk.page]); }
int session_count(const App *a) {
    switch (a->desk.page) {
    case DESK_LOADING:
        return 1;
    case DESK_TITLE:
        return SDL_arraysize(title_rows);
    case DESK_PAUSE:
        return SDL_arraysize(pause_rows);
    case DESK_CONFIRM:
    case DESK_LOAD_ERROR:
        return 2;
    case DESK_SAVES:
        return 5;
    case DESK_LOAD:
        return 6;
    case DESK_SETTINGS:
        return PREF_COUNT + 4;
    case DESK_BINDINGS:
        return session_commands() + XR_BINDINGS + 1;
    case DESK_CREDITS:
        return SDL_arraysize(credit_rows);
    case DESK_DOCUMENT:
        return 3;
    case DESK_HELP:
    case DESK_BRIEF:
        return 3;
    case DESK_RECORD:
        return 1;
    default:
        return 0;
    }
}
static void briefing(const App *a, char *out, size_t n) {
    const Sim *g = &a->sim;
    char key[80];
    snprintf(key, sizeof key, "brief.%s", g->mission);
    const char *text = tr(a, key);
    snprintf(out, n, "%s", !strcmp(text, key) ? g->objective : text);
}
static const char *lesson(const App *a) {
    const Side *s = &a->sim.blue;
    if (a->camera.aboard >= 0 && s->armor)
        return "lesson.aboard";
    if (s->armor)
        return "lesson.tank";
    if (s->mill)
        return "lesson.haul";
    if (s->work_left)
        return "lesson.build";
    return "lesson.start";
}
static void help(const App *a, char *out, size_t n) {
    const Sim *g = &a->sim;
    const char *next = NULL;
    for (int i = 0; i < g->order_count; i++)
        if (!strcmp(g->advice, g->orders[i].id))
            next = g->orders[i].label;
    if (!strcmp(g->mission, "gate-six"))
        snprintf(out, n, "%s %s", tr(a, lesson(a)), g->report);
    else
        snprintf(out, n, "%s / Progress %d. %s. %s", g->objective, g->progress,
                 next ? next : g->stage, g->report);
}
static void document_page(const App *a, char *out, size_t n) {
    const char *s = a->desk.document;
    if (!s) {
        SDL_strlcpy(out, "Document unavailable in this distribution.", n);
        return;
    }
    for (int i = 0; i < a->desk.document_line && *s; i++)
        (void)SDL_StepUTF8(&s, NULL);
    const char *end = s;
    for (int i = 0; i < 160 && *end; i++)
        (void)SDL_StepUTF8(&end, NULL);
    snprintf(out, n, "%.*s", (int)(end - s), s);
}
static void description(App *a, char *out, size_t n) {
    Session *d = &a->desk;
    if (d->detail[0]) {
        SDL_strlcpy(out, d->detail, n);
        return;
    }
    if (d->page == DESK_CONFIRM) {
        snprintf(out, n, "%s. Unsaved changes to the named operation or slot may be lost.",
                 d->pending_name);
        return;
    }
    if (d->page == DESK_DOCUMENT) {
        document_page(a, out, n);
        return;
    }
    if (d->page == DESK_BRIEF) {
        briefing(a, out, n);
        return;
    }
    if (d->page == DESK_HELP) {
        help(a, out, n);
        return;
    }
    if (d->page == DESK_RECORD) {
        char *record = sim_text(&a->sim, "journal");
        snprintf(out, n, "%s%s",
                 !strcmp(a->sim.mission, "high-water") && !strcmp(a->sim.result, "victory")
                     ? tr(a, "ending.high-water")
                     : "",
                 record ? record : "");
        free(record);
        return;
    }
    if (d->page == DESK_CREDITS) {
        snprintf(out, n, "%s / Build %s", tr(a, "credits.summary"), ORRA_BUILD_ID);
        return;
    }
    if (d->page == DESK_TITLE) {
        snprintf(out, n, "%s",
                 *d->diagnostic ? d->diagnostic : "Command the Orra Basin yards from the field.");
        return;
    }
    snprintf(out, n, "%s", a->message_ticks ? a->message : "");
}
void session_description(App *a, char *out, size_t n) { description(a, out, n); }
const char *session_row(App *a, int i, char *out, size_t n) {
    Session *d = &a->desk;
    if (i < 0 || i >= session_count(a))
        return "";
    const char *key = "desk.back";
    switch (d->page) {
    case DESK_TITLE:
        key = title_rows[i];
        break;
    case DESK_PAUSE:
        key = (i == 4 && d->practice) ? "desk.endpractice" : pause_rows[i];
        break;
    case DESK_LOADING:
        key = "desk.cancel";
        break;
    case DESK_LOAD_ERROR:
        key = i ? "desk.cancel" : "Retry loading";
        break;
    case DESK_CONFIRM:
        key = i ? "desk.confirm" : "desk.cancel";
        break;
    case DESK_SETTINGS:
        if (i < PREF_COUNT)
            return session_pref_label(a, i, out, n);
        key = (const char *[]){"desk.controls", "desk.apply", "desk.reset",
                               "desk.cancel"}[i - PREF_COUNT];
        break;
    case DESK_BINDINGS:
        if (i < session_commands()) {
            SDL_Scancode k;
            const char *label = session_command(i, &k);
            snprintf(out, n, "%s: %s", tr(a, label), SDL_GetScancodeName(d->edit.binding[k]));
            return out;
        } else if (i < session_commands() + XR_BINDINGS) {
            int physical = i - session_commands();
            snprintf(out, n, "%s: %s", session_xr_control(physical),
                     tr(a, session_xr_action(d->edit.xr[physical])));
            return out;
        }
        break;
    case DESK_SAVES:
    case DESK_LOAD: {
        int count = d->page == DESK_LOAD ? 3 : 2;
        if (i == session_count(a) - 1)
            break;
        int slot = i < count ? i : i - count;
        snprintf(out, n, "%s%s / %s", i < count ? "" : "Delete ", slots[slot],
                 d->occupied[slot] ? d->slot_title[slot] : "empty");
        return out;
    }
    case DESK_CREDITS:
        key = credit_rows[i];
        break;
    case DESK_DOCUMENT:
        key = (const char *[]){"desk.prev", "desk.next", "desk.back"}[i];
        break;
    case DESK_BRIEF:
    case DESK_HELP:
        key = i == 0   ? "desk.newtask"
              : i == 1 ? (d->page == DESK_HELP ? "desk.brief" : "desk.help")
                       : (d->practice ? "desk.endpractice" : "desk.practice");
        break;
    default:
        break;
    }
    return tr(a, key);
}
static void perform(App *a, DeskAction action, const char *target, int slot) {
    Session *d = &a->desk;
    bool success = true;
    if (action == DO_MISSION)
        success = app_mission(a, target);
    if (action == DO_RESTART)
        success = app_retry(a);
    if (action == DO_DESK || action == DO_EXIT) {
        d->playing = false;
        if (action == DO_EXIT) {
#ifdef __EMSCRIPTEN__
            session_menu(a, DESK_TITLE);
            EM_ASM({ location.assign("release.html"); });
#else
            a->running = false;
#endif
        } else
            session_menu(a, DESK_TITLE);
        return;
    }
    if (action == DO_SAVE) {
        success = app_queue_save(a, slot);
        if (success)
            d->slot = slot;
    }
    if (action == DO_LOAD) {
        success = app_read_slot(a, slot);
        if (success && slot != 2)
            d->slot = slot;
    }
    if (action == DO_DELETE) {
        success = app_delete_slot(a, slot);
        session_check_slots(a);
        snprintf(d->detail, sizeof d->detail, "%s",
                 success ? "Slot cleared." : "Could not clear slot.");
        return;
    }
    if (success) {
        if (d->page == DESK_LOADING)
            return;
        d->playing = true;
        if (action == DO_LOAD) {
            d->dirty = false;
            d->saved_tick = a->sim.tick;
        }
        session_menu(a, DESK_CLOSED);
    } else
        SDL_strlcpy(d->detail, a->message, sizeof d->detail);
}
void session_request(App *a, DeskAction action, const char *target, int slot) {
    Session *d = &a->desk;
    if (a->save_pending && (action == DO_DELETE || action == DO_LOAD)) {
        SDL_strlcpy(d->detail, "Wait for the current storage operation.", sizeof d->detail);
        return;
    }
    bool guard = action == DO_DELETE || (action == DO_SAVE && d->occupied[slot]) ||
                 (action != DO_SAVE && d->playing && (d->dirty || a->sim.tick != d->saved_tick));
    if (!guard) {
        perform(a, action, target, slot);
        return;
    }
    d->cancel_page = d->page;
    d->pending = action;
    d->pending_slot = slot;
    SDL_strlcpy(d->target, target ? target : "", sizeof d->target);
    const char *verb = action == DO_MISSION   ? "Switch to"
                       : action == DO_LOAD    ? "Load"
                       : action == DO_SAVE    ? "Overwrite"
                       : action == DO_DELETE  ? "Delete"
                       : action == DO_RESTART ? "Restart"
                                              : "Leave";
    snprintf(d->pending_name, sizeof d->pending_name, "%s %s", verb,
             target && *target                                               ? target
             : action == DO_SAVE || action == DO_LOAD || action == DO_DELETE ? slots[slot]
                                                                             : a->sim.title);
    session_menu(a, DESK_CONFIRM);
}
static void practice(App *a) {
    Session *d = &a->desk;
    if (d->practice) {
        if (app_restore_bookmark(a, d->practice)) {
            free(d->practice);
            d->practice = NULL;
            d->guided = false;
            d->dirty = d->practice_dirty;
            d->saved_tick = d->practice_tick;
            session_menu(a, d->practice_guide ? DESK_TITLE : DESK_CLOSED);
            d->playing = !d->practice_guide;
        }
        return;
    }
    d->practice = app_bookmark(a);
    if (!d->practice)
        return;
    d->practice_dirty = d->dirty;
    d->practice_tick = d->saved_tick;
    d->practice_guide = !d->playing;
    if (!app_mission(a, "gate-six")) {
        free(d->practice);
        d->practice = NULL;
        return;
    }
    d->guided = true;
    session_menu(a, DESK_HELP);
}
static void root_menu(App *a) { session_menu(a, a->desk.playing ? DESK_PAUSE : DESK_TITLE); }
static void start_watch(App *a) {
    if (!app_mission(a, "gate-six"))
        return;
    session_menu(a, DESK_BRIEF);
}
void session_pick(App *a, int i) {
    Session *d = &a->desk;
    if (i < 0 || i >= session_count(a))
        return;
    d->row = i;
    switch (d->page) {
    case DESK_TITLE:
        if (i == 0)
            start_watch(a);
        else if (i == 1) {
            int slot = d->occupied[d->recent] ? d->recent
                       : d->occupied[0]       ? 0
                       : d->occupied[1]       ? 1
                                              : 2;
            session_request(a, DO_LOAD, NULL, slot);
        } else if (i == 2)
            session_menu(a, DESK_LOAD);
        else if (i == 3)
            practice(a);
        else if (i == 4)
            session_menu(a, DESK_SETTINGS);
        else if (i == 5)
            session_menu(a, DESK_CREDITS);
        else
            perform(a, DO_EXIT, NULL, 0);
        break;
    case DESK_PAUSE:
        if (i == 0)
            session_menu(a, DESK_CLOSED);
        else if (i == 1 || i == 2) {
            if (d->practice)
                SDL_strlcpy(d->detail, "End practice to use campaign saves.", sizeof d->detail);
            else
                session_menu(a, i == 1 ? DESK_SAVES : DESK_LOAD);
        } else if (i == 3)
            session_menu(a, DESK_BRIEF);
        else if (i == 4)
            practice(a);
        else if (i == 5)
            session_menu(a, DESK_RECORD);
        else if (i == 6)
            session_menu(a, DESK_SETTINGS);
        else if (i == 9)
            session_menu(a, DESK_CREDITS);
        else if (d->practice && i != 7)
            practice(a);
        else
            session_request(a, i == 7 ? DO_RESTART : i == 8 ? DO_DESK : DO_EXIT, NULL, 0);
        break;
    case DESK_LOADING:
        ++a->sector_request;
        a->pending_mission[0] = 0;
        a->pending_slot_load = 0;
        SDL_free(a->pending_ledger);
        a->pending_ledger = NULL;
        session_menu(a, d->playing ? DESK_PAUSE : DESK_TITLE);
        break;
    case DESK_LOAD_ERROR: {
        int loading_slot = a->pending_slot_load;
        char id[32];
        SDL_strlcpy(id, a->pending_mission, sizeof id);
        char *ledger = a->pending_ledger ? SDL_strdup(a->pending_ledger) : NULL;
        session_menu(a, DESK_CLOSED);
        if (i == 0 && loading_slot)
            app_read_slot(a, loading_slot - 1);
        else if (i == 0 && *id) {
#ifdef __EMSCRIPTEN__
            EM_ASM({ Module.invalidateSector(UTF8ToString($0)); }, id);
#endif
            app_stage(a, id, ledger);
        } else {
            session_menu(a, d->playing ? DESK_PAUSE : DESK_TITLE);
            ++a->sector_request;
            a->pending_slot_load = 0;
            a->pending_mission[0] = 0;
            SDL_free(a->pending_ledger);
            a->pending_ledger = NULL;
        }
        SDL_free(ledger);
        break;
    }
    case DESK_CONFIRM: {
        DeskAction action = d->pending;
        session_menu(a, d->cancel_page);
        d->pending = DO_NONE;
        if (i)
            perform(a, action, d->target, d->pending_slot);
        break;
    }
    case DESK_LOAD:
    case DESK_SAVES: {
        int count = d->page == DESK_LOAD ? 3 : 2;
        if (i == session_count(a) - 1)
            root_menu(a);
        else if (i < count)
            session_request(a, d->page == DESK_LOAD ? DO_LOAD : DO_SAVE, NULL, i);
        else if (d->occupied[i - count])
            session_request(a, DO_DELETE, NULL, i - count);
        break;
    }
    case DESK_SETTINGS:
        if (i < PREF_COUNT)
            session_adjust(a, 1);
        else if (i == PREF_COUNT)
            session_menu(a, DESK_BINDINGS);
        else if (i == PREF_COUNT + 2)
            session_defaults(&d->edit);
        else {
            if (i == PREF_COUNT + 1) {
                d->current = d->edit;
                session_apply(a);
                session_settings(a, true);
            }
            root_menu(a);
        }
        break;
    case DESK_BINDINGS:
        if (i == session_commands() + XR_BINDINGS) {
            Preferences edited = d->edit;
            session_menu(a, DESK_SETTINGS);
            d->edit = edited;
        } else if (i >= session_commands())
            session_xr_adjust(d, i - session_commands(), 1);
        else {
            d->rebinding = true;
            SDL_strlcpy(d->detail, "Press a new key. Escape cancels.", sizeof d->detail);
        }
        break;
    case DESK_BRIEF:
    case DESK_HELP:
        if (i == 0)
            session_menu(a, DESK_CLOSED);
        else if (i == 1)
            session_menu(a, d->page == DESK_HELP ? DESK_BRIEF : DESK_HELP);
        else
            practice(a);
        break;
    case DESK_CREDITS:
        if (i == 7)
            root_menu(a);
        else if (i == 6) {
            const char *report = d->diagnostic[0] ? d->diagnostic : ORRA_BUILD_ID;
            if (SDL_SetClipboardText(report))
                SDL_strlcpy(d->detail, "Build information copied.", sizeof d->detail);
            else
                snprintf(d->detail, sizeof d->detail, "Build information: %s", report);
        } else {
            char path[1400];
            snprintf(path, sizeof path, "%s/%s", a->sim.root, documents[i]);
            SDL_free(d->document);
            d->document = SDL_LoadFile(path, NULL);
            d->document_line = 0;
            session_menu(a, DESK_DOCUMENT);
        }
        break;
    case DESK_DOCUMENT:
        if (i == 2)
            session_menu(a, DESK_CREDITS);
        else {
            int length = d->document ? (int)SDL_utf8strlen(d->document) : 0;
            d->document_line = SDL_clamp(d->document_line + (i ? 160 : -160), 0,
                                         160 * (SDL_max(0, length - 1) / 160));
        }
        break;
    case DESK_RECORD:
        root_menu(a);
        break;
    default:
        break;
    }
}
bool session_input(App *a, SDL_Scancode k) {
    Session *d = &a->desk;
    if (!d->enabled)
        return false;
    if (d->page == DESK_CLOSED) {
        if (k == SDL_SCANCODE_P || (k == SDL_SCANCODE_ESCAPE && a->panel == FIELD && !a->aiming))
            session_pause(a);
        else if (k == SDL_SCANCODE_H)
            session_menu(a, DESK_HELP);
        else
            return false;
        return true;
    }
    if (d->rebinding) {
        session_rebind(a, k);
        return true;
    }
    int count = session_count(a);
    if (k == SDL_SCANCODE_RETURN || k == SDL_SCANCODE_SPACE)
        session_pick(a, d->row);
    else if (k == SDL_SCANCODE_ESCAPE || k == SDL_SCANCODE_P) {
        if (d->page == DESK_CONFIRM)
            session_pick(a, 0);
        else if (d->page == DESK_LOADING)
            session_pick(a, 0);
        else if (d->page == DESK_LOAD_ERROR)
            session_pick(a, 1);
        else if (d->page == DESK_PAUSE || d->page == DESK_HELP || d->page == DESK_BRIEF)
            session_menu(a, DESK_CLOSED);
        else if (d->page == DESK_BINDINGS || d->page == DESK_DOCUMENT)
            session_pick(a, count - 1);
        else
            root_menu(a);
    } else if (k == SDL_SCANCODE_UP || k == SDL_SCANCODE_DOWN || k == SDL_SCANCODE_PAGEDOWN) {
        d->row = (d->row + count +
                  (k == SDL_SCANCODE_UP     ? -1
                   : k == SDL_SCANCODE_DOWN ? 1
                                            : 4)) %
                 count;
        char row[200];
        session_speak(a, session_row(a, d->row, row, sizeof row));
    } else if (k == SDL_SCANCODE_LEFT || k == SDL_SCANCODE_RIGHT) {
        int delta = k == SDL_SCANCODE_LEFT ? -1 : 1;
        if (d->page == DESK_SETTINGS)
            session_adjust(a, delta);
        if (d->page == DESK_BINDINGS && d->row >= session_commands() &&
            d->row < session_commands() + XR_BINDINGS)
            session_xr_adjust(d, d->row - session_commands(), delta);
    }
    return true;
}
