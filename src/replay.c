#include "app.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Replays drive the keyboard and mouse entry points. Capture reads the player's framebuffer. */
static void key_event(App *a, SDL_Scancode sc, bool down) {
    SDL_Event e = {0};
    e.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    e.key.scancode = sc;
    app_event(a, &e);
}
static bool expect(const App *a, const char *key, const char *value) {
    if(!strcmp(key,"mission"))return !strcmp(a->sim.mission,value);
    if(!strcmp(key,"stage"))return !strcmp(a->sim.stage,value);
    if(!strcmp(key,"vessel"))return a->camera.aboard>=0 && !strcmp(a->sim.place[a->camera.aboard].key,value);
    if(!strcmp(key,"tick"))return a->sim.tick==atoi(value);
    if(!strcmp(key,"hold-target"))return a->sim.hold_ticks==atoi(value);
    if (!strcmp(key, "result"))
        return !strcmp(a->sim.result, value);
    if (!strcmp(key, "panel"))
        return !strcmp(a->panel == SLATE ? "orders" : a->panel == GUIDE ? "guide" : "field", value);
    if (!strcmp(key, "height"))
        return fabsf(a->camera.pos.y - (float)atof(value)) < .02f;
    if (!strcmp(key, "aboard"))
        return (a->camera.aboard >= 0) == (atoi(value) != 0);
    if (!strcmp(key, "armor"))
        return a->sim.blue.armor == (atoi(value) != 0);
    if (!strcmp(key, "section"))
        return a->sim.blue.section == (atoi(value) != 0);
    if (!strcmp(key, "alloy"))
        return a->sim.blue.alloy == atoi(value);
    return false;
}
bool app_route(App *a, const char *path) {
    FILE *file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, "Cannot open %s\n", path);
        return false;
    }
    char line[384];
    int n = 0, rate=30;
    bool ok = true;
    while (ok && fgets(line, sizeof line, file)) {
        char cmd[20] = {0}, arg[120] = {0}, extra[120] = {0};
        n++;
        int count = sscanf(line, "%19s %119s %119s", cmd, arg, extra);
        if (count < 1 || cmd[0] == '#')
            continue;
        if (!strcmp(cmd, "key") || !strcmp(cmd, "hold")) {
            SDL_Scancode sc = SDL_GetScancodeFromName(arg);
            ok = sc != SDL_SCANCODE_UNKNOWN;
            key_event(a, sc, true);
            if (!strcmp(cmd, "hold"))
                for (int i = 0; i < atoi(extra); i++)
                    app_tick(a);
            key_event(a, sc, false);
        } else if (!strcmp(cmd,"rate")) {
            rate=atoi(arg);ok=rate>=15 && rate<=240;
        } else if (!strcmp(cmd,"frames")) {
            for(int i=0;i<atoi(arg);i++)app_frame(a,1.0/rate);
        } else if (!strcmp(cmd, "wait")) {
            for (int i = 0; i < atoi(arg); i++)
                app_tick(a);
        } else if (!strcmp(cmd, "look")) {
            SDL_Event click = {0};
            click.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
            app_event(a, &click);
            SDL_Event e = {0};
            e.type = SDL_EVENT_MOUSE_MOTION;
            e.motion.xrel = (float)atoi(arg);
            e.motion.yrel = (float)atoi(extra);
            app_event(a, &e);
        } else if (!strcmp(cmd, "expect"))
            ok = expect(a, arg, extra);
        else if (!strcmp(cmd, "shot")) {
            app_draw(a);
            const char *dir = SDL_getenv("ORRA_CAPTURE_DIR");
            if (dir && *dir) {
                char out[1400];
                snprintf(out, sizeof out, "%s/%s.ppm", dir, arg);
                ok = render_capture(&a->renderer, out);
            }
            SDL_GL_SwapWindow(a->window);
        } else
            ok = false;
    }
    fclose(file);
    if(!ok){
        const char *dir=SDL_getenv("ORRA_FAILURE_DIR");
        if(dir && *dir){
            char filename[1300];app_draw(a);
            snprintf(filename,sizeof filename,"%s/%s-failure.ppm",dir,a->sim.mission);
            render_capture(&a->renderer,filename);
            snprintf(filename,sizeof filename,"%s/%s-state.txt",dir,a->sim.mission);
            FILE *state=fopen(filename,"w");
            if(state){fprintf(state,"%s line %d\n%s\n%s\n",path,n,orra_status(),a->sim.report);fclose(state);}
        }
    }
    fprintf(stderr, "%s %s at line %d, tick %d, %s, alloy %d, body %.1f %.1f\n", path,
            ok ? "passed" : "FAILED", n, a->sim.tick, a->sim.result, a->sim.blue.alloy, a->camera.pos.x,
            a->camera.pos.z);
    return ok;
}
