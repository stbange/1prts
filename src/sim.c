#include "game.h"
#include <lauxlib.h>
#include <lualib.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

static void copy(char *out, size_t size, const char *s) {
    snprintf(out, size, "%s", s ? s : "");
}
static bool fail(Sim *g, const char *message) {
    copy(g->error, sizeof g->error, message);
    return false;
}
static int number(lua_State *L, int table, const char *key) {
    lua_getfield(L, table, key);
    int x = (int)lua_tointeger(L, -1);
    lua_pop(L, 1);
    return x;
}
static bool flag(lua_State *L, int table, const char *key) {
    lua_getfield(L, table, key);
    bool x = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return x;
}
static void string_field(lua_State *L, int table, const char *key, char *out, size_t size) {
    lua_getfield(L, table, key);
    copy(out, size, lua_tostring(L, -1));
    lua_pop(L, 1);
}
static void string_at(lua_State *L, int index, char *out, size_t size) {
    lua_geti(L, -1, index);
    copy(out, size, lua_tostring(L, -1));
    lua_pop(L, 1);
}
static float float_at(lua_State *L, int index) {
    lua_geti(L, -1, index);
    float x = (float)lua_tonumber(L, -1);
    lua_pop(L, 1);
    return x;
}
static bool call(Sim *g, int nargs, int nresults) {
    if (lua_pcall(g->L, nargs, nresults, 0) == LUA_OK)
        return true;
    fail(g, lua_tostring(g->L, -1));
    lua_pop(g->L, 1);
    return false;
}
static bool load_table(Sim *g, const char *file) {
    if (g->bundle != LUA_NOREF) {
        lua_rawgeti(g->L, LUA_REGISTRYINDEX, g->bundle);
        lua_getfield(g->L, -1, file);
        lua_remove(g->L, -2);
        return lua_istable(g->L, -1) || fail(g, "Content entry must return a table");
    }
    char path[1220];
    snprintf(path, sizeof path, "%s/scripts/%s.lua", g->root, file);
    if (luaL_loadfile(g->L, path) != LUA_OK) {
        fail(g, lua_tostring(g->L, -1));
        lua_pop(g->L, 1);
        return false;
    }
    if (!call(g, 0, 1))
        return false;
    return lua_istable(g->L, -1) || fail(g, "Content script must return a table");
}
static void method(Sim *g, const char *name) {
    *(Sim **)lua_getextraspace(g->L) = g;
    lua_rawgeti(g->L, LUA_REGISTRYINDEX, g->rules);
    lua_getfield(g->L, -1, name);
    lua_remove(g->L, -2);
}
static void read_side(lua_State *L, const char *name, Side *a) {
    lua_getfield(L, -1, name);
    a->alloy = number(L, -1, "alloy");
    a->loads = number(L, -1, "loads");
    a->haul = number(L, -1, "haul");
    a->draw = number(L, -1, "draw");
    a->hold = number(L, -1, "hold");
    a->walk = number(L, -1, "walk");
    a->rollout = number(L, -1, "rollout");
    a->drive = number(L, -1, "drive");
    a->stock = number(L, -1, "stock");
    a->ready = number(L, -1, "readiness");
    a->harvesting = flag(L, -1, "harvesting");
    a->power = flag(L, -1, "power");
    a->mill = flag(L, -1, "mill");
    a->watch = flag(L, -1, "watch");
    a->motor = flag(L, -1, "motor");
    a->section = flag(L, -1, "section");
    a->armor = flag(L, -1, "armor");
    a->advance = flag(L, -1, "advance");
    a->guard = flag(L, -1, "guard");
    a->work[0] = 0;
    a->work_left = 0;
    lua_getfield(L, -1, "queues");
    const char *bays[] = {"fit", "watch", "motor"};
    for (int i = 0; i < 3; i++) {
        lua_getfield(L, -1, bays[i]);
        if (lua_istable(L, -1)) {
            string_field(L, -1, "id", a->work, sizeof a->work);
            a->work_left = number(L, -1, "left");
            lua_pop(L, 1);
            break;
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 2);
}
static V3 vector(lua_State *L, const char *name) {
    lua_getfield(L, -1, name);
    V3 p = {0};
    if (lua_istable(L, -1))
        p = (V3){float_at(L, 1), float_at(L, 2), float_at(L, 3)};
    lua_pop(L, 1);
    return p;
}
static void moving_frames(Sim *g) {
    lua_State *L = g->L;
    for (int i = 0; i < g->places; ++i) {
        g->place[i].moved = false;
        g->place[i].berth = 0;
        g->place[i].ramp = 0;
    }
    lua_getfield(L, -1, "frames");
    if (lua_istable(L, -1)) {
        lua_pushnil(L);
        while (lua_next(L, -2)) {
            int i = (int)lua_tointeger(L, -2) - 1;
            if (i >= 0 && i < g->places && lua_istable(L, -1)) {
                Place *p = &g->place[i];
                p->moved = true;
                p->current = (V3){float_at(L, 1), float_at(L, 2), float_at(L, 3)};
                p->facing = float_at(L, 4);
                p->berth = (int)float_at(L, 6);
                p->ramp = float_at(L, 7);
                int parent = (int)float_at(L, 5);
                p->linked = parent >= 0 && parent <= g->places && parent != i + 1 ? parent : 0;
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
}
bool sim_contact(const Sim *g, const char *target, V3 *p, int *age) {
    lua_State *L = g->L;
    int seen = -1;
    lua_rawgeti(L, LUA_REGISTRYINDEX, g->state);
    lua_getfield(L, -1, "contacts");
    if (lua_istable(L, -1)) {
        lua_pushnil(L);
        while (lua_next(L, -2)) {
            char key[64];
            string_field(L, -1, "target", key, sizeof key);
            int tick = number(L, -1, "tick");
            if (!strcmp(key, target) && tick > seen) {
                *p = (V3){number(L, -1, "x"), 0, number(L, -1, "z")};
                seen = tick;
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 2);
    *age = g->tick - seen;
    return seen >= 0;
}
void sim_crew_report(const Sim *g, const char *key, char *out, size_t size) {
    lua_State *L = g->L;
    out[0] = 0;
    lua_rawgeti(L, LUA_REGISTRYINDEX, g->state);
    lua_getfield(L, -1, "contacts");
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, key);
        if (lua_istable(L, -1)) {
            char target[40];
            string_field(L, -1, "target", target, sizeof target);
            snprintf(out, size, "%s / %d E %d N / %dS AGO", target, number(L, -1, "x"),
                     -number(L, -1, "z"), (g->tick - number(L, -1, "tick")) / 30);
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 2);
}
void sim_inspect(Sim *g, const char *key, char *out, size_t size) {
    out[0] = 0;
    method(g, "inspect");
    if (!lua_isfunction(g->L, -1)) { lua_pop(g->L, 1); return; }
    lua_rawgeti(g->L, LUA_REGISTRYINDEX, g->state);
    lua_pushstring(g->L, key);
    if (call(g, 2, 1)) {
        copy(out, size, lua_tostring(g->L, -1));
        lua_pop(g->L, 1);
    }
}
void sim_log(const Sim *g, int offset, char *out, size_t size) {
    lua_State *L = g->L;
    out[0] = 0;
    lua_rawgeti(L, LUA_REGISTRYINDEX, g->state);
    lua_getfield(L, -1, "log");
    if (lua_istable(L, -1)) {
        lua_geti(L, -1, (int)lua_rawlen(L, -1) - offset);
        if (lua_istable(L, -1)) {
            char report[156];
            string_field(L, -1, "text", report, sizeof report);
            snprintf(out, size, "%ds / %s", (g->tick - number(L, -1, "tick")) / 30, report);
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 2);
}
int sim_crew_status(const Sim *g, const char *key, int *age) {
    lua_State *L = g->L;
    lua_rawgeti(L, LUA_REGISTRYINDEX, g->state);
    const char *fields[] = {"service", "roster", "reports"};
    int status = 0;
    *age = 0;
    for (int i = 0; i < 3; i++) {
        lua_getfield(L, -1, fields[i]);
        if (lua_istable(L, -1)) {
            lua_getfield(L, -1, key);
            if (!lua_isnil(L, -1)) {
                if (i == 2)
                    *age = g->tick - (int)lua_tointeger(L, -1);
                else if (!status)
                    status = i + 1;
            }
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    return status;
}
static bool reserve_places(Sim *g, size_t count) {
    if (count <= (size_t)g->capacity) return true;
    if (count > INT_MAX / 2 || count > SIZE_MAX / (2 * sizeof(Place))) return false;
    int capacity = (int)count + (int)count / 2;
    Place *places = calloc((size_t)capacity, sizeof *places);
    int *roster = calloc((size_t)capacity, sizeof *roster);
    if (!places || !roster) { free(places); free(roster); return false; }
    if (g->places) memcpy(places, g->place, (size_t)g->places * sizeof *places);
    if (g->crew_count) memcpy(roster, g->roster, (size_t)g->crew_count * sizeof *roster);
    free(g->place); free(g->roster);
    g->place = places; g->roster = roster; g->capacity = capacity;
    return true;
}
static int prepare_spawn(lua_State *L) {
    Sim *g = *(Sim **)lua_getextraspace(L);
    lua_Integer count = luaL_checkinteger(L, 1);
    lua_pushboolean(L, count > 0 && reserve_places(g, (size_t)count));
    return 1;
}
static bool sync_site(Sim *g) {
    lua_State *L = g->L;
    lua_rawgeti(L, LUA_REGISTRYINDEX, g->site_ref);
    size_t count = lua_rawlen(L, -1);
    if (!count || !reserve_places(g, count)) return fail(g, "Bad site size or insufficient memory");
    if ((int)count < g->places) {
        g->places = 0;
        g->crew_count = 0;
    }
    for (int i = g->places; i < (int)count; i++) {
        Place *p = &g->place[i];
        memset(p, 0, sizeof *p);
        lua_geti(L, -1, i + 1);
        if (!lua_istable(L, -1))
            return fail(g, "Invalid site entry");
        string_at(L, 1, p->model, sizeof p->model);
        p->pos = (V3){float_at(L, 2), float_at(L, 3), float_at(L, 4)};
        p->yaw = float_at(L, 5);
        string_at(L, 6, p->condition, sizeof p->condition);
        string_field(L, -1, "key", p->key, sizeof p->key);
        string_field(L, -1, "name", p->name, sizeof p->name);
        string_field(L, -1, "family", p->family, sizeof p->family);
        string_field(L, -1, "role", p->role, sizeof p->role);
        string_field(L, -1, "asset", p->asset, sizeof p->asset);
        string_field(L, -1, "type", p->type, sizeof p->type);
        string_field(L, -1, "owner", p->owner, sizeof p->owner);
        if (!*p->key) snprintf(p->key, sizeof p->key, "site-%d", i + 1);
        if (!*p->type) copy(p->type, sizeof p->type, p->model);
        if (p->role[0] && strcmp(p->owner, "rival")) g->roster[g->crew_count++] = i;
        p->size = vector(L, "size");
        p->parent = number(L, -1, "parent");
        if (p->parent < 0 || p->parent > i)
            return fail(g, "Invalid scene parent");
        p->hull = flag(L, -1, "hull");
        p->station = flag(L, -1, "station");
        lua_pop(L, 1);
    }
    g->places = (int)count;
    lua_pop(L, 1);
    return true;
}

bool sim_snapshot(Sim *g) {
    if (!sync_site(g)) return false;
    lua_State *L = g->L;
    lua_rawgeti(L, LUA_REGISTRYINDEX, g->state);
    g->tick = number(L, -1, "tick");
    g->relief_at = number(L, -1, "relief_at");
    string_field(L, -1, "report", g->report, sizeof g->report);
    string_field(L, -1, "result", g->result, sizeof g->result);
    g->clock = number(L, -1, "clock");
    g->progress = number(L, -1, "progress");
    g->reserved = number(L, -1, "reserved");
    g->report_at = number(L, -1, "report_at");
    string_field(L, -1, "objective", g->objective, sizeof g->objective);
    string_field(L, -1, "stage", g->stage, sizeof g->stage);
    string_field(L, -1, "advice", g->advice, sizeof g->advice);
    moving_frames(g);
    read_side(L, "blue", &g->blue);
    read_side(L, "red", &g->red);
    lua_pop(L, 1);
    return true;
}
bool sim_reset(Sim *g) {
    for (int i = 0; i < g->places; i++) {
        g->place[i].moved = false;
        g->place[i].berth = 0;
        g->place[i].ramp = 0;
    }
    method(g, "new");
    if (!call(g, 0, 1))
        return false;
    if (!lua_istable(g->L, -1)) {
        lua_pop(g->L, 1);
        return fail(g, "Mission must return a state table");
    }
    const char *sides[] = {"blue", "red"};
    for (int i = 0; i < 2; i++) {
        lua_getfield(g->L, -1, sides[i]);
        if (!lua_istable(g->L, -1)) {
            lua_pop(g->L, 2);
            return fail(g, "Missing faction state");
        }
        lua_getfield(g->L, -1, "queues");
        bool valid = lua_istable(g->L, -1);
        lua_pop(g->L, 2);
        if (!valid) {
            lua_pop(g->L, 1);
            return fail(g, "Missing faction queues");
        }
    }
    luaL_unref(g->L, LUA_REGISTRYINDEX, g->state);
    g->state = luaL_ref(g->L, LUA_REGISTRYINDEX);
    return sim_snapshot(g);
}
static bool read_catalog(Sim *g) {
    const char *names[] = {"units", "buildings", "factions", "missions", "upgrades"};
    lua_State *L = g->L;
    for (int c = 0; c < CATEGORY_COUNT; c++) {
        lua_getfield(L, -1, names[c]);
        Catalog *list = &g->cats[c];
        list->count = (int)lua_rawlen(L, -1);
        if (list->count < 1 || list->count > MAX_CATALOG)
            return fail(g, "Bad catalog size");
        for (int i = 0; i < list->count; i++) {
            lua_geti(L, -1, i + 1);
            Entry *e = &list->entry[i];
            if (!lua_istable(L, -1))
                return fail(g, "Catalog entry must be a table");
            string_field(L, -1, "id", e->id, sizeof e->id);
            string_field(L, -1, "name", e->name, sizeof e->name);
            string_field(L, -1, "role", e->role, sizeof e->role);
            string_field(L, -1, "owner", e->owner, sizeof e->owner);
            if (!e->id[0] || !e->name[0])
                return fail(g, "Missing catalog ID or name");
            for (int j = 0; j < i; j++)
                if (!strcmp(list->entry[j].id, e->id))
                    return fail(g, "Duplicate catalog ID");
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    return true;
}
bool sim_open(Sim *g, const char *root) { return sim_open_mission(g, root, "gate-six"); }

bool sim_open_mission(Sim *g, const char *root, const char *mission) {
    memset(g, 0, sizeof *g);
    copy(g->root, sizeof g->root, root);
    g->state = LUA_NOREF;
    g->L = luaL_newstate();
    if (!g->L)
        return fail(g, "Unable to create Lua state");
    lua_State *L = g->L;
    luaL_openlibs(L);
    *(Sim **)lua_getextraspace(L) = g;
    lua_pushcfunction(L, prepare_spawn);
    lua_setglobal(L, "orra_reserve_scene");
    g->bundle = LUA_NOREF;
    copy(g->mission, sizeof g->mission, mission);
    copy(g->title, sizeof g->title, "Gate Six");
    copy(g->faction, sizeof g->faction, "WEIR ASSEMBLY");
    copy(g->terrain, sizeof g->terrain, "terrain");
    g->spawn = (V3){3, 0, 22};
    g->spawn_yaw = -.052f;
    char filename[1280];
    snprintf(filename, sizeof filename, "%s/content.json", root);
    FILE *manifest = fopen(filename, "rb");
    if (manifest) {
        fclose(manifest);
        snprintf(filename, sizeof filename, "%s/scripts/runtime.lua", root);
        if (luaL_loadfile(L, filename) != LUA_OK)
            return fail(g, lua_tostring(L, -1));
        lua_pushstring(L, root);
        lua_pushstring(L, mission);
        if (!call(g, 2, 1))
            return false;
        string_field(L, -1, "title", g->title, sizeof g->title);
        string_field(L, -1, "faction", g->faction, sizeof g->faction);
        string_field(L, -1, "terrain", g->terrain, sizeof g->terrain);
        const char *voices[] = {"ack.wav",     "blocked.wav", "weir.wav",    "sounding.wav",
                                "terrace.wav", "salt.wav",    "lantern.wav", "meridian.wav"};
        lua_getfield(L, -1, "media");
        if (lua_istable(L, -1))
            for (int i = 0; i < 8; i++)
                string_field(L, -1, voices[i], g->sounds[i], sizeof g->sounds[i]);
        lua_pop(L, 1);
        g->spawn = vector(L, "spawn");
        lua_getfield(L, -1, "spawn");
        g->spawn_yaw = float_at(L, 4);
        lua_pop(L, 1);
        lua_getfield(L, -1, "operations");
        g->operation_count = (int)lua_rawlen(L, -1);
        if (g->operation_count < 1 || g->operation_count > MAX_CATALOG)
            return fail(g, "Operation list size");
        for (int i = 0; i < g->operation_count; i++) {
            lua_geti(L, -1, i + 1);
            Entry *e = &g->operations[i];
            string_field(L, -1, "id", e->id, sizeof e->id);
            string_field(L, -1, "name", e->name, sizeof e->name);
            string_field(L, -1, "role", e->role, sizeof e->role);
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
        g->bundle = luaL_ref(L, LUA_REGISTRYINDEX);
    } else if (strcmp(mission, "gate-six"))
        return fail(g, "Mission requires content.json");
    if (!load_table(g, "catalog") || !read_catalog(g) || !load_table(g, "site"))
        return false;
    g->site_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    if (!sync_site(g)) return false;
    if (!load_table(g, "gate_six"))
        return false;
    g->hold_ticks = number(L, -1, "hold_ticks");
    lua_getfield(L, -1, "actions");
    g->order_count = (int)lua_rawlen(L, -1);
    if (g->order_count < 1 || g->hold_ticks <= 0)
        return fail(g, "Invalid mission rules");
    g->orders = calloc((size_t)g->order_count, sizeof *g->orders);
    if (!g->orders) return fail(g, "Order allocation failed");
    for (int i = 0; i < g->order_count; i++) {
        lua_geti(L, -1, i + 1);
        if (!lua_istable(L, -1))
            return fail(g, "Invalid mission order");
        string_at(L, 1, g->orders[i].id, sizeof g->orders[i].id);
        string_at(L, 2, g->orders[i].label, sizeof g->orders[i].label);
        string_at(L, 3, g->orders[i].actor, sizeof g->orders[i].actor);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    g->rules = luaL_ref(L, LUA_REGISTRYINDEX);
    return sim_reset(g);
}
bool sim_command(Sim *g, int index) {
    if (index < 0 || index >= g->order_count)
        return false;
    return sim_order(g, g->orders[index].id);
}
bool sim_order(Sim *g, const char *id) {
    method(g, "command");
    lua_rawgeti(g->L, LUA_REGISTRYINDEX, g->state);
    lua_pushstring(g->L, id);
    if (!call(g, 2, 1))
        return false;
    bool accepted = lua_toboolean(g->L, -1);
    lua_pop(g->L, 1);
    return sim_snapshot(g) && accepted;
}
bool sim_step(Sim *g, int ticks) {
    for (int i = 0; i < ticks; i++) {
        method(g, "step");
        lua_rawgeti(g->L, LUA_REGISTRYINDEX, g->state);
        if (!call(g, 1, 0))
            return false;
    }
    return sim_snapshot(g);
}
char *sim_text(Sim *g,const char *action) {
    method(g,action);
    if (!lua_isfunction(g->L,-1)) {lua_pop(g->L,1);return NULL;}
    lua_rawgeti(g->L,LUA_REGISTRYINDEX,g->state);
    if (!call(g,1,1)) return NULL;
    size_t size;
    const char *value=lua_tolstring(g->L,-1,&size);
    char *buf=value ? malloc(size+1) : NULL;
    if (buf) {memcpy(buf,value,size);buf[size]=0;}
    lua_pop(g->L,1);
    return buf;
}
bool sim_import(Sim *g,const char *action,const char *data) {
    bool restore=!strcmp(action,"restore");
    method(g,action);
    if (!restore) lua_rawgeti(g->L,LUA_REGISTRYINDEX,g->state);
    lua_pushstring(g->L,data);
    if (!call(g,restore ? 1 : 2,1)) return false;
    if (restore) {
        if (!lua_istable(g->L,-1)) { lua_pop(g->L,1);return fail(g,"Invalid saved mission"); }
        luaL_unref(g->L,LUA_REGISTRYINDEX,g->state);
        g->state=luaL_ref(g->L,LUA_REGISTRYINDEX);
    } else lua_pop(g->L,1);
    return sim_snapshot(g);
}
uint32_t sim_digest(Sim *g) {
    method(g, "record");
    if (!lua_isfunction(g->L, -1)) { lua_pop(g->L, 1); return 0; }
    lua_rawgeti(g->L, LUA_REGISTRYINDEX, g->state);
    if (!call(g, 1, 1)) return 0;
    size_t size;
    const char *record = lua_tolstring(g->L, -1, &size);
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < size; i++) hash = (hash ^ (unsigned char)record[i]) * 16777619u;
    lua_pop(g->L, 1);
    return hash;
}
void sim_close(Sim *g) {
    if (g->L)
        lua_close(g->L);
    free(g->orders); free(g->place); free(g->roster);
    g->orders = NULL; g->place = NULL; g->roster = NULL;
    g->L = NULL;
}
