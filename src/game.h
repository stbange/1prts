#ifndef ORRA_GAME_H
#define ORRA_GAME_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <lua.h>

#define ORRA_PI 3.14159265f
#define MAX_SITE 64
#define MAX_CATALOG 64
#define CATEGORY_COUNT 5
#define ORDER_COUNT 10
typedef struct {
    float x, y, z;
} V3;
typedef struct {
    float v[16];
} Mat;
typedef struct {
    char id[32], name[40], role[128], owner[24];
} Entry;
typedef struct {
    Entry entry[MAX_CATALOG];
    int count;
} Catalog;
typedef struct {
    char id[24], label[44];
} Order;
typedef struct {
    char model[24], condition[24];
    V3 pos;
    float yaw;
} Place;
typedef struct {
    int alloy, loads, haul, draw, hold, walk, drive, rollout, ready, work_left, stock;
    bool harvesting, mill, watch, motor, section, armor, power, advance, guard;
    char work[24];
} Side;
typedef struct {
    lua_State *L;
    int rules, state;
    char root[1024], error[320], report[160], result[16];
    int tick, hold_ticks, relief_at;
    Side blue, red;
    Catalog cats[CATEGORY_COUNT];
    Order orders[ORDER_COUNT];
    Place place[MAX_SITE];
    int places;
} Sim;
typedef struct {
    V3 pos;
    float yaw, pitch;
    int aboard;
    V3 local;
} Camera;
typedef struct {
    float w, h, d;
    bool boardable;
} Shape;
enum { UNITS, BUILDINGS, FACTIONS, MISSIONS, UPGRADES };
bool sim_open(Sim *, const char *);
void sim_close(Sim *);
bool sim_reset(Sim *);
bool sim_step(Sim *, int);
bool sim_command(Sim *, int);
bool sim_snapshot(Sim *);
Mat mat_mul(Mat, Mat);
Mat mat_projection(float, float);
Mat mat_view(V3, float, float);
V3 to_world(V3, V3, float);
V3 to_local(V3, V3, float);
void camera_reset(Camera *);
void camera_look(Camera *, float, float);
void camera_move(Camera *, const Sim *, float, float, float);
V3 camera_eye(const Camera *, const Sim *);
bool world_object(const Sim *, int, V3 *);
Shape world_shape(const char *);
bool world_terminal(const Sim *, const Camera *);
int world_nearest(const Sim *, const Camera *);
bool world_interact(const Sim *, Camera *);
#endif
