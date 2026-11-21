#include "game.h"
#include <lauxlib.h>
#include <lualib.h>
#include <stdio.h>
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
bool sim_snapshot(Sim *g) {
    lua_State *L = g->L;
    lua_rawgeti(L, LUA_REGISTRYINDEX, g->state);
    g->tick = number(L, -1, "tick");
    string_field(L, -1, "report", g->report, sizeof g->report);
    string_field(L, -1, "result", g->result, sizeof g->result);
    read_side(L, "blue", &g->blue);
    read_side(L, "red", &g->red);
    lua_pop(L, 1);
    return true;
}
bool sim_reset(Sim *g) {
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
bool sim_open(Sim *g, const char *root) {
    memset(g, 0, sizeof *g);
    copy(g->root, sizeof g->root, root);
    g->state = LUA_NOREF;
    g->L = luaL_newstate();
    if (!g->L)
        return fail(g, "Unable to create Lua state");
    lua_State *L = g->L;
    luaL_openlibs(L);
    if (!load_table(g, "catalog") || !read_catalog(g) || !load_table(g, "site"))
        return false;
    g->places = (int)lua_rawlen(L, -1);
    if (g->places <= 0 || g->places > MAX_SITE)
        return fail(g, "Bad site size");
    for (int i = 0; i < g->places; i++) {
        Place *p = &g->place[i];
        lua_geti(L, -1, i + 1);
        if (!lua_istable(L, -1))
            return fail(g, "Invalid site entry");
        string_at(L, 1, p->model, sizeof p->model);
        p->pos = (V3){float_at(L, 2), float_at(L, 3), float_at(L, 4)};
        p->yaw = float_at(L, 5);
        string_at(L, 6, p->condition, sizeof p->condition);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    if (!load_table(g, "gate_six"))
        return false;
    g->hold_ticks = number(L, -1, "hold_ticks");
    lua_getfield(L, -1, "actions");
    if (lua_rawlen(L, -1) != ORDER_COUNT || g->hold_ticks <= 0)
        return fail(g, "Invalid mission rules");
    for (int i = 0; i < ORDER_COUNT; i++) {
        lua_geti(L, -1, i + 1);
        if (!lua_istable(L, -1))
            return fail(g, "Invalid mission order");
        string_at(L, 1, g->orders[i].id, sizeof g->orders[i].id);
        string_at(L, 2, g->orders[i].label, sizeof g->orders[i].label);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    g->rules = luaL_ref(L, LUA_REGISTRYINDEX);
    return sim_reset(g);
}
bool sim_command(Sim *g, int index) {
    if (index < 0 || index >= ORDER_COUNT)
        return false;
    method(g, "command");
    lua_rawgeti(g->L, LUA_REGISTRYINDEX, g->state);
    lua_pushstring(g->L, g->orders[index].id);
    if (!call(g, 2, 1))
        return false;
    bool accepted = lua_toboolean(g->L, -1);
    lua_pop(g->L, 1);
    sim_snapshot(g);
    return accepted;
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
void sim_close(Sim *g) {
    if (g->L)
        lua_close(g->L);
    g->L = NULL;
}
