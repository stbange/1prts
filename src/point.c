#include "app.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static bool slab(float p, float v, float low, float high, float *near, float *far) {
    if (fabsf(v) < .00001f)
        return p >= low && p <= high;
    float a = (low - p) / v, b = (high - p) / v;
    *near = fmaxf(*near, fminf(a, b));
    *far = fminf(*far, fmaxf(a, b));
    return *near <= *far;
}
WorldHit world_point(const Sim *s, V3 eye, V3 ray, int aboard) {
    float length = sqrtf(ray.x * ray.x + ray.y * ray.y + ray.z * ray.z);
    WorldHit best = {.place = -1, .distance = 1100};
    if (length < .0001f)
        return best;
    ray = (V3){ray.x / length, ray.y / length, ray.z / length};
    if (ray.y < -.0001f) {
        float d = -eye.y / ray.y;
        V3 p = {eye.x + d * ray.x, 0, eye.z + d * ray.z};
        if (d > 0 && d < best.distance && world_ground(s, p)) {
            best.ground = true;
            best.distance = d;
            best.point = p;
        }
    }
    for (int i = 0; i < s->places; i++) {
        const Place *row = &s->place[i];
        if (i == aboard || !strcmp(row->model, s->terrain))
            continue;
        V3 origin;
        if (!world_object(s, i, &origin))
            continue;
        Shape sh = world_asset_shape(s, i);
        if (sh.w <= 0) {
            if (!strcmp(row->family, "marker"))
                sh = (Shape){6, 1.8f, 6, false};
            else if (!strcmp(row->type, "lift-car"))
                sh = (Shape){3.4f, 1, 3.5f, false};
            else if (!strcmp(row->model, "worker") || !strcmp(row->family, "people") ||
                     strstr(row->model, "section"))
                sh = (Shape){1, 1.88f, 1, false};
            else
                continue;
        }
        float yaw = world_angle(s, i);
        V3 p = to_local(eye, origin, yaw), v = to_local(ray, (V3){0}, yaw);
        float near = 0, far = best.distance;
        if (!slab(p.x, v.x, -sh.w * .5f, sh.w * .5f, &near, &far) ||
            !slab(p.z, v.z, -sh.d * .5f, sh.d * .5f, &near, &far) ||
            !slab(p.y, v.y, 0, sh.h, &near, &far) || near <= .04f || near >= best.distance)
            continue;
        best =
            (WorldHit){.place = i,
                       .distance = near,
                       .point = {eye.x + near * ray.x, eye.y + near * ray.y, eye.z + near * ray.z}};
    }
    return best;
}
bool app_mark(App *a, V3 eye, V3 direction) {
    if (a->panel != FIELD)
        return false;
    WorldHit hit = world_point(&a->sim, eye, direction, a->camera.aboard);
    if (hit.place < 0 && !hit.ground)
        return false;
    if (a->aiming) {
        a->ground_target[0] = 0;
        if (hit.place >= 0)
            a->destination = hit.place;
        else
            snprintf(a->ground_target, sizeof a->ground_target, "g-%d-%d",
                     (int)lroundf(hit.point.x) + 420, (int)lroundf(hit.point.z) + 1100);
        a->aiming = false;
        a->pointed = true;
        a->panel = CREW_JOB;
        return true;
    }
    if (hit.place < 0)
        return false;
    const Place *p = &a->sim.place[hit.place];
    int recipient = hit.place;
    if (p->parent &&
        (p->family[0] && (!strcmp(p->family, "fixture") || !strcmp(p->family, "operator"))))
        recipient = p->parent - 1;
    for (int m = 0; m < a->sim.crew_count; m++)
        if (a->sim.roster[m] == recipient) {
            a->member = m;
            a->listing = 0;
            a->destination = recipient;
            a->panel = CREW_JOB;
            return true;
        }
    if (p->key[0]) {
        snprintf(a->message, sizeof a->message, "%s / %s", p->name[0] ? p->name : p->key, p->owner);
        a->message_ticks = 120;
        return true;
    }
    return false;
}
void app_screen_point(App *a, float x, float y) {
    float aspect = a->height > 0 ? (float)a->width / a->height : 1;
    float scale = tanf(.5f * 1.04f);
    const float dy = (1 - 2 * y) * scale;
    V3 ray = {(2 * x - 1) * aspect * scale, sinf(a->camera.pitch) + dy * cosf(a->camera.pitch),
              -cosf(a->camera.pitch) + dy * sinf(a->camera.pitch)};
    app_mark(a, camera_eye(&a->camera, &a->sim), to_world(ray, (V3){0}, a->camera.yaw));
}
