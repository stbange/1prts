#ifndef ORRA_APP_H
#define ORRA_APP_H
#include "render.h"
typedef enum { FIELD, SLATE, GUIDE, OPERATIONS, CREWS } Panel;
typedef struct {
    Sim sim;
    Camera camera;
    Renderer renderer;
    SDL_Window *window;
    SDL_GLContext context;
    SDL_AudioStream *audio;
    uint8_t *tone[8];
    uint32_t tone_size[8];
    Panel panel;
    int selected, category, item, width, height;
    bool running, touch, captured, xr, keys[SDL_SCANCODE_COUNT];
    bool left_down, right_down;
    SDL_FingerID left_id, right_id;
    float left_x, left_y, stick_x, stick_y, stride_x, stride_y;
    int listing, roster[MAX_SITE], crew_count;
    double accumulator;
    char message[120];
    int message_ticks;
    unsigned audio_cues;
    int voice_at;
    bool manual_clock;
    void (*observe_order)(int tick, int index, bool accepted);
} App;
bool app_open(App *, const char *, bool);
bool app_mission(App *,const char *);
void app_choose(App *);
int app_list_count(const App *);
void app_close(App *);
void app_key(App *, SDL_Scancode);
void app_event(App *, const SDL_Event *);
void app_tick(App *);
void app_frame(App *, double);
void app_advance(App *, double);
void orra_clock(int);
void app_draw(App *);
void app_action(App *, int);
void app_touch(App *, float, float);
void ui_size(const App *, int *, int *);
void ui_draw(App *, int, int);
void ui_relief_notice(Renderer *, const Sim *, bool);
void ui_tap(App *, float, float);
bool app_route(App *, const char *);
int orra_run(int, char **);
void xr_attach(App *);
const char *orra_status(void);
void orra_xr_mode(int);
void orra_xr_tick(double, float, float, float);
void orra_xr_eye(const float *, const float *, int, int, int, int, float, float, float);
void orra_xr_action(int);
#endif
