#include "app.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#define INK 0x17282bee
#define LINE 0x718981c8
#define PAPER 0xebdfbfff
#define MUTE 0xb2c1aeff
#define AMBER 0xf6ad65ff
#define BLUE 0x7cbabdff
#define DARK 0x1a292bff
#define R(x, y, w, h, c) render_rect(r, (x), (y), (w), (h), (c))
#define T(x, y, s, text, c) render_text(r, (x), (y), (s), (text), (c))
static const char *categories[] = {"UNITS", "FACILITIES", "FACTIONS", "OPERATIONS", "UPGRADES"};
void ui_size(const App *a, int *w, int *h) {
    float scale = fminf(a->height / 540.f, a->width / 478.f);
    if (scale < .1f)
        scale = .1f;
    *w = (int)lroundf(a->width / scale);
    *h = (int)lroundf(a->height / scale);
}
static void radio_face(Renderer *r, float x, float y, float s) {
    for (int j = 0; j < 18; j++)
        for (int i = 0; i < 18; i++) {
            char c = r->portrait[j][i];
            uint32_t col;
            switch (c) {
            case 'b':
                col = 0x223b3dff;
                break;
            case 'c':
                col = 0x836b50ff;
                break;
            case 'd':
                col = 0x486665ff;
                break;
            case 'e':
                col = 0xb49972ff;
                break;
            case 'f':
                col = 0xdec79bff;
                break;
            case 'g':
                col = 0x79918aff;
                break;
            case 'a':
                col = AMBER;
                break;
            case 'l':
                col = 0x344949ff;
                break;
            default:
                continue;
            }
            R(x + i * s, y + j * s, s, s, col);
        }
}
static void wrapped(Renderer *r, int x, int y, int max, float scale, const char *text, uint32_t col) {
    char line[128];
    int len = 0;
    int chars = (int)(max / (6 * scale));
    while (*text) {
        while (*text == ' ')
            text++;
        const char *end = strchr(text, ' ');
        int n = end ? (int)(end - text) : (int)strlen(text);
        if (len && len + n + 1 > chars) {
            line[len] = 0;
            T(x, y, scale, line, col);
            y += (int)ceilf(10 * scale);
            len = 0;
        }
        if (len)
            line[len++] = ' ';
        if (n > 120 - len)
            n = 120 - len;
        memcpy(line + len, text, (size_t)n);
        len += n;
        text += n;
    }
    line[len] = 0;
    T(x, y, scale, line, col);
}
static void compass(Renderer *r, int w, float yaw) {
    int center = w / 2;
    int degree = (int)lroundf(yaw * 180 / ORRA_PI);
    for (int angle = -180; angle <= 180; angle += 5) {
        int d = (angle - degree + 540) % 360 - 180;
        if (d < -44 || d > 44 || angle == 180)
            continue;
        int x = center + d * 2;
        int size = (angle % 15 == 0) ? 7 : 3;
        R(x, 24 - size, 1, size, 0xdde1c6cd);
        const char *dir = angle == 0      ? "N"
                          : angle == 90   ? "E"
                          : angle == -90  ? "W"
                          : angle == -180 ? "S"
                                          : "";
        if (*dir)
            T(x - 3, 7, 1.2f, dir, PAPER);
    }
    R(center - 2, 26, 5, 2, AMBER);
    R(center, 24, 1, 4, AMBER);
}
static void order_panel(App *a, int w, int h) {
    Renderer *r = &a->renderer;
    int panel = w > 720 ? 344 : 300, x = w - panel - 16, y = 66;
    int row = 24;
    R(x, y, panel, 326, 0x101f21f5);
    R(x, y, panel, 3, AMBER);
    if (a->panel == RADIO) {
        T(x + 14, y + 14, 1.8f, "RADIO LOG", PAPER);
        for (int i = 0; i < 7; i++) {
            char line[180];
            sim_log(&a->sim, i, line, sizeof line);
            wrapped(r, x + 15, y + 48 + 38 * i, panel - 30, 1.12f, line, i ? MUTE : PAPER);
        }
        return;
    }
    const char *title = a->panel == CREW_JOB     ? a->sim.place[a->sim.roster[a->member]].name
                        : a->panel == OPERATIONS ? "OPERATIONS"
                        : a->panel == CREWS      ? "CREW DISPATCH"
                                                 : "COMMAND SLATE";
    T(x + 14, y + 15, 1.95f, title, PAPER);
    int count = app_list_count(a), selected = a->panel == SLATE ? a->selected : a->listing,
        first = selected / 10 * 10;
    T(x + 14, y + 38, 1, count > 10 ? "PAGE DOWN / TAP FOR NEXT" : "FIELD REGISTER", BLUE);
    T(x + panel - 62, y + 38, 1, "TAB CLOSE", MUTE);
    for (int i = first; i < count && i < first + 10; i++) {
        int yy = y + 57 + (i - first) * row;
        bool sel = i == selected;
        R(x + 8, yy, panel - 16, row - 1, sel ? 0x344f50ff : 0x223437ee);
        if (sel)
            R(x + 8, yy, 3, row - 1, AMBER);
        char idx[3];
        snprintf(idx, sizeof idx, "%d", (i + 1) % 10);
        T(x + 17, yy + 8, 1.2f, idx, AMBER);
        static const char *options[]={"Save operation","Resume saved operation","Retry this operation","Continue campaign"};
        const char *label = a->panel == OPERATIONS ? (i<a->sim.operation_count ? a->sim.operations[i].name : options[i-a->sim.operation_count])
                            : a->panel == CREWS    ? a->sim.place[a->sim.roster[i]].name
                                                   : a->panel == SLATE ? a->sim.orders[i].label : "";
        if (a->panel == CREW_JOB) {
            const Order *order = app_crew_order(a, i - 3);
            const char *field = app_field_choice(a,i,true);
            label = field ? field : order    ? order->label
                    : i == 0 ? "Report on selected target"
                    : i == 1 ? "Release field assignment"
                             : a->sim.place[a->sim.roster[a->member]].role;
        }
        if (a->panel == SLATE && !strcmp(a->sim.orders[i].id, a->sim.advice))
            T(x + panel - 25, yy + 7, 1.35f, ">", AMBER);
        T(x + 39, yy + 7, 1.26f, label, PAPER);
    }
    char detail[100] = "ARROWS SELECT   ENTER DISPATCH";
    if (a->panel == CREWS && count) {
        const Place *crew = &a->sim.place[a->sim.roster[selected]];
        int age, state = sim_crew_status(&a->sim, crew->key, &age);
        snprintf(detail, sizeof detail, "%s / %s / %dS", crew->role,
                 state == 1   ? "ON SHIFT"
                 : state == 2 ? "REPORTED"
                              : "READY",
                 age / 30);
    } else if (a->panel == CREW_JOB) {
        snprintf(detail, sizeof detail, "< TARGET  %s  >", *a->ground_target ? a->ground_target : a->sim.place[a->destination].name);
        char report[110];
        sim_crew_report(&a->sim, a->sim.place[a->sim.roster[a->member]].key, report, sizeof report);
        if (!*report) sim_inspect(&a->sim,a->sim.place[a->sim.roster[a->member]].key,report,sizeof report);
        T(x + 15, y + 282, 1.1f, report, BLUE);
    } else if (a->panel == SLATE && !strcmp(a->sim.mission, "tide-window")) {
        snprintf(detail, sizeof detail, "PILOT / RIVAL QUAY 05:00 - 08:00");
    } else if (a->panel == SLATE && a->sim.reserved)
        snprintf(detail, sizeof detail, "STORES / %d ALLOY RESERVED", a->sim.reserved);
    else if (a->panel == SLATE && !strcmp(a->sim.mission, "gate-six"))
        snprintf(detail, sizeof detail, "HOLD %dS / PAGE %d OF %d", a->sim.hold_ticks / 30,
                 first / 10 + 1, (count + 9) / 10);
    T(x + 15, y + 306, 1, detail, MUTE);
    (void)h;
}
static void guide_panel(App *a, int w, int h) {
    Renderer *r = &a->renderer;
    int wid = w > 700 ? 415 : 340, x = w - wid - 16, y = 65;
    Catalog *c = &a->sim.cats[a->category];
    Entry *e = &c->entry[a->item];
    char counter[32];
    snprintf(counter, sizeof counter, "%02d / %02d", a->item + 1, c->count);
    R(x, y, wid, 228, 0x17282bf6);
    R(x, y, wid, 3, BLUE);
    T(x + 16, y + 15, 1.9f, "BASIN FIELD GUIDE", PAPER);
    T(x + 16, y + 45, 1.25f, categories[a->category], BLUE);
    T(x + wid - 82, y + 45, 1.2f, counter, MUTE);
    R(x + 15, y + 64, wid - 30, 1, LINE);
    T(x + 16, y + 80, 1.85f, e->name, PAPER);
    T(x + 16, y + 103, 1.0f, e->owner, AMBER);
    wrapped(r, x + 16, y + 125, wid - 32, 1.35f, e->role, PAPER);
    T(x + 16, y + 199, 1.15f, "< PREV     NEXT >      CATEGORY     CLOSE", MUTE);
    (void)h;
}
void ui_relief_notice(Renderer *r, const Sim *g, bool compact) {
    if (!g->relief_at || strcmp(g->result, "active"))
        return;
    char text[56];
    if (g->red.hold > 0)
        snprintf(text, sizeof text, "CONTROL LOST IN %dS", (g->hold_ticks - g->red.hold + 29) / 30);
    else if (g->tick < g->relief_at)
        snprintf(text, sizeof text, "COMPACT RELIEF IN %dS", (g->relief_at - g->tick + 29) / 30);
    else if (g->red.walk >= 300)
        snprintf(text, sizeof text, "RIVAL IN SWITCH ROOM");
    else if (g->red.walk > 0 || g->red.drive > 0)
        snprintf(text, sizeof text, "COMPACT ON APPROACH");
    else
        return;
    float scale = compact ? .94f : 1.14f;
    int width = (int)ceilf(strlen(text) * 6 * scale) + 16;
    R(15, 89, width, 22, 0x142326ff);
    R(15, 89, 3, 22, AMBER);
    T(23, 96, scale, text, AMBER);
}
static const char *field_objective(const Sim *g) {
    const Side *crew = &g->blue;
    if (!crew->mill) return "Commission Plate Mill";
    if (!crew->harvesting) return "Dispatch the hauler";
    if (!crew->watch) return "Build the Watch House";
    if (!crew->section) return "Muster a Gate Section";
    if (g->relief_at) {
        if (!crew->motor) return "Build Motor Hall for support";
        if (!crew->armor) return "Produce a Lintel for support";
        if (!crew->guard) return "Send armor to the gate yard";
    }
    if (!crew->advance) return "Send infantry to Gate Six";
    return crew->walk < 300 ? "Infantry moving to Gate Six" : "Hold the switch room";
}
void ui_draw(App *a, int w, int h) {
    Renderer *r = &a->renderer;
    Sim *g = &a->sim;
    char line[128];
    R(15, 14, 158, 43, INK);
    R(15, 14, 3, 43, AMBER);
    R(27, 23, 12, 3, PAPER);
    R(27, 29, 8, 2, PAPER);
    R(27, 35, 12, 3, PAPER);
    T(47, 21, .94f, a->sim.faction, BLUE);
    T(47, 35, strlen(g->title) > 13 ? 1.13f : 1.52f, g->title, PAPER);
    T(183, 21, 1.05f, "[M] OPS", PAPER);
    T(183, 38, 1.05f, "[C] CREWS", BLUE);
    if (w > 700)
        T(258, 38, 1.05f, "[L] LOG", MUTE);
    if (w > 600)
        T(315, 38, 1.05f, "[B] CHART", MUTE);
    if (a->camera.aboard>=0 && a->panel==FIELD) {
        const Place *p=&g->place[a->camera.aboard];
        snprintf(line,sizeof line,"ABOARD %s",p->name[0]?p->name:p->key);
        T(23,118,1.12f,line,PAPER);
    }
    if (a->panel == FIELD && !strcmp(g->result, "active")) {
        const char *objective = g->objective[0] ? g->objective : field_objective(g);
        int ow = (int)strlen(objective) * 7 + 19;
        R(15, 64, ow, 20, 0x1b2c2bc9);
        R(15, 64, 2, 20, BLUE);
        T(23, 70, 1.14f, objective, PAPER);
    }
    ui_relief_notice(r, g, w < 700 && a->panel != FIELD);
    if (w > 675)
        compass(r, w, a->camera.yaw);
    R(w - 183, 14, 168, 43, INK);
    T(w - 172, 22, 1.0f, "ALLOY", MUTE);
    snprintf(line, sizeof line, "%02d", g->blue.alloy);
    T(w - 172, 35, 1.85f, line, PAPER);
    R(w - 115, 21, 1, 28, LINE);
    T(w - 104, 22, 1, "GRID", MUTE);
    snprintf(line, sizeof line, "%d/6", g->blue.draw);
    T(w - 104, 36, 1.35f, line, g->blue.power ? BLUE : AMBER);
    T(w - 57, 22, 1, g->clock ? "TIME" : "HOLD", MUTE);
    snprintf(line, sizeof line, "%02dS", g->blue.hold / 30);
    if (g->clock)
        snprintf(line, sizeof line, "%d:%02d", g->clock / 1800, (g->clock / 30) % 60);
    T(w - 57, 36, 1.35f, line, AMBER);
    int bottom = h - 16, radio_w = w < 625 ? 253 : 355;
    R(15, bottom - 48, radio_w, 48, INK);
    R(15, bottom - 48, radio_w, 1, LINE);
    R(21, bottom - 42, 40, 37, 0x4f6260ff);
    radio_face(r, 22, bottom - 43, 2.05f);
    snprintf(line, sizeof line, "DISPATCH / RADIO   %dS AGO", (g->tick - g->report_at) / 30);
    T(70, bottom - 39, 1.f, line, BLUE);
    const char *msg = a->message_ticks ? a->message : g->report;
    wrapped(r, 70, bottom - 24, radio_w - 65, w < 625 ? 1.0f : 1.14f, msg, PAPER);
    if (a->panel == FIELD) {
        float cx = w * .5f, cy = h * .5f;
        R(cx - 6, cy, 3, 1, 0xf0e6bfd9);
        R(cx + 4, cy, 3, 1, 0xf0e6bfd9);
        R(cx, cy - 6, 1, 3, 0xf0e6bfd9);
        R(cx, cy + 4, 1, 3, 0xf0e6bfd9);
        int near = world_nearest(g, &a->camera);
        if (app_lift(a,false)) T(cx-62,cy+25,1.1f,"[E] CALL / RIDE LIFT",PAPER);
        else if (near >= 0) {
            snprintf(line, sizeof line, "[E] %s %s", near != a->camera.aboard ? "BOARD" : "LEAVE",
                     g->place[near].model);
            int ww = (int)strlen(line) * 7 + 18;
            R(cx - ww / 2, cy + 19, ww, 20, INK);
            T(cx - ww / 2 + 8, cy + 25, 1.1f, line, PAPER);
        } else if (world_terminal(g, &a->camera)) {
            R(cx - 63, cy + 19, 126, 20, INK);
            T(cx - 55, cy + 25, 1.1f, "[E] DISPATCH DESK", PAPER);
        }
    } else if (a->panel == CHART)
        chart_draw(a, w);
    else if (a->panel != GUIDE)
        order_panel(a, w, h);
    else
        guide_panel(a, w, h);
    int by = bottom - 39;
    R(w - 180, by, 165, 39, INK);
    R(w - 180, by, 165, 1, LINE);
    T(w - 171, by + 7, 1.25f, a->touch ? "ORDERS    INTERACT" : "[TAB] ORDERS", PAPER);
    T(w - 171, by + 24, 1.0f, a->touch ? "GUIDE      LOOK / MOVE" : "[G] GUIDE    [E] BOARD", MUTE);
    if (a->touch) {
        R(23, h - 144, 62, 63, 0x2336399c);
        R(38, h - 113, 32, 1, LINE);
        R(54, h - 129, 1, 32, LINE);
    }
    if (strcmp(g->result, "active")) {
        int x = (w - 350) / 2, y = h / 2 - 31;
        R(x, y, 350, 62, 0x162829ef);
        R(x, y, 350, 3, AMBER);
        T(x + 16, y + 14, 2,
          !strcmp(g->result, "victory")
              ? (!strcmp(g->mission, "gate-six") ? "GATE SIX SECURED" : "OPERATION COMPLETE")
              : "CONTROL LOST",
          PAPER);
        T(x + 16, y + 42, 1.12f, "[R] RESTART OPERATION", BLUE);
    }
}
void ui_wrist(App *a) {
    if (a->panel == CHART)
        chart_draw(a, 350);
    else if (a->panel == GUIDE)
        guide_panel(a, 350, 400);
    else if (a->panel != FIELD)
        order_panel(a, 350, 400);
    else
        ui_relief_notice(&a->renderer, &a->sim, true);
}
void ui_tap(App *a, float fx, float fy) {
    int w, h;
    ui_size(a, &w, &h);
    float x = fx * w, y = fy * h;
    if (strcmp(a->sim.result, "active") && fabsf(y - h / 2.f) < 35 && fabsf(x - w / 2.f) < 180) {
        app_key(a, SDL_SCANCODE_R);
        return;
    }
    if (y < 57 && x >= 310 && x < 392) {
        app_key(a, SDL_SCANCODE_B);
        return;
    }
    if (a->panel == CHART && y >= 67) {
        chart_pick(a, w, x, y);
        return;
    }
    if (y < 57 && x >= 250 && x < 310) {
        app_key(a, SDL_SCANCODE_L);
        return;
    }
    if (a->panel == CREW_JOB && x > w - (w > 720 ? 344 : 300) - 16 && y >= 362 && y < 392) {
        app_target(a, x > w - 155 ? 1 : -1);
        return;
    }
    if (y < 57 && x >= 15 && x < 250) {
        app_key(a, (x >= 175 && y > 30) ? SDL_SCANCODE_C : SDL_SCANCODE_M);
        return;
    }
    if (x > w - 183 && y > h - 59) {
        if (y > h - 37)
            app_key(a, SDL_SCANCODE_F1);
        else if (a->touch && x > w - 96)
            app_key(a, SDL_SCANCODE_E);
        else
            app_key(a, SDL_SCANCODE_TAB);
        return;
    }
    if ((a->panel == SLATE || a->panel == OPERATIONS || a->panel == CREWS || a->panel == CREW_JOB) &&
        x > w - (w > 720 ? 344 : 300) - 16) {
        int row = (int)floorf((y - 123) / 24.f);
        if (row >= 0 && row < ORDER_COUNT) {
            int *sel = a->panel == SLATE ? &a->selected : &a->listing;
            row += *sel / 10 * 10;
            if (row < app_list_count(a)) {
                *sel = row;
                app_choose(a);
            }
        } else if (y >= 96 && y < 123 && app_list_count(a) > 10)
            app_key(a, SDL_SCANCODE_PAGEDOWN);
        else if (y >= 66 && y < 123)
            app_key(a, SDL_SCANCODE_ESCAPE);
    }
    if (a->panel == GUIDE && x > w - (w > 700 ? 415 : 340) - 16 && y >= 245 && y < 291) {
        float pos = (x - (w - (w > 700 ? 415 : 340) - 16)) / (w > 700 ? 415.f : 340.f);
        if (pos < .24f)
            app_key(a, SDL_SCANCODE_UP);
        else if (pos < .48f)
            app_key(a, SDL_SCANCODE_DOWN);
        else if (pos < .80f)
            app_key(a, SDL_SCANCODE_RIGHT);
        else
            app_key(a, SDL_SCANCODE_F1);
    }
}
