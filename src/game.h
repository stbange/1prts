#ifndef ORRA_GAME_H
#define ORRA_GAME_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <lua.h>

#define ORRA_PI 3.14159265f
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
    char id[80], label[64], actor[64];
} Order;
typedef struct {
    char type[32], owner[24], model[24], condition[24], key[64], name[50], family[20], role[24], asset[180];
    V3 pos, size, current;
    float yaw, facing;
    int parent, linked, berth;
    float ramp;
    bool hull, station, moved;
} Place;
typedef struct {
    int alloy, loads, haul, draw, hold, walk, drive, rollout, ready, work_left, stock;
    bool harvesting, mill, watch, motor, section, armor, power, advance, guard;
    char work[24];
} Side;
typedef struct {
    lua_State *L;
    int rules, state, bundle;
    char root[1024], error[320], report[160], result[16];
    int tick, hold_ticks, relief_at;
    Side blue, red;
    Catalog cats[CATEGORY_COUNT];
    Order *orders;
    Place *place;
    int *roster, crew_count, capacity, site_ref;
    int places, order_count, operation_count, clock, progress, reserved, report_at;
    char sounds[8][180];
    char mission[32], title[48], faction[48], terrain[24], objective[104], stage[32], advice[24];
    V3 spawn;
    float spawn_yaw;
    Entry operations[MAX_CATALOG];
} Sim;
typedef struct {
    V3 pos;
    float yaw, pitch;
    int aboard;
    V3 local;
} Camera;
typedef struct { int place; bool ground; float distance; V3 point; } WorldHit;
bool world_ground(const Sim *, V3);
WorldHit world_point(const Sim *, V3, V3, int);
typedef struct {
    float w, h, d;
    bool boardable;
} Shape;
enum { UNITS, BUILDINGS, FACTIONS, MISSIONS, UPGRADES };
bool sim_open(Sim *, const char *);
bool sim_open_mission(Sim *, const char *, const char *);
bool sim_order(Sim *, const char *);
void sim_crew_report(const Sim *, const char *, char *, size_t);
void sim_inspect(Sim *, const char *, char *, size_t);
void sim_log(const Sim *, int, char *, size_t);
int sim_crew_status(const Sim *, const char *, int *);
float world_angle(const Sim *, int);
Shape world_asset_shape(const Sim *, int);
void sim_close(Sim *);
bool sim_reset(Sim *);
uint32_t sim_digest(Sim *);
char *sim_text(Sim *,const char *);
bool sim_import(Sim *,const char *,const char *);
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
