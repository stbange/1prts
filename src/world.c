#include "game.h"
#include <math.h>
#include <string.h>
static float clamp(float v, float lo, float hi) {
    return fminf(hi, fmaxf(v, lo));
}
Mat mat_mul(Mat a, Mat b) {
    Mat c = {{0}};
    for (int j = 0; j < 4; j++)
        for (int i = 0; i < 4; i++)
            for (int k = 0; k < 4; k++)
                c.v[j * 4 + i] += a.v[k * 4 + i] * b.v[j * 4 + k];
    return c;
}
Mat mat_projection(float aspect, float fov) {
    Mat m = {{0}};
    float f = 1 / tanf(fov * .5f), near = .1f, far = 1150;
    m.v[0] = f / aspect;
    m.v[5] = f;
    m.v[10] = (far + near) / (near - far);
    m.v[11] = -1;
    m.v[14] = 2 * far * near / (near - far);
    return m;
}
Mat mat_view(V3 p, float yaw, float pitch) {
    float c = cosf(yaw), s = sinf(yaw), a = cosf(pitch), b = sinf(pitch);
    Mat m = {{c, -s * b, -s * a, 0, 0, a, -b, 0, s, c * b, c * a, 0, 0, 0, 0, 1}};
    m.v[12] = -c * p.x - s * p.z;
    m.v[13] = s * b * p.x - a * p.y - c * b * p.z;
    m.v[14] = s * a * p.x + b * p.y - c * a * p.z;
    return m;
}
V3 to_world(V3 p, V3 base, float yaw) {
    float c = cosf(yaw), s = sinf(yaw);
    return (V3){base.x + c * p.x - s * p.z, base.y + p.y, base.z + s * p.x + c * p.z};
}
V3 to_local(V3 p, V3 base, float yaw) {
    return to_world((V3){p.x - base.x, p.y - base.y, p.z - base.z}, (V3){0, 0, 0}, -yaw);
}
Shape world_shape(const char *s) {
    static const struct {
        const char *id;
        Shape s;
    } shapes[] = {{"survey", {18, 7.1f, 13, false}},    {"dynamo", {16, 8.2f, 13, false}},
                  {"mill", {17, 8.6f, 12, false}},      {"watch", {14, 6.6f, 10, false}},
                  {"motor", {21, 8.2f, 14, false}},     {"gate", {17, 7, 9, false}},
                  {"mole", {7.45f, 4, 17.04f, true}},   {"lintel", {3.64f, 2.6f, 8.32f, true}},
                  {"wake", {6.05f, 3.8f, 13.84f, true}}};
    for (size_t i = 0; i < sizeof shapes / sizeof *shapes; i++)
        if (!strcmp(s, shapes[i].id))
            return shapes[i].s;
    return (Shape){0};
}
Shape world_asset_shape(const Sim *g, int i) {
    const Place *p = &g->place[i];
    if (p->size.x > 0 && strcmp(p->family, "people"))
        return (Shape){p->size.x, p->size.y, p->size.z, p->hull};
    return world_shape(p->model);
}
static int parent_of(const Place *p) { return (p->moved ? p->linked : p->parent) - 1; }
float world_angle(const Sim *g, int i) {
    float yaw = 0;
    for (int n = 0; i >= 0 && n < g->places; n++) {
        const Place *p = &g->place[i];
        yaw += p->moved ? p->facing : p->yaw;
        i = parent_of(p);
    }
    return yaw;
}
static bool base_object(const Sim *g, int i, V3 *out) {
    const Place *p = &g->place[i];
    const char *v = p->condition;
    *out = p->pos;
    if (p->moved) {
        *out = p->current;
        return true;
    }
    if (!*v)
        return true;
    if (!strcmp(v, "fitters")) {
        for (int j = 0; j < g->places; j++) {
            const Place *site = &g->place[j];
            if (g->blue.work_left && !strcmp(site->model, g->blue.work)) {
                float t = clamp((150 - g->blue.work_left) / 30.f, 0, 1);
                out->x += (site->pos.x + 4 - out->x) * t;
                out->z += (site->pos.z + 6 - out->z) * t;
                break;
            }
        }
        return true;
    }
    if (!strcmp(v, "redbase"))
        return true;
    if (!strcmp(v, "redmill"))
        return g->red.mill;
    if (!strcmp(v, "redwatch"))
        return g->red.watch;
    if (!strcmp(v, "redmotor"))
        return g->red.motor;
    if (!strcmp(v, "redhaul")) {
        float t = 1 - fabsf(g->red.haul / 150.f - 1);
        out->x += 12 * t;
        out->z += 7 * t;
        return true;
    }
    if (!strcmp(v, "mill"))
        return g->blue.mill;
    if (!strcmp(v, "watch"))
        return g->blue.watch;
    if (!strcmp(v, "motor"))
        return g->blue.motor;
    if (!strcmp(v, "haul") || !strcmp(v, "haulcrew")) {
        float t = 1 - fabsf(g->blue.haul / 150.f - 1);
        out->x -= t * 8;
        out->z -= t * 31;
        return true;
    }
    if (!strcmp(v, "armor") || !strcmp(v, "tankcrew")) {
        float t = g->blue.drive / 210.f, exit = 1.f - g->blue.rollout / 120.f;
        out->x -= t * 7 + exit * 29;
        out->z += exit * 22 - t * 58;
        return g->blue.armor;
    }
    if (!strcmp(v, "enemyarmor")) {
        out->x -= g->red.drive / 210.f * 13;
        out->z += g->red.drive / 210.f * 34;
        return g->red.armor;
    }
    if (!strcmp(v, "section")) {
        out->x -= g->blue.walk / 300.f * 17;
        out->z -= g->blue.walk / 300.f * 47;
        return g->blue.section;
    }
    if (!strcmp(v, "enemysection")) {
        out->x -= g->red.walk / 300.f * 12;
        out->z += g->red.walk / 300.f * 21;
        return g->red.section;
    }
    return false;
}
bool world_object(const Sim *g, int i, V3 *out) {
    if (!base_object(g, i, out))
        return false;
    int parent = parent_of(&g->place[i]);
    for (int depth = 0; parent >= 0 && depth < g->places; depth++) {
        V3 origin;
        if (!base_object(g, parent, &origin))
            return false;
        const Place *p = &g->place[parent];
        *out = to_world(*out, origin, p->moved ? p->facing : p->yaw);
        parent = parent_of(p);
    }
    return parent < 0;
}
void camera_reset(Camera *c) {
    *c = (Camera){.pos = {3, 0, 22}, .yaw = -.052f, .pitch = .075f, .aboard = -1};
}
V3 camera_eye(const Camera *c, const Sim *g) {
    V3 p = c->pos;
    if (c->aboard >= 0) {
        V3 origin;
        world_object(g, c->aboard, &origin);
        p = to_world(c->local, origin, world_angle(g, c->aboard));
    }
    p.y += 1.72f;
    return p;
}
void camera_look(Camera *c, float dx, float dy) {
    c->yaw = remainderf(c->yaw + dx, 2 * ORRA_PI);
    c->pitch = clamp(c->pitch + dy, -1.30f, 1.30f);
}
static bool passable(const Sim *g, V3 p) {
    if (fabsf(p.x) > 420 || p.z < -1100 || p.z > 220)
        return false;
    if (!strcmp(g->terrain, "terrain") && p.x > 71 && p.x < 80 && p.z < 38)
        return false;
    if (!strcmp(g->terrain, "estuary") && p.z < -30 && p.z > -170 && fabsf(p.x) < 64)
        return false;
    for (int i = 0; i < g->places; i++) {
        const Place *place = &g->place[i];
        Shape sh = world_asset_shape(g, i);
        V3 origin;
        if (sh.w == 0 || !world_object(g, i, &origin))
            continue;
        V3 a = to_local(p, origin, world_angle(g, i));
        if (a.y < -1.8f || a.y > sh.h + 1)
            continue;
        float x = fabsf(a.x), z = a.z;
        if (sh.boardable) {
            if (x < sh.w * (place->hull ? .49f : .81f) + .16f && fabsf(z) < sh.d * .5f + .18f)
                return false;
        } else if (!strcmp(place->model, "gate")) {
            if (x > 8.3f && x < 18 && fabsf(z) < 5.6f)
                return false;
            if (x < 8.5f && fabsf(z + 4) < .5f)
                return false;
            if (x > 4.0f && x < 8.3f && z > -1.4f && z < 4)
                return false;
        } else {
            float w = sh.w * .5f, d = sh.d * .5f;
            if (x < w + .42f && fabsf(z) < d + .42f &&
                (x > w - .42f || z < -d + .42f || (z > d - .42f && x > 2.45f)))
                return false;
        }
    }
    return true;
}
static float pavement_height(V3 p) {
    if (p.x >= -23.4f && p.x <= -15.6f && p.z >= 0 && p.z <= 7)
        return 5.8f;
    if (fabsf(p.x + 19.5f) <= 2.2f && p.z > 7 && p.z < 20)
        return ceilf((20 - p.z) / .65f) * .29f;
    return 0;
}
static float ramp_floor(float z, float length) {
    return .84f * clamp(1 - (z - length * .5f) / 2.4f, 0, 1);
}
static bool occupied(const Sim *g, int aboard, V3 p) {
    for (int i = 0; i < g->places; i++) {
        const Place *seat = &g->place[i];
        if (seat->parent == aboard + 1 && !strcmp(seat->family, "operator") &&
            fabsf(p.y - seat->pos.y) < 1.2f && hypotf(p.x - seat->pos.x, p.z - seat->pos.z) < .39f)
            return true;
    }
    return false;
}
void camera_move(Camera *c, const Sim *g, float strafe, float forward, float sec) {
    if (strafe == 0 && forward == 0)
        return;
    float len = hypotf(strafe, forward);
    if (len > 1) {
        strafe /= len;
        forward /= len;
    }
    float speed = (c->aboard >= 0 ? 2.6f : 6.0f) * sec;
    float yaw = c->yaw - (c->aboard >= 0 ? world_angle(g, c->aboard) : 0);
    V3 delta = {(strafe * cosf(yaw) + forward * sinf(yaw)) * speed, 0,
                (strafe * sinf(yaw) - forward * cosf(yaw)) * speed};
    if (c->aboard >= 0) {
        Shape sh = world_asset_shape(g, c->aboard);
        const Place *hull = &g->place[c->aboard];
        float side = hull->hull ? .47f : .32f, end = hull->hull ? .49f : .30f;
        V3 next = c->local;
        next.x = clamp(next.x + delta.x, -sh.w * side, sh.w * side);
        if (!occupied(g, c->aboard, next))
            c->local.x = next.x;
        next = c->local;
        next.z = clamp(next.z + delta.z, -sh.d * end, hull->hull ? sh.d * .5f + 3.5f : sh.d * .45f);
        if (!occupied(g, c->aboard, next))
            c->local.z = next.z;
        if (hull->hull && c->local.z >= sh.d * .45f)
            c->local.y = ramp_floor(c->local.z, sh.d);
        if (!strcmp(hull->family, "ferry") || !strcmp(hull->family, "station")) {
            float bridge = fminf(5.8f, sh.h * .47f);
            if (fabsf(c->local.x + sh.w * .29f) < 1.3f && c->local.z <= sh.d * .08f &&
                c->local.z >= sh.d * .08f - 14.3f)
                c->local.y = .84f + bridge * clamp((sh.d * .08f - c->local.z) / 13.44f, 0, 1);
            else if ((c->local.z > sh.d * .08f && c->local.z < sh.d * .45f) ||
                     c->local.z < -sh.d * .405f)
                c->local.y = .84f;
        }
        if (hull->hull && delta.z > 0 && c->local.z >= sh.d * .5f + 2.4f && fabsf(c->local.x) < 2.2f)
            world_interact(g, c);
    } else {
        V3 next = c->pos;
        next.x += delta.x;
        next.y = !strcmp(g->terrain, "terrain") ? pavement_height(next) : 0;
        if (c->pos.y > 0 && strcmp(g->terrain, "terrain"))
            next.y = fmaxf(0, c->pos.y - .35f * speed);
        if (fabsf(next.y - c->pos.y) < .47f && passable(g, next))
            c->pos = next;
        V3 before = c->pos;
        next = c->pos;
        next.z += delta.z;
        next.y = !strcmp(g->terrain, "terrain") ? pavement_height(next) : 0;
        if (c->pos.y > 0 && strcmp(g->terrain, "terrain"))
            next.y = fmaxf(0, c->pos.y - .35f * speed);
        if (fabsf(next.y - c->pos.y) < .47f && passable(g, next))
            c->pos = next;
        if (strcmp(g->terrain, "terrain")) {
            int near = world_nearest(g, c);
            if (near >= 0 && g->place[near].hull) {
                V3 origin;
                world_object(g, near, &origin);
                float yaw = world_angle(g, near);
                V3 from = to_local(before, origin, yaw), to = to_local(c->pos, origin, yaw);
                float threshold = world_asset_shape(g, near).d * .5f + 2.4f;
                if (from.z >= threshold && to.z < threshold && fabsf(to.x) < 2.2f)
                    world_interact(g, c);
            }
        }
    }
}
bool world_terminal(const Sim *g, const Camera *c) {
    if (c->aboard >= 0)
        return false;
    for (int i = 0; i < g->places; i++) {
        const Place *p = &g->place[i];
        if ((!p->station || p->hull) && strcmp(p->model, "survey") && strcmp(p->model, "gate"))
            continue;
        V3 origin;
        if (!world_object(g, i, &origin))
            continue;
        V3 pos = to_local(c->pos, origin, world_angle(g, i));
        if (fabsf(pos.x) < 4 && pos.z < 1 && pos.z > -6.1f)
            return true;
    }
    return false;
}
static V3 feet(const Sim *g, const Camera *c) {
    V3 eye = camera_eye(c, g);
    eye.y -= 1.72f;
    return eye;
}
int world_nearest(const Sim *g, const Camera *c) {
    if (c->aboard >= 0 && !g->place[c->aboard].hull)
        return c->aboard;
    int best = -1;
    float distance = 3.5f;
    V3 observer = feet(g, c);
    for (int i = 0; i < g->places; i++) {
        const Place *p = &g->place[i];
        V3 origin;
        Shape sh = world_asset_shape(g, i);
        if (!sh.boardable || i == c->aboard || !world_object(g, i, &origin))
            continue;
        V3 hatch = to_world((V3){0, p->hull ? .84f : 0, sh.d * .53f}, origin, world_angle(g, i));
        if (p->hull)
            hatch = to_world((V3){0, .84f, sh.d * .50f}, origin, world_angle(g, i));
        float d = hypotf(hypotf(observer.x - hatch.x, observer.z - hatch.z), observer.y - hatch.y);
        if (c->aboard >= 0 && !strcmp(g->place[c->aboard].family, "station") &&
            !strcmp(p->family, "tender")) {
            V3 bow = to_world((V3){0, .84f, -sh.d * .49f}, origin, world_angle(g, i));
            d = fminf(d, hypotf(hypotf(observer.x - bow.x, observer.z - bow.z), observer.y - bow.y));
        }
        if (d < distance) {
            distance = d;
            best = i;
        }
    }
    if (best < 0 && c->aboard >= 0) {
        Shape sh = world_asset_shape(g, c->aboard);
        if (c->local.z > sh.d * .49f - 2.8f && fabsf(c->local.x) < 3.5f)
            best = c->aboard;
    }
    return best;
}
bool world_interact(const Sim *g, Camera *c) {
    int i = world_nearest(g, c);
    if (i < 0)
        return false;
    const Place *p = &g->place[i];
    Shape sh = world_asset_shape(g, i);
    V3 origin;
    world_object(g, i, &origin);
    if (c->aboard == i) {
        int parent = parent_of(p);
        V3 exit = to_world((V3){0, 0, sh.d * (p->hull ? .5f : .57f) + (p->hull ? .32f : .55f)}, origin,
                           world_angle(g, i));
        if (p->hull) {
            exit = feet(g, c);
            if (parent < 0 && (origin.y > 3 || !passable(g, (V3){exit.x, 0, exit.z + 2})))
                return false;
            if (parent < 0 && !strcmp(p->key, "kestrel") && origin.z < -.3f && origin.z > -229.7f)
                return false;
        }
        c->aboard = parent;
        if (parent >= 0) {
            V3 deck;
            world_object(g, parent, &deck);
            c->local = to_local(exit, deck, world_angle(g, parent));
        } else
            c->pos = exit;
    } else {
        V3 here = feet(g, c);
        c->aboard = i;
        if (p->hull) {
            c->local = to_local(here, origin, world_angle(g, i));
            c->local.z = clamp(c->local.z, -sh.d * .49f, sh.d * .5f + 3.5f);
        } else
            c->local = (V3){0, .84f, sh.d * .39f};
    }
    return true;
}
