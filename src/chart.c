#include "app.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static bool mark(const App *a, int id, V3 *p, int *age) {
    const Place *row = &a->sim.place[id];
    if (!(row->role[0] || !strcmp(row->family, "marker")))
        return false;
    if (!strcmp(row->owner, "rival"))
        return sim_contact(&a->sim, row->key, p, age);
    *age = -1;
    return world_object(&a->sim, id, p);
}
static int chart(App *a, int width, float pick_x, float pick_y, bool draw) {
    float x = width - 315, y = 67, w = 298, h = 300;
    float range = !strcmp(a->sim.terrain, "orbit")     ? 840
                  : !strcmp(a->sim.terrain, "estuary") ? 420
                                                       : 300;
    float scale = (h - 60) / range;
    const float ox = x + w * .5f, oy = y + h - 25 - 90 * scale;
    Renderer *r = &a->renderer;
    if (draw) {
        render_rect(r, x, y, w, h, 0x132729f5);
        render_text(r, x + 11, y + 12, 1.65f, "SECTOR CHART", 0xe9e2c6ff);
        render_text(r, x + 11, y + 34, 1, a->aiming ? "MARK A DESTINATION" : "SELECT A CREW",
                    0x8dc4beff);
        render_text(r, x + w - 30, y + 12, 1, "N ^", 0xc8be98ff);
        for (int i = 1; i < 6; i++) {
            render_rect(r, x + i * w / 6, y + 48, 1, h - 65, 0x30494aff);
            render_rect(r, x + 8, y + 48 + i * (h - 65) / 6, w - 16, 1, 0x30494aff);
        }
    }
    int best = -1;
    float near = 12;
    for (int i = 0; i < a->sim.places; i++) {
        int age;
        V3 p;
        if (!mark(a, i, &p, &age))
            continue;
        float px = ox + p.x * scale, py = oy + p.z * scale;
        if (px < x + 4 || px > x + w - 4 || py < y + 47 || py > y + h - 18)
            continue;
        const Place *row = &a->sim.place[i];
        if (draw) {
            unsigned ink = age >= 0                         ? 0xe29d62ff
                           : !strcmp(row->family, "marker") ? 0xc8be98ff
                                                            : 0x83d0cdff;
            render_rect(r, px - 3, py - 3, age >= 0 ? 7 : 5, age >= 0 ? 2 : 5, ink);
            if (i == a->destination) {
                render_rect(r, px - 5, py + 5, 10, 1, 0xf2d183ff);
                char caption[100];
                const char *label = row->name[0] ? row->name : row->key;
                if (age >= 0)
                    snprintf(caption, sizeof caption, "%s / LAST REPORT %dS", label, age / 30);
                else
                    snprintf(caption, sizeof caption, "%s", label);
                render_text(r, x + 10, y + h - 15, 1, caption, ink);
            }
        }
        float distance = hypotf(px - pick_x, py - pick_y);
        if (distance < near) {
            best = i;
            near = distance;
        }
    }
    return best;
}
void chart_draw(App *a, int width) { chart(a, width, 0, 0, true); }
void chart_pick(App *a, int width, float x, float y) {
    int i = chart(a, width, x, y, false);
    if (i < 0)
        return;
    if (a->aiming) {
        a->destination = i;
        a->ground_target[0] = 0;
        a->aiming = false;
        a->pointed = false;
        a->panel = CREW_JOB;
        return;
    }
    for (int j = 0; j < a->sim.crew_count; j++)
        if (a->sim.roster[j] == i) {
            a->member = j;
            a->destination = i;
            a->listing = 0;
            a->panel = CREW_JOB;
            a->ground_target[0] = 0;
            return;
        }
    a->destination = i;
}
