#include "app.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(e)                                                                                   \
    do {                                                                                           \
        if (!(e)) {                                                                                \
            fprintf(stderr, "Session test line %d: %s\n", __LINE__, #e);                           \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

static void inspect_menu(App *a, DeskPage page) {
    session_menu(a, page);
    char text[1600];
    session_description(a, text, sizeof text);
    CHECK(*session_title(a) && orra_menu_count() == session_count(a));
    CHECK(!*orra_menu_row(-1) && !*orra_menu_row(orra_menu_count()));
    for (int i = 0; i < orra_menu_count(); i++)
        CHECK(*orra_menu_row(i));
    ui_draw(a, 800, 540);
    a->renderer.ui_count = 0;
    ui_wrist(a);
    a->renderer.ui_count = 0;
    session_input(a, SDL_SCANCODE_DOWN);
    session_input(a, SDL_SCANCODE_UP);
    CHECK(a->desk.row == 0);
}
static void slots(App *a) {
    Session *d = &a->desk;
    CHECK(app_mission(a, "gate-six") && d->occupied[2]);
    session_pick(a, 0);
    app_action(a, 1);
    app_tick(a);
    uint32_t digest = sim_digest(&a->sim);
    int tick = a->sim.tick;
    app_key(a, SDL_SCANCODE_M);
    app_key(a, SDL_SCANCODE_2);
    CHECK(d->page == DESK_CONFIRM && strstr(orra_menu_detail(), "tide-window"));
    app_advance(a, .2);
    CHECK(a->sim.tick == tick);
    orra_menu_pick(0);
    CHECK(a->panel == OPERATIONS && sim_digest(&a->sim) == digest);
    app_key(a, SDL_SCANCODE_F5);
    CHECK(d->occupied[0]);
    app_tick(a);
    app_key(a, SDL_SCANCODE_F5);
    CHECK(d->page == DESK_CONFIRM);
    orra_menu_pick(1);
    CHECK(d->page == DESK_CLOSED);
    app_tick(a);
    app_key(a, SDL_SCANCODE_2);
    CHECK(d->page == DESK_CONFIRM);
    orra_menu_pick(1);
    CHECK(!strcmp(a->sim.mission, "tide-window"));
    CHECK(app_slot_info(2, d->detail, sizeof d->detail) && !strcmp(d->detail, "tide-window"));
    CHECK(app_read_slot(a, 0) && !strcmp(a->sim.mission, "gate-six") && a->sim.tick == tick + 1);
    inspect_menu(a, DESK_SAVES);
    session_pick(a, 1);
    CHECK(d->occupied[1]);
    a->save_pending = true;
    CHECK(app_queue_save(a, 1));
    CHECK(!app_queue_save(a, 1));
    session_request(a, DO_LOAD, NULL, 0);
    CHECK(strstr(d->detail, "Wait"));
    a->desk.write_slot = 2;
    app_save_complete(a, true);
    CHECK(!a->desk.queued_save && !a->save_pending);
    inspect_menu(a, DESK_LOAD);
    session_pick(a, 4);
    CHECK(d->page == DESK_CONFIRM);
    session_pick(a, 1);
    CHECK(!d->occupied[1] && d->occupied[0] && d->occupied[2]);
    CHECK(!app_delete_slot(a, -1));
    session_pick(a, 5);
    CHECK(d->page == DESK_PAUSE);
    session_pick(a, 2);
    session_pick(a, 2);
    CHECK(!strcmp(a->sim.mission, "tide-window"));
    session_menu(a, DESK_TITLE);
    d->playing = false;
    session_pick(a, 1);
    CHECK(d->playing);
}
static void training(App *a) {
    Session *d = &a->desk;
    CHECK(app_mission(a, "tide-window"));
    app_tick(a);
    uint32_t digest = sim_digest(&a->sim);
    Camera previous = a->camera;
    session_pause(a);
    session_pick(a, 4);
    CHECK(d->practice && d->page == DESK_HELP);
    CHECK(strstr(orra_menu_detail(), "four alloy"));
    session_pick(a, 0);
    app_action(a, 1);
    app_tick(a);
    session_menu(a, DESK_HELP);
    CHECK(strstr(orra_menu_detail(), "fit"));
    session_pick(a, 1);
    CHECK(strstr(orra_menu_detail(), "winter food"));
    session_pick(a, 1);
    session_pick(a, 0);
    CHECK(sim_step(&a->sim, 125));
    app_key(a, SDL_SCANCODE_H);
    CHECK(strstr(orra_menu_detail(), "mill is ready"));
    session_pick(a, 0);
    int builds[] = {0, 3, 5};
    for (int i = 0; i < 3; i++) {
        app_action(a, builds[i]);
        CHECK(sim_step(&a->sim, 300));
    }
    CHECK(a->sim.blue.armor);
    const char *vehicles[] = {"mole", "lintel"};
    for (int i = 0; i < 2; i++) {
        int index = 0;
        while (index < a->sim.places && strcmp(a->sim.place[index].key, vehicles[i]))
            index++;
        V3 position;
        CHECK(index < a->sim.places && world_object(&a->sim, index, &position));
        Shape shape = world_asset_shape(&a->sim, index);
        a->camera.pos = to_world((V3){0, 0, shape.d * .53f}, position, world_angle(&a->sim, index));
        app_key(a, SDL_SCANCODE_E);
        CHECK(a->camera.aboard == index);
        app_key(a, SDL_SCANCODE_H);
        CHECK(strstr(orra_menu_detail(), i ? "You are aboard" : "rear hatch"));
        session_pick(a, 0);
        if (i == 0) {
            app_key(a, SDL_SCANCODE_E);
            CHECK(a->camera.aboard < 0);
        }
    }
    session_menu(a, DESK_PAUSE);
    session_pick(a, 1);
    CHECK(strstr(orra_menu_detail(), "End practice"));
    session_pick(a, 4);
    CHECK(!d->practice && sim_digest(&a->sim) == digest &&
          !memcmp(&previous, &a->camera, sizeof previous));
    inspect_menu(a, DESK_BRIEF);
    CHECK(strstr(orra_menu_detail(), "Two power cores"));
    session_pick(a, 1);
    CHECK(strstr(orra_menu_detail(), "Progress"));
    session_pick(a, 0);
    app_key(a, SDL_SCANCODE_P);
    session_pick(a, 5);
    CHECK(strstr(orra_menu_detail(), "No completed"));
    session_pick(a, 0);
}
static void settings(App *a) {
    Session *d = &a->desk;
    inspect_menu(a, DESK_SETTINGS);
    for (int i = 0; i < PREF_COUNT; i++) {
        session_pick(a, i);
        char text[180];
        CHECK(*session_row(a, i, text, sizeof text));
    }
    session_pick(a, PREF_COUNT + 2);
    CHECK(d->edit.value[PREF_MASTER] == 4);
    session_pick(a, PREF_SCALE);
    session_pick(a, PREF_SCALE);
    session_pick(a, PREF_MUTE);
    session_pick(a, PREF_SPEED);
    session_pick(a, PREF_LANGUAGE);
    session_pick(a, PREF_COUNT);
    CHECK(d->page == DESK_BINDINGS);
    session_pick(a, 0);
    CHECK(d->rebinding);
    session_input(a, SDL_SCANCODE_S);
    CHECK(strstr(d->detail, "assigned"));
    session_pick(a, 0);
    session_rebind(a, SDL_SCANCODE_ESCAPE);
    session_pick(a, 0);
    SDL_Event bind = {0};
    bind.type = SDL_EVENT_KEY_DOWN;
    bind.key.scancode = SDL_SCANCODE_Q;
    app_event(a, &bind);
    inspect_menu(a, DESK_BINDINGS);
    int xr = session_commands() + 2;
    session_pick(a, xr);
    CHECK(d->edit.xr[2] == 3 && d->edit.xr[3] == 2);
    session_input(a, SDL_SCANCODE_LEFT);
    CHECK(d->edit.xr[2] == 2);
    session_input(a, SDL_SCANCODE_RIGHT);
    CHECK(d->edit.xr[2] == 3);
    CHECK(d->edit.binding[SDL_SCANCODE_W] == SDL_SCANCODE_Q);
    session_input(a, SDL_SCANCODE_ESCAPE);
    CHECK(d->page == DESK_SETTINGS);
    session_pick(a, PREF_COUNT + 1);
    CHECK(d->current.value[PREF_SCALE] == 2 && session_gain(a, PREF_MUSIC) == 0);
    CHECK(session_key(a, SDL_SCANCODE_Q) == SDL_SCANCODE_W &&
          session_key(a, SDL_SCANCODE_W) == SDL_SCANCODE_UNKNOWN);
    CHECK(orra_xr_binding(2) == 3 && orra_xr_binding(-1) == -1);
    CHECK(!strcmp(orra_language_tag(), "ru"));
    CHECK(a->renderer.lang.selected == 1 &&
          strcmp(lang_text(&a->renderer.lang, "desk.title"), "Orra Basin"));
    CHECK(lang_glyph(&a->renderer.lang, 0x420) && !lang_glyph(&a->renderer.lang, 0x9999));
    CHECK(!strcmp(
        lang_text(&a->renderer.lang, "brief.gate-six"),
        "Watch Officer Iva, Weir Assembly. The terraces need a road for winter food. Across Gate "
        "Six, the Compact needs deep water for its supply barges. Commission the Plate Mill, send "
        "the Mole for plate, then choose infantry or armor. Hold the switch room for 60 seconds."));
    char translated[300];
    const Language *language = &a->renderer.lang;
    CHECK(strstr(
        lang_format(language, "Plate Mill queued. Crew assigned.", translated, sizeof translated),
        "Plate Mill."));
    CHECK(strstr(lang_format(language, "CONTROL LOST IN 14S", translated, sizeof translated),
                 "14 с"));
    CHECK(!strcmp(lang_format(language, "CONTROL LOST IN 14?", translated, sizeof translated),
                  "CONTROL LOST IN 14?"));
    CHECK(!strcmp(lang_format(language, "unchanged", translated, sizeof translated), "unchanged"));
    CHECK(strstr(orra_field_label(SDL_SCANCODE_TAB), "Приказы"));
    CHECK(!*orra_field_label(-1));
    render_text(&a->renderer, 0, 0, 1, lang_text(&a->renderer.lang, "desk.title"), 0xffffffff);
    inspect_menu(a, DESK_SETTINGS);
    session_pick(a, PREF_COUNT + 2);
    session_pick(a, PREF_COUNT + 3);
    CHECK(d->current.value[PREF_SCALE] == 2);
    session_defaults(&d->current);
    session_settings(a, false);
    CHECK(d->current.value[PREF_SCALE] == 2);
    session_menu(a, DESK_CLOSED);
    int start = a->sim.tick;
    app_advance(a, 1. / 15);
    CHECK(a->sim.tick == start + 1);
    session_defaults(&d->current);
    session_apply(a);
    session_settings(a, true);
    CHECK(!a->renderer.lang.selected && session_gain(a, PREF_RADIO) == 1);
}
static void recovery_and_controls(App *a) {
    Session *d = &a->desk;
    CHECK(app_mission(a, "gate-six"));
    session_menu(a, DESK_CLOSED);
    int tick = a->sim.tick;
    SDL_strlcpy(a->pending_mission, "high-water", sizeof a->pending_mission);
    app_sector_ready(a, a->sector_request, 0);
    CHECK(d->page == DESK_LOAD_ERROR && a->sim.tick == tick);
    inspect_menu(a, DESK_LOAD_ERROR);
    session_input(a, SDL_SCANCODE_ESCAPE);
    CHECK(!*a->pending_mission);
    SDL_strlcpy(a->pending_mission, "tide-window", sizeof a->pending_mission);
    app_sector_ready(a, a->sector_request, 0);
    session_pick(a, 0);
    CHECK(!strcmp(a->sim.mission, "tide-window"));
    SDL_strlcpy(a->pending_mission, "high-water", sizeof a->pending_mission);
    inspect_menu(a, DESK_LOADING);
    session_input(a, SDL_SCANCODE_ESCAPE);
    CHECK(!*a->pending_mission && d->page == DESK_PAUSE);
    CHECK(app_mission(a, "gate-six"));
    for (int option = 0; option < 3; option++) {
        session_menu(a, DESK_CLOSED);
        a->panel = OPERATIONS;
        a->listing = a->sim.operation_count + option;
        app_tick(a);
        app_choose(a);
        CHECK(d->page == (option == 0 ? DESK_SAVES : option == 1 ? DESK_LOAD : DESK_CONFIRM));
    }
    session_input(a, SDL_SCANCODE_ESCAPE);
    session_menu(a, DESK_PAUSE);
    session_pick(a, 4);
    session_pick(a, 0);
    CHECK(d->guided);
    ui_draw(a, 640, 480);
    a->renderer.ui_count = 0;
    app_key(a, SDL_SCANCODE_F5);
    CHECK(strstr(a->message, "practice"));
    a->panel = OPERATIONS;
    for (int i = 0; i <= 3; i++) {
        a->listing = i == 0 ? 0 : a->sim.operation_count + i;
        app_choose(a);
        CHECK(strstr(a->message, "practice"));
    }
    session_menu(a, DESK_PAUSE);
    session_pick(a, 4);
    session_menu(a, DESK_CLOSED);
    a->panel = FIELD;
    SDL_Event event = {0};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.scancode = SDL_SCANCODE_W;
    d->current.value[PREF_TOGGLE] = 1;
    app_event(a, &event);
    CHECK(a->keys[SDL_SCANCODE_W]);
    event.type = SDL_EVENT_KEY_UP;
    app_event(a, &event);
    CHECK(a->keys[SDL_SCANCODE_W]);
    event.type = SDL_EVENT_KEY_DOWN;
    app_event(a, &event);
    CHECK(!a->keys[SDL_SCANCODE_W]);
    d->current.value[PREF_TOGGLE] = 0;
    d->current.value[PREF_NARRATE] = 1;
    event.key.scancode = SDL_SCANCODE_TAB;
    app_event(a, &event);
    CHECK(a->panel == SLATE);
    d->current.value[PREF_NARRATE] = 0;
    app_key(a, SDL_SCANCODE_L);
    sound_caption(a, "Gate watch", "audio.muster");
    sound_caption(a, "Fitters", "audio.complete");
    app_key(a, SDL_SCANCODE_PAGEDOWN);
    CHECK(a->listing == 7);
    ui_draw(a, 800, 540);
    a->renderer.ui_count = 0;
    CHECK(strstr(orra_menu_row(7), "Fitters"));
    if (a->mix[0] && a->voice && a->tone[2]) {
        SDL_PauseAudioStreamDevice(a->voice);
        SDL_ClearAudioStream(a->voice);
        sound_levels(a);
        float baseline = SDL_GetAudioStreamGain(a->mix[0]);
        CHECK(baseline > 0 && SDL_PutAudioStreamData(a->voice, a->tone[2], a->tone_size[2]));
        sound_tick(a);
        CHECK(SDL_GetAudioStreamGain(a->mix[0]) < baseline * .5f);
        SDL_ClearAudioStream(a->voice);
        sound_tick(a);
        CHECK(SDL_GetAudioStreamGain(a->mix[0]) == baseline);
        SDL_ResumeAudioStreamDevice(a->voice);
    }
    orra_fault("Audio route changed");
    CHECK(strstr(d->diagnostic, "Audio route changed"));
    SDL_strlcpy(a->renderer.error, "Model unavailable", sizeof a->renderer.error);
    app_draw(a);
    CHECK(d->page == DESK_PAUSE && strstr(d->detail, "Model unavailable"));
    session_menu(a, DESK_CLOSED);
}
static void incomplete_language(const char *folder) {
    Language language = {0};
    char path[1400];
    SDL_strlcpy(language.root, folder, sizeof language.root);
    lang_add_list(
        &language,
        "invalid row\nEnglish|en.txt|-\nSample|sample.txt|../absent.font\nMissing|absent.txt|-");
    CHECK(language.available == 3);
    const char *base = "invalid row\nintro|Intro\norder|{1} from {2}.\n";
    const char *local = "order|{2}: {1}\n";
    snprintf(path, sizeof path, "%s/en.txt", folder);
    CHECK(SDL_SaveFile(path, base, strlen(base)));
    snprintf(path, sizeof path, "%s/sample.txt", folder);
    CHECK(SDL_SaveFile(path, local, strlen(local)));
    lang_select(&language, 1);
    char formatted[100];
    CHECK(!strcmp(lang_format(&language, "Steel from Quay.", formatted, sizeof formatted),
                  "Quay: Steel"));
    CHECK(!strcmp(lang_text(&language, "intro"), "Intro"));
    lang_select(&language, 2);
    CHECK(!strcmp(lang_text(&language, "intro"), "Intro"));
    lang_select(&language, 99);
    CHECK(language.selected == 0);
    for (int i = 0; i < 20; i++)
        lang_add_list(&language, "Another|en.txt|-");
    CHECK(language.available == 16);
    lang_close(&language);
}
static void recovery_paths(App *a, const char *folder) {
    Session *d = &a->desk;
    char text[500], bad[1400];
    xr_attach(NULL);
    CHECK(!strcmp(orra_language_tag(), "en"));
    CHECK(!orra_menu_count() && !orra_menu_flags() && !*orra_menu_heading() &&
          !*orra_menu_detail());
    orra_menu_pick(-1);
    xr_attach(a);
    session_menu(a, DESK_TITLE);
    d->playing = false;
    session_pick(a, 2);
    CHECK(d->page == DESK_LOAD);
    session_input(a, SDL_SCANCODE_ESCAPE);
    session_pick(a, 4);
    CHECK(d->page == DESK_SETTINGS);
    d->row = PREF_RESOLUTION;
    session_input(a, SDL_SCANCODE_RIGHT);
    CHECK(d->edit.value[PREF_RESOLUTION] == 1);
    session_pick(a, PREF_COUNT + 1);
    session_input(a, SDL_SCANCODE_ESCAPE);
    session_menu(a, DESK_PAUSE);
    session_pick(a, 3);
    CHECK(d->page == DESK_BRIEF);
    session_menu(a, DESK_PAUSE);
    session_pick(a, 6);
    CHECK(d->page == DESK_SETTINGS);
    session_menu(a, DESK_PAUSE);
    session_pick(a, 9);
    CHECK(d->page == DESK_CREDITS);
    inspect_menu(a, DESK_DOCUMENT);
    CHECK(strstr(orra_menu_detail(), "unavailable"));
    session_pick(a, 2);
    session_pick(a, 2);
    inspect_menu(a, DESK_DOCUMENT);
    char first[800];
    SDL_strlcpy(first, orra_menu_detail(), sizeof first);
    session_pick(a, 1);
    CHECK(*orra_menu_detail() && strcmp(first, orra_menu_detail()));
    session_pick(a, 0);
    CHECK(!strcmp(first, orra_menu_detail()));
    session_menu(a, DESK_PAUSE);
    session_input(a, SDL_SCANCODE_RETURN);
    CHECK(d->page == DESK_CLOSED);
    d->playing = true;
    d->dirty = true;
    session_request(a, DO_RESTART, NULL, 0);
    CHECK(d->page == DESK_CONFIRM);
    inspect_menu(a, DESK_CONFIRM);
    session_input(a, SDL_SCANCODE_ESCAPE);
    CHECK(!*session_row(a, -1, text, sizeof text));
    CHECK(!app_mission(a, "chart-does-not-exist"));
    CHECK(d->page == DESK_LOAD_ERROR);
    app_sector_ready(a, a->sector_request, 1);
    CHECK(d->page == DESK_LOAD_ERROR && *d->detail);
    session_pick(a, 1);
    CHECK(app_mission(a, "gate-six"));
    a->panel = OPERATIONS;
    for (int i = 4; i < 6; i++) {
        session_menu(a, DESK_CLOSED);
        a->listing = a->sim.operation_count + i;
        app_choose(a);
        CHECK(d->page == (i == 4 ? DESK_PAUSE : DESK_HELP));
    }
    session_menu(a, DESK_CLOSED);
    a->panel = CREWS;
    orra_menu_pick(0);
    CHECK(a->panel == CREW_JOB);
    a->panel = FIELD;
    CHECK(!orra_menu_count());
    CHECK(*orra_field_label(SDL_SCANCODE_L) && *orra_field_label(SDL_SCANCODE_E) &&
          *orra_field_label(SDL_SCANCODE_H));
    a->web_controls = true;
    orra_device(1);
    ui_draw(a, 390, 730);
    a->renderer.ui_count = 0;
    ui_tap(a, .97f, .95f);
    CHECK(a->panel == FIELD);
    orra_device(0);
    a->web_controls = false;
    CHECK(session_key(a, SDL_SCANCODE_F12) == SDL_SCANCODE_F12);
    d->current.value[PREF_COMFORT] = 1;
    orra_xr_tick(0, 0, 0, .1f);
    d->current.value[PREF_COMFORT] = 0;
    d->current.value[PREF_TURN] = 2;
    orra_xr_tick(0, 0, 0, .1f);
    orra_menu_pick(-1);
    session_tap(a, 0, 0, 800, 540);
    a->manual_clock = true;
    SDL_Event key = {0};
    key.type = SDL_EVENT_KEY_DOWN;
    key.key.scancode = SDL_SCANCODE_L;
    SDL_PushEvent(&key);
    orra_clock(0);
    CHECK(a->panel == RADIO);
    a->manual_clock = false;
    char invalid[] = "not a saved commander";
    CHECK(!app_restore_bookmark(a, invalid));
    CHECK(app_delete_slot(a, 1));
    snprintf(bad, sizeof bad, "%s/reserve.sav", folder);
    CHECK(SDL_SaveFile(bad, "ORRA1 ffffffff\ncorrupted", 23));
    CHECK(!app_slot_info(1, text, sizeof text));
    CHECK(app_write_slot(a, 0));
    for (int index = 0; index < 2; index++) {
        a->pending_slot_load = index + 1;
        SDL_strlcpy(a->pending_mission, "gate-six", sizeof a->pending_mission);
        app_sector_ready(a, a->sector_request, 1);
        CHECK(d->page == (index == 0 ? DESK_CLOSED : DESK_LOAD));
    }
    a->pending_slot_load = 1;
    session_menu(a, DESK_LOAD_ERROR);
    session_pick(a, 0);
    d->needs_checkpoint = true;
    a->save_pending = true;
    d->write_slot = -1;
    app_save_complete(a, true);
    CHECK(!d->needs_checkpoint && d->occupied[2]);
    a->panel = FIELD;
    session_menu(a, DESK_CLOSED);
    session_defaults(&d->current);
    session_apply(a);
    session_settings(a, true);
}
void session_tests(App *a, const char *root) {
    (void)root;
    char folder[300];
    snprintf(folder, sizeof folder, "session-unit-%llu", (unsigned long long)SDL_GetTicksNS());
    SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "ORRA_SAVE_DIR", folder, true);
    xr_attach(a);
    session_init(a, true);
    CHECK(a->desk.page == DESK_TITLE && !a->desk.playing);
    inspect_menu(a, DESK_TITLE);
    session_pick(a, 0);
    CHECK(a->desk.page == DESK_BRIEF);
    session_pick(a, 0);
    slots(a);
    training(a);
    settings(a);
    recovery_and_controls(a);
    incomplete_language(folder);
    recovery_paths(a, folder);
    inspect_menu(a, DESK_CREDITS);
    for (int i = 0; i < 6; i++) {
        session_pick(a, i);
        CHECK(a->desk.document && *orra_menu_detail());
        session_pick(a, 1);
        session_pick(a, 0);
        session_pick(a, 2);
    }
    session_pick(a, 6);
    CHECK(strstr(a->desk.detail, "Build information"));
    session_report(a, "renderer unavailable");
    CHECK(strstr(a->desk.diagnostic, ORRA_BUILD_ID));
    session_init(a, true);
    CHECK(strstr(orra_menu_detail(), "renderer unavailable"));
    session_pick(a, 5);
    session_pick(a, 7);
    CHECK(a->desk.page == DESK_TITLE);
    session_pick(a, 3);
    CHECK(a->desk.practice);
    session_pick(a, 2);
    CHECK(a->desk.page == DESK_TITLE);
    session_pick(a, 0);
    session_pick(a, 0);
    orra_device(1);
    CHECK(orra_menu_flags() & 2);
    for (int p = SLATE; p <= RADIO; p++) {
        a->panel = (Panel)p;
        CHECK(*orra_menu_heading());
        int n = orra_menu_count();
        for (int i = 0; i < n; i++) {
            orra_menu_row(i);
            orra_menu_detail();
        }
        if (p == CREW_JOB) {
            orra_menu_pick(n - 3);
            orra_menu_pick(n - 2);
        }
        if (p == GUIDE) {
            for (int i = 0; i < 4; i++)
                orra_menu_pick(i);
        }
        if (n)
            orra_menu_pick(n - 1);
    }
    a->panel = SLATE;
    orra_menu_pick(1);
    a->panel = FIELD;
    orra_interrupt();
    CHECK(a->desk.page == DESK_PAUSE);
    orra_xr_tick(1, 1, 1, .5f);
    session_input(a, SDL_SCANCODE_PAGEDOWN);
    session_tap(a, .95f, .98f, 800, 540);
    CHECK(orra_xr_pick(.5f, .98f));
    CHECK(!orra_xr_mark(0, 0, 0, 0, 0, -1));
    orra_menu_key(SDL_SCANCODE_ESCAPE);
    CHECK(a->desk.page == DESK_CLOSED);
    SDL_Event event = {0};
    event.type = SDL_EVENT_WILL_ENTER_BACKGROUND;
    app_event(a, &event);
    CHECK(a->desk.page == DESK_PAUSE);
    session_pick(a, 7);
    if (a->desk.page == DESK_CONFIRM)
        session_pick(a, 1);
    app_tick(a);
    session_pause(a);
    session_pick(a, 8);
    CHECK(a->desk.page == DESK_CONFIRM);
    session_pick(a, 1);
    session_pick(a, 6);
    CHECK(!a->running);
    a->running = true;
    a->desk.enabled = false;
    SDL_UnsetEnvironmentVariable(SDL_GetEnvironment(), "ORRA_SAVE_DIR");
}
