#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif
/* Uses the same Lua VM and authoritative mission entry points as the SDL host. */
static bool replay(Sim *g, FILE *in) {
    char line[320], verb[24], value[140];
    unsigned lineno = 0;
    while (fgets(line, sizeof line, in)) {
        lineno++;
        int n = sscanf(line, "%23s %139s", verb, value);
        if (n < 1 || verb[0] == '#')
            continue;
        bool ok = false;
        if (n == 2 && !strcmp(verb, "order"))
            ok = sim_order(g, value);
        else if (n == 2 && !strcmp(verb, "reject"))
            ok = !sim_order(g, value);
        else if (n == 2 && !strcmp(verb, "wait")) {
            char *end;
            long ticks = strtol(value, &end, 10);
            ok = !*end && ticks >= 0 && ticks <= 108000 && sim_step(g, (int)ticks);
        } else if (n == 2 && !strcmp(verb, "checkpoint")) {
            printf("state %s %u\n", value, sim_digest(g));
            ok = true;
        } else if (n == 2 && !strcmp(verb, "result"))
            ok = !strcmp(value, g->result);
        else if (n == 2 && !strcmp(verb, "alloy"))
            ok = atoi(value) == g->blue.alloy;
        if (!ok) {
            fprintf(stderr, "Line %u at tick %d: %s / %s\n", lineno, g->tick, line, g->report);
            return false;
        }
    }
    return !ferror(in);
}
int main(int argc, char **argv) {
    char base[1024];
    snprintf(base, sizeof base, "%s", argv[0]);
#ifdef _WIN32
    GetModuleFileNameA(NULL, base, sizeof base - 1);
#elif defined(__APPLE__)
    uint32_t capacity = sizeof base;
    if (_NSGetExecutablePath(base, &capacity))
        snprintf(base, sizeof base, "%s", argv[0]);
#else
    ssize_t length = readlink("/proc/self/exe", base, sizeof base - 1);
    if (length >= 0)
        base[length] = 0;
#endif
    char *last = NULL;
    for (char *c = base; *c; c++)
        if (*c == '/' || *c == '\\')
            last = c;
    if (last)
        *last = 0;
    else
        snprintf(base, sizeof base, ".");
    const char *root = base, *mission = "gate-six", *route = NULL;
    bool list = false;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--help")) {
            puts("orra-sim [--data directory] [--list] [--mission id] [--replay file|-]");
            return 0;
        }
        if (!strcmp(argv[i], "--list"))
            list = true;
        else if (i + 1 < argc && !strcmp(argv[i], "--data"))
            root = argv[++i];
        else if (i + 1 < argc && !strcmp(argv[i], "--mission"))
            mission = argv[++i];
        else if (i + 1 < argc && !strcmp(argv[i], "--replay"))
            route = argv[++i];
        else {
            fprintf(stderr,
                    "Usage: orra-sim [--data dir] [--list] [--mission id] [--replay file|-]\n");
            return 2;
        }
    }
    Sim g;
    if (!sim_open_mission(&g, root, mission)) {
        fprintf(stderr, "%s\n", g.error);
        sim_close(&g);
        return 1;
    }
    if (list)
        for (int i = 0; i < g.operation_count; i++)
            printf("%s\t%s\n", g.operations[i].id, g.operations[i].name);
    bool ok = true;
    if (route) {
        FILE *f = !strcmp(route, "-") ? stdin : fopen(route, "r");
        ok = f && replay(&g, f);
        if (f && f != stdin)
            fclose(f);
    }
    printf("%s tick=%d result=%s alloy=%d progress=%d hold=%d\n", g.mission, g.tick, g.result,
           g.blue.alloy, g.progress, g.hold_ticks);
    sim_close(&g);
    return ok ? 0 : 1;
}
