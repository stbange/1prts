#include "app.h"
#if defined(_WIN32)
#include <SDL3/SDL_main.h>
#endif
int main(int argc, char **argv) {
    return orra_run(argc, argv);
}
