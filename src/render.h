#ifndef ORRA_RENDER_H
#define ORRA_RENDER_H
#include "game.h"
#include <SDL3/SDL.h>
typedef struct {
    float x, y;
    uint8_t rgba[4];
} UiVertex;
typedef struct {
    char name[24];
    unsigned buffer, array;
    int count;
    float radius;
} Mesh;
typedef struct {
    unsigned surface,scene, sky, panel, empty, ui_buffer, ui_array;
    int vp, pose, eye, shadow, livery, inverse, sky_eye, orbit, night_light, extent, tracked,
        wrist_projection;
    Mesh *meshes;
    int mesh_count,mesh_capacity,loaded_places;
    char portrait[18][20];
    unsigned char glyph[128][7];
    UiVertex *ui;
    int ui_count, ui_cap;
    int width, height;
    char error[400];
} Renderer;
bool render_open(Renderer *, const Sim *);
void render_close(Renderer *);
void render_view(Renderer *, const Sim *, const Camera *, Mat, V3, int, int, int, int);
void render_rect(Renderer *, float, float, float, float, uint32_t);
void render_text(Renderer *, float, float, float, const char *, uint32_t);
void render_overlay(Renderer *, int, int);
void render_slate(Renderer *, int, int, const Mat *);
bool render_capture(Renderer *, const char *);
#endif
