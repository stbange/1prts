#include "app.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool walk(const Sim *s, int id) {
    const Place *item = &s->place[id];
    Shape sh = world_asset_shape(s, id);
    V3 origin;
    if (!sh.w || !world_object(s, id, &origin))
        return false;
    int fixture = -1, crew = 0;
    for (int i = 0; i < s->places; i++)
        if (s->place[i].parent == id + 1) {
            if (!strcmp(s->place[i].family, "fixture"))
                fixture = i;
            if (!strcmp(s->place[i].family, "operator"))
                crew++;
        }
    if (fixture < 0 || !crew)
        return false;
    Camera c = {.aboard = -1, .yaw = world_angle(s, id)};
    c.pos = to_world((V3){0, 0, sh.d * .5f + 1.12f}, origin, c.yaw);
    if (!strcmp(item->type, "station")) {
        for (int i = 0; i < s->places; i++)
            if (!strcmp(s->place[i].key, "lark")) {
                c.aboard = i;
                c.local = (V3){0, .84f, -world_asset_shape(s, i).d * .49f};
            }
    }
    if (sh.boardable && (!world_interact(s, &c) || c.aboard != id))
        return false;
    float goal = s->place[fixture].pos.z;
    for (int i = 0; i < (int)((sh.d + 8) * 16); i++) {
        V3 eye = camera_eye(&c, s);
        V3 p = to_local(eye, origin, world_angle(s, id));
        if (p.z < goal + .25f)
            break;
        camera_move(&c, s, 0, 1, 1.f / 30);
    }
    V3 visit = to_local(camera_eye(&c, s), origin, world_angle(s, id));
    if (fabsf(visit.z - goal) > 1.3f || fabsf(visit.x) > 2.5f)
        return false;
    for (int i = 0; i < (int)((sh.d + 9) * 16); i++) {
        V3 p = to_local(camera_eye(&c, s), origin, world_angle(s, id));
        float exit_at = !strcmp(item->type, "station") ? sh.d * .5f - 2.5f
                        : item->hull                   ? sh.d * .5f + 1.5f
                                                       : sh.d * .4f;
        if (p.z >= exit_at && c.aboard == id) {
            if (world_interact(s, &c) && c.aboard >= 0)
                return true;
        }
        if (p.z > sh.d * .5f + .4f && c.aboard < 0)
            return true;
        camera_move(&c, s, 0, -1, 1.f / 30);
    }
    return false;
}
void rooms_test(const char *root) {
    const char *operations[] = {"gate-six",   "tide-window", "compact-bank", "westbound",
                                "white-mile", "night-mail",  "high-water"};
    char passed[80][32] = {0}, expected[80][32] = {0};
    int total = 0, n = 0;
    for (int k = 0; k < 7; k++) {
        Sim s;
        if (!sim_open_mission(&s, root, operations[k]))
            exit(1);
        if (k == 0) {
            const char *orders[] = {"mill", "harvest", "motor", "armor", "watch", "section"};
            const int delays[] = {120, 300, 300, 300, 120, 120};
            for (int j = 0; j < 6; j++) {
                sim_order(&s, orders[j]);
                sim_step(&s, delays[j]);
            }
        }
        for (int i = 0; i < s.places; i++) {
            const Place *p = &s.place[i];
            if (strcmp(p->family, "fixture"))
                continue;
            int host = p->parent - 1;
            const char *type = s.place[host].type;
            if (k == 6 && !strcmp(type, "station")) {
                if (!sim_order(&s, "module") || !sim_step(&s, 541) || !sim_order(&s, "lift") ||
                    !sim_step(&s, 1801) || !sim_order(&s, "dock") || !sim_step(&s, 661))
                    exit(1);
            }
            int known = 0, done = 0;
            for (int j = 0; j < n; j++)
                if (!strcmp(expected[j], type))
                    known = 1;
            if (!known)
                snprintf(expected[n++], 32, "%s", type);
            for (int j = 0; j < total; j++)
                if (!strcmp(passed[j], type))
                    done = 1;
            if (!done && walk(&s, host)) {
                snprintf(passed[total++], 32, "%s", type);
                printf("walk %s %s %s\n", type, operations[k], p->name);
            }
        }
        sim_close(&s);
    }
    if (total != 61 || n != 61) {
        for (int i = 0; i < n; i++) {
            bool ok = false;
            for (int j = 0; j < total; j++)
                if (!strcmp(expected[i], passed[j]))
                    ok = true;
            if (!ok)
                fprintf(stderr, "Missing interior walk %s\n", expected[i]);
        }
        fprintf(stderr, "Walked %d of %d interiors.\n", total, n);
        exit(1);
    }
}
