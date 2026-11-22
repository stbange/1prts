#include "app.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static void tell(App *a, const char *msg) {
    snprintf(a->message, sizeof a->message, "%s", msg);
    a->message_ticks = 95;
}
static void capture_mouse(App *a, bool on) {
    a->captured = on;
    SDL_SetWindowRelativeMouseMode(a->window, on);
}
static void enter_sector(App *a) {
#ifdef __EMSCRIPTEN__
    EM_ASM({ Module.currentMission = UTF8ToString($0); }, a->sim.mission);
#endif
    camera_reset(&a->camera);
    a->camera.pos = a->sim.spawn;
    a->camera.yaw = a->sim.spawn_yaw;
    if (!strcmp(a->sim.mission,"compact-bank")) {
        for (int i=0;i<a->sim.places;i++) if (!strcmp(a->sim.place[i].key,"shed")) {
            a->camera.aboard=i;a->camera.local=(V3){0,.84f,5};break;
        }
    }
    a->panel = FIELD;
    a->message[0] = 0;
    a->selected = a->listing = a->category = a->item = a->message_ticks = 0;
    a->stride_x = a->stride_y = a->stick_x = a->stick_y = 0;
    a->left_down = a->right_down = false;
    memset(a->keys, 0, sizeof a->keys);
    a->member = a->destination = 0;
    a->aiming=a->pointed=false;
    a->ground_target[0]=0;
    a->voice_at = -1000;
    a->caption_at = -1000;
    a->call_count = 0;
    a->caption[0] = a->speaker[0] = 0;
}
static void clear_request(App *a) {
    ++a->sector_request;
    a->pending_slot_load = 0;
    SDL_free(a->pending_ledger);
    a->pending_ledger=NULL;
    a->pending_mission[0]=0;
}
void app_adopt(App *a,Sim *s,Renderer *r) {
    clear_request(a);
    sim_close(&a->sim); render_close(&a->renderer);
    a->sim=*s; a->renderer=*r;
    enter_sector(a);
    if (a->desk.enabled) {
        a->renderer.quality = a->desk.current.value[PREF_QUALITY];
        a->renderer.brightness = (float)(a->desk.current.value[PREF_BRIGHT] - 1);
        lang_select(&a->renderer.lang, a->desk.current.value[PREF_LANGUAGE]);
    }
}
static bool open_stage(App *a, const char *id, const char *ledger) {
    Sim *next = calloc(1, sizeof *next);
    Renderer *view = calloc(1, sizeof *view);
    bool ok = next && view && sim_open_mission(next, a->sim.root, id) &&
              (!ledger || sim_import(next, "enter", ledger)) && render_open(view, next);
    if (ok) {
        app_adopt(a,next,view);
        session_entered(a);
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
void app_sector_ready(App *a, int request, int success) {
    if (request != a->sector_request || !*a->pending_mission)
        return;
    if (success && a->pending_slot_load) {
        int slot = a->pending_slot_load - 1;
        clear_request(a);
        if (app_read_slot(a, slot)) {
            session_menu(a, DESK_CLOSED);
            a->desk.playing = true;
        } else {
            session_menu(a, DESK_LOAD);
            SDL_strlcpy(a->desk.detail, a->message, sizeof a->desk.detail);
        }
        return;
    }
    if (success && open_stage(a, a->pending_mission, a->pending_ledger))
        return;
    const char *error = a->message;
    if (!success) {
        tell(a,
             "Could not load sector files. Check the connection and select the operation again.");
#ifdef __EMSCRIPTEN__
        error = emscripten_run_script_string("Module.lastAssetError");
#endif
    }
    if (a->desk.enabled) {
        session_menu(a, DESK_LOAD_ERROR);
        SDL_strlcpy(a->desk.detail, error, sizeof a->desk.detail);
        return;
    }
    a->pending_mission[0] = 0;
    SDL_free(a->pending_ledger);
    a->pending_ledger = NULL;
}
bool app_stage(App *a, const char *id, const char *ledger) {
    clear_request(a);
#ifdef __EMSCRIPTEN__
    if (!EM_ASM_INT(
            { return Module.requestSector(UTF8ToString($0), $1); }, id, a->sector_request)) {
        if (ledger && !(a->pending_ledger = SDL_strdup(ledger)))
            return false;
        SDL_strlcpy(a->pending_mission, id, sizeof a->pending_mission);
        tell(a, "Loading operation charts and models.");
        if (a->desk.enabled)
            session_menu(a, DESK_LOADING);
        return true;
    }
#endif
    if (open_stage(a, id, ledger))
        return true;
    if (a->desk.enabled) {
        SDL_strlcpy(a->pending_mission, id, sizeof a->pending_mission);
        if (ledger)
            a->pending_ledger = SDL_strdup(ledger);
        session_menu(a, DESK_LOAD_ERROR);
        SDL_strlcpy(a->desk.detail, a->message, sizeof a->desk.detail);
    }
    return false;
}
bool app_mission(App *a, const char *id) { return app_stage(a, id, NULL); }
bool app_retry(App *a) {
    char *entry = sim_text(&a->sim, "entry");
    if (!entry)
        return false;
    char mission[sizeof a->sim.mission];
    SDL_strlcpy(mission, a->sim.mission, sizeof mission);
    bool ok = app_stage(a, mission, entry);
    free(entry);
    return ok;
}
const Order *app_crew_order(const App *a, int choice) {
    const char *key = a->sim.place[a->sim.roster[a->member]].key;
    for (int i = 0; i < a->sim.order_count; i++)
        if (!strcmp(a->sim.orders[i].actor, key) && choice-- == 0)
            return &a->sim.orders[i];
    return NULL;
}
static int assigned_count(const App *a) {
    int n = 0;
    while (app_crew_order(a, n))
        n++;
    return n;
}
static const char *field_choices[][2] = {
    {"point", "Point at a world destination"},
    {"group", "Add or remove from group"},
    {"move", "Move to target"},
    {"scout", "Scout and report"},
    {"escort", "Escort moving crew"},
    {"engage", "Engage reported contact"},
    {"hold", "Hold position"},
    {"withdraw", "Withdraw to target"},
    {"support", "Support or recover target"},
    {"chart", "Choose target on sector chart"},
    {"bay:armor", "Produce another armored crew"},
    {"bay:section", "Muster another section"},
    {"bay:priority", "Assign this bay reserve power"},
    {"bay:power", "Switch this bay grid feed"},
    {"bay:staff", "Relieve or recall this bay watch"},
};
const char *app_field_choice(const App *a,int listing,bool label) {
    int i=listing-3-assigned_count(a);
    return i>=0 && i<(int)(sizeof field_choices/sizeof field_choices[0]) ? field_choices[i][label ? 1 : 0] : NULL;
}
int app_list_count(const App *a) {
    if (a->panel == CREW_JOB) {
        const Place *p=&a->sim.place[a->sim.roster[a->member]];
        bool bay=!strcmp(a->sim.mission,"gate-six") && (!strcmp(p->type,"watch") || !strcmp(p->type,"motor"));
        return assigned_count(a) + 3 + (bay ? 15 : 10);
    }
    return a->panel == OPERATIONS ? a->sim.operation_count + (a->desk.enabled ? 6 : 4)
           : a->panel == CREWS    ? a->sim.crew_count
                                  : a->sim.order_count;
}
void app_target(App *a, int direction) {
    a->ground_target[0]=0;a->pointed=false;
    for (int i = 0; i < a->sim.places; i++) {
        a->destination = (a->destination + direction + a->sim.places) % a->sim.places;
        const Place *p = &a->sim.place[a->destination];
        V3 last;
        int age;
        if (!strcmp(p->owner, "rival") && !sim_contact(&a->sim, p->key, &last, &age))
            continue;
        if (p->key[0] && (!strcmp(p->family, "marker") || p->role[0]))
            return;
    }
}
static void crew_card(App *a, int member) {
    a->pointed=false;a->ground_target[0]=0;
    a->member = member;
    a->listing = 0;
    a->destination = a->sim.roster[member];
    a->panel = CREW_JOB;
}
void app_choose(App *a) {
    if (a->panel == OPERATIONS && a->listing < a->sim.operation_count) {
        const char *id = a->sim.operations[a->listing].id;
        if (a->desk.enabled) {
            if (a->desk.practice) {
                tell(a, "End practice before changing operation.");
                return;
            }
            session_request(a, DO_MISSION, id, 0);
        } else
            app_mission(a, id);
    } else if (a->panel == OPERATIONS) {
        int action = a->listing - a->sim.operation_count;
        if (a->desk.enabled && action >= 4) {
            session_menu(a, action == 4 ? DESK_PAUSE : DESK_HELP);
            return;
        }
        if (a->desk.enabled && a->desk.practice && action == 3) {
            tell(a, "End practice before continuing the campaign.");
            return;
        }
        if (a->desk.enabled && action < 3) {
            if (a->desk.practice) {
                tell(a, "End practice to use campaign controls.");
                return;
            }
            if (action == 2)
                session_request(a, DO_RESTART, NULL, 0);
            else
                session_menu(a, action == 0 ? DESK_SAVES : DESK_LOAD);
            return;
        }
        switch (action) {
        case 0:
            app_save(a);
            break;
        case 1:
            app_resume(a);
            break;
        case 2:
            app_retry(a);
            break;
        case 3:
            app_continue(a);
            break;
        }
    } else if (a->panel == CREWS && a->listing < a->sim.crew_count)
        crew_card(a, a->listing);
    else if (a->panel == CREW_JOB) {
        const char *key = a->sim.place[a->sim.roster[a->member]].key;
        const Order *job = app_crew_order(a, a->listing - 3);
        char order[160];
        const char *field=app_field_choice(a,a->listing,false);
        if (a->destination<0 || a->destination>=a->sim.places) {
            a->destination=a->sim.roster[a->member];
            a->pointed=false;
            tell(a,"Target has left the sector. Mark a new destination.");
            return;
        }
        const char *target=*a->ground_target ? a->ground_target : a->sim.place[a->destination].key;
        if (field && !strcmp(field, "chart")) {
            a->aiming = true;
            a->pointed = false;
            a->listing = assigned_count(a) + 5;
            a->panel = CHART;
            return;
        }
        if (field && !strcmp(field,"point")) {
            a->aiming=true; a->pointed=false; a->listing+=2; a->panel=FIELD;
            tell(a,"Point at a crew, work site or ground. Click or tap to mark.");
            return;
        }
        V3 location;
        if (a->pointed && !*a->ground_target && !world_object(&a->sim,a->destination,&location)) {
            tell(a,"Target has left the sector. Mark a new destination.");return;
        }
        if (field && !strcmp(field,"group"))
            snprintf(order,sizeof order,"group:%s",key);
        else if (field && !strncmp(field,"bay:",4))
            snprintf(order,sizeof order,"%s@%s",field,key);
        else if (field)
            snprintf(order,sizeof order,"%s:%s@%s",field,key,target);
        else if (job)
            snprintf(order, sizeof order, "%s", job->id);
        else if (a->listing == 1)
            snprintf(order, sizeof order, "release:%s", key);
        else
            snprintf(order, sizeof order, a->listing == 2 ? "work:%s@%s" : "crew:%s@%s", key,
                     a->sim.place[a->destination].key);
        if (!sim_order(&a->sim, order))
            tell(a, a->sim.report[0] ? a->sim.report : "Crew unavailable for that assignment.");
        else
            a->message_ticks = 0;
    } else if (a->panel == SLATE)
        app_action(a, a->selected);
}
bool app_open(App *a, const char *root, bool hidden) {
    memset(a, 0, sizeof *a);
    session_defaults(&a->desk.current);
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
    SDL_SetWindowTitle(a->window, "Orra Basin / Loading mission 1 of 2");
    if (!sim_open(&a->sim, root))
        return false;
    SDL_SetWindowTitle(a->window, "Orra Basin / Preparing world 2 of 2");
    if (!render_open(&a->renderer, &a->sim))
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
            SDL_AudioStream **stream = i < 2 ? &a->audio : &a->voice;
            if (SDL_LoadWAV(path, &spec, &a->tone[i], &a->tone_size[i]) && !*stream)
                *stream =
                    SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
        }
        if (a->voice)
            SDL_ResumeAudioStreamDevice(a->voice);
        if (a->audio) {
            SDL_ResumeAudioStreamDevice(a->audio);
            sound_open(a);
        }
    }
    enter_sector(a);
    SDL_SetWindowTitle(a->window, "Orra Basin");
    a->running = true;
    return true;
}
void app_close(App *a) {
    session_end(a);
    SDL_free(a->pending_ledger);
    a->pending_ledger = NULL;
    sound_close(a);
    if (a->audio)
        SDL_DestroyAudioStream(a->audio);
    if (a->voice)
        SDL_DestroyAudioStream(a->voice);
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
    SDL_AudioStream *stream = tone < 2 ? a->audio : a->voice;
    if (stream)
        SDL_SetAudioStreamGain(stream, session_gain(a, tone < 2 ? PREF_FX : PREF_RADIO));
    if (stream && a->tone[tone] && session_gain(a, tone < 2 ? PREF_FX : PREF_RADIO) > 0 &&
        SDL_PutAudioStreamData(stream, a->tone[tone], (int)a->tone_size[tone])) {
        a->audio_cues++;
        if (tone >= 2) sound_tick(a);
    }
}
void app_action(App *a, int index) {
    bool accepted = sim_command(&a->sim, index);
    play_cue(a, accepted ? 0 : 1);
    if (accepted && a->sim.tick - a->voice_at >= 180) {
        const char *calls[] = {"WEIR", "SOUNDING", "TERRACE", "SALT", "LANTERN", "MERIDIAN"};
        for (int i = 0; i < 6; i++)
            if (!strncmp(a->sim.faction, calls[i], strlen(calls[i]))) {
                const char *lines[] = {"audio.weir", "audio.sounding", "audio.terrace",
                                       "audio.salt", "audio.lantern",  "audio.meridian"};
                sound_caption(a, calls[i], lines[i]);
                play_cue(a, i + 2);
            }
        a->voice_at = a->sim.tick;
    }
    if (!accepted)
        sound_caption(a, "Dispatch", "audio.blocked");
    if (accepted)
        a->desk.dirty = true;
    session_speak(a, a->sim.report);
    if (a->observe_order)
        a->observe_order(a->sim.tick, index, accepted);
    a->message_ticks = 0;
}
bool app_lift(App *a,bool activate) {
    Camera *c=&a->camera;
    if (c->aboard<0 || strcmp(a->sim.place[c->aboard].type,"station") ||
        fabsf(c->local.x+4)>2.1f || fabsf(c->local.z-326)>3.1f) return false;
    for (int i=0;i<a->sim.places;i++) {
        const Place *car=&a->sim.place[i];
        if (strcmp(car->type,"lift-car")) continue;
        if (!activate) return true;
        bool inside=fabsf(c->local.x+4)<1.6f && fabsf(c->local.z-326)<1.8f;
        int level=inside && fabsf(c->local.y-car->current.y)<.2f ? car->current.y<3 : c->local.y>=3;
        sim_order(&a->sim,level ? "lift-call-1" : "lift-call-0");
        return true;
    }
    return false;
}
void app_key(App *a, SDL_Scancode k) {
    if (session_input(a, k))
        return;
    if (a->desk.enabled && (k == SDL_SCANCODE_F5 || k == SDL_SCANCODE_F9)) {
        if (a->desk.practice) {
            tell(a, "End practice to use campaign saves.");
            return;
        }
        session_check_slots(a);
        session_request(a, k == SDL_SCANCODE_F5 ? DO_SAVE : DO_LOAD, NULL, a->desk.slot);
        return;
    }
    if (k == SDL_SCANCODE_B) {
        a->panel = a->panel == CHART ? FIELD : CHART;
        capture_mouse(a, false);
        return;
    }
    if (k==SDL_SCANCODE_F5) {app_save(a);return;}
    if (k==SDL_SCANCODE_F9) {app_resume(a);return;}
    if (a->aiming && k==SDL_SCANCODE_ESCAPE) {a->aiming=false;a->panel=CREW_JOB;return;}
    if (k == SDL_SCANCODE_L) {
        a->panel = a->panel == RADIO ? FIELD : RADIO;
        a->listing = 0;
        capture_mouse(a, false);
        return;
    }
    if (a->panel == RADIO &&
        (k == SDL_SCANCODE_DOWN || k == SDL_SCANCODE_UP || k == SDL_SCANCODE_PAGEDOWN)) {
        int count = 7 + SDL_min(a->call_count, 12);
        a->listing = (a->listing + count +
                      (k == SDL_SCANCODE_UP         ? -1
                       : k == SDL_SCANCODE_PAGEDOWN ? 7
                                                    : 1)) %
                     count;
        return;
    }
    if (k == SDL_SCANCODE_V && a->sim.crew_count) {
        V3 eye = camera_eye(&a->camera, &a->sim);
        int best = -1;
        float distance = 450;
        for (int j = 0; j < a->sim.crew_count; j++) {
            V3 p;
            int i = a->sim.roster[j];
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
        if(a->desk.enabled)session_request(a,DO_RESTART,NULL,0);
        else app_retry(a);
        return;
    }
    if (k == SDL_SCANCODE_E) {
        if (app_lift(a,true)) return;
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
        if (a->desk.enabled && a->desk.playing)
            session_request(a, DO_EXIT, NULL, 0);
        else
            a->running = false;
        break;
    case SDL_EVENT_WILL_ENTER_BACKGROUND:
    case SDL_EVENT_WINDOW_MINIMIZED:
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        session_pause(a);
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
            if (e->type == SDL_EVENT_KEY_DOWN && a->desk.rebinding) {
                session_rebind(a, sc);
                break;
            }
            sc = session_key(a, sc);
            if (e->type == SDL_EVENT_KEY_DOWN && !e->key.repeat)
                a->touch = false;
            bool movement = sc == SDL_SCANCODE_W || sc == SDL_SCANCODE_A || sc == SDL_SCANCODE_S ||
                            sc == SDL_SCANCODE_D || sc == SDL_SCANCODE_LSHIFT;
            if (movement && a->desk.current.value[PREF_TOGGLE]) {
                if (e->type == SDL_EVENT_KEY_DOWN && !e->key.repeat)
                    a->keys[sc] = !a->keys[sc];
                break;
            }
            a->keys[sc] = e->type == SDL_EVENT_KEY_DOWN;
            if (a->keys[sc] && !e->key.repeat) {
                app_key(a, sc);
                if (a->desk.page == DESK_CLOSED && a->desk.current.value[PREF_NARRATE] &&
                    orra_menu_count() > 0) {
                    int row = a->panel == SLATE ? a->selected : a->listing;
                    session_speak(a, orra_menu_row(row));
                }
            }
        }
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (e->button.which == SDL_TOUCH_MOUSEID)
            break;
        a->touch = false;
        if (a->desk.page != DESK_CLOSED || a->panel != FIELD) {
            int w, h;
            SDL_GetWindowSize(a->window, &w, &h);
            ui_tap(a, e->button.x / w, e->button.y / h);
        } else if (a->captured && e->button.button == SDL_BUTTON_LEFT)
            app_screen_point(a,.5f,.5f);
        else capture_mouse(a,true);
        break;
    case SDL_EVENT_MOUSE_MOTION:
        if (a->captured && a->panel == FIELD && a->desk.page == DESK_CLOSED) {
            float gain = .0011f * (1 + a->desk.current.value[PREF_LOOK]);
            camera_look(&a->camera, e->motion.xrel * gain,
                        e->motion.yrel * gain * (a->desk.current.value[PREF_INVERT] ? 1 : -1));
        }
        break;
    case SDL_EVENT_FINGER_DOWN: {
        a->touch = true;
        float x = e->tfinger.x, y = e->tfinger.y;
        if (a->desk.page != DESK_CLOSED || a->panel != FIELD || y < .115f ||
            (x > .58f && y > .84f) || strcmp(a->sim.result, "active")) {
            app_touch(a, x, y);
            break;
        }
        if (x < .45f) {
            a->left_down = true;
            a->left_id = e->tfinger.fingerID;
            a->left_x = x;
            a->left_y = y;
        } else {
            a->touch_motion=0;
            a->right_down = true;
            a->right_id = e->tfinger.fingerID;
        }
        break;
    }
    case SDL_EVENT_FINGER_MOTION:
        if (a->left_down && a->left_id == e->tfinger.fingerID) {
            a->stick_x = (e->tfinger.x - a->left_x) * 9;
            a->stick_y = (a->left_y - e->tfinger.y) * 9;
        } else if (a->right_down && a->right_id == e->tfinger.fingerID) {
            a->touch_motion+=fabsf(e->tfinger.dx)+fabsf(e->tfinger.dy);
            float gain = .5f * (1 + a->desk.current.value[PREF_LOOK]);
            camera_look(&a->camera, e->tfinger.dx * 3.2f * gain,
                        e->tfinger.dy * 2.6f * gain *
                            (a->desk.current.value[PREF_INVERT] ? 1 : -1));
        }
        break;
    case SDL_EVENT_FINGER_UP:
    case SDL_EVENT_FINGER_CANCELED:
        if (a->left_down && a->left_id == e->tfinger.fingerID) {
            a->left_down = false;
            a->stick_x = a->stick_y = 0;
        }
        if (a->right_down && a->right_id == e->tfinger.fingerID) {
            if (e->type==SDL_EVENT_FINGER_UP && a->touch_motion<.015f) app_screen_point(a,e->tfinger.x,e->tfinger.y);
            a->right_down = false;
        }
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
    if (a->desk.page != DESK_CLOSED)
        return;
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
    int riding=-1;
    if (a->camera.aboard>=0 && !strcmp(a->sim.place[a->camera.aboard].type,"station") &&
        fabsf(a->camera.local.x+4)<1.6f && fabsf(a->camera.local.z-326)<1.8f)
        for (int i=0;i<a->sim.places;i++)
            if (!strcmp(a->sim.place[i].type,"lift-car") && fabsf(a->sim.place[i].current.y-a->camera.local.y)<.08f) riding=i;
    float old_yaw = world_angle(&a->sim, a->camera.aboard);
    if (!sim_step(&a->sim, 1)) {
        if (a->desk.enabled)
            session_report(a, a->sim.error);
        SDL_Log("Mission error: %s", a->sim.error);
        a->running = false;
    } else if (active && strcmp(a->sim.result, "active"))
        play_cue(a, !strcmp(a->sim.result, "defeat"));
    else if (!warning && a->sim.relief_at) {
        sound_caption(a, "Gate watch", "audio.muster");
        play_cue(a, 1);
    } else if (previous_jobs < finished(&a->sim.blue)) {
        sound_caption(a, "Fitters", "audio.complete");
        play_cue(a, 0);
    }
    if (riding>=0) a->camera.local.y=a->sim.place[riding].current.y;
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
    if (a->desk.enabled && a->renderer.error[0]) {
        session_report(a, a->renderer.error);
        session_pause(a);
        SDL_strlcpy(a->desk.detail, a->renderer.error, sizeof a->desk.detail);
        a->renderer.error[0] = 0;
    }
}
void app_advance(App *a, double dt) {
    if (a->desk.page != DESK_CLOSED) {
        a->accumulator = 0;
        return;
    }
    a->accumulator += fmin(fmax(dt, 0), .25) * (a->desk.current.value[PREF_SPEED] ? .5 : 1.);
    while (a->accumulator + 1.e-12 >= 1. / 30.) {
        app_tick(a);
        a->accumulator = fmax(0, a->accumulator - 1. / 30.);
    }
}
void app_frame(App *a, double dt) {
    Uint64 now=SDL_GetTicks();
    if (now>=a->next_audio_update) {
        sound_tick(a);
        a->next_audio_update=now+200;
    }
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
