#ifndef ORRA_SESSION_H
#define ORRA_SESSION_H
#include <SDL3/SDL.h>

typedef enum {
    DESK_CLOSED,
    DESK_TITLE,
    DESK_PAUSE,
    DESK_CONFIRM,
    DESK_SAVES,
    DESK_LOAD,
    DESK_SETTINGS,
    DESK_BINDINGS,
    DESK_BRIEF,
    DESK_RECORD,
    DESK_CREDITS,
    DESK_DOCUMENT,
    DESK_HELP,
    DESK_LOAD_ERROR,
    DESK_LOADING
} DeskPage;
typedef enum {
    DO_NONE,
    DO_MISSION,
    DO_RESTART,
    DO_DESK,
    DO_EXIT,
    DO_SAVE,
    DO_LOAD,
    DO_DELETE
} DeskAction;
typedef enum {
    PREF_WINDOW,
    PREF_RESOLUTION,
    PREF_VSYNC,
    PREF_CAP,
    PREF_QUALITY,
    PREF_BRIGHT,
    PREF_MASTER,
    PREF_MUSIC,
    PREF_FX,
    PREF_RADIO,
    PREF_MUTE,
    PREF_SCALE,
    PREF_CAPTION,
    PREF_BACKDROP,
    PREF_SPEED,
    PREF_COMFORT,
    PREF_TURN,
    PREF_LOOK,
    PREF_INVERT,
    PREF_TOGGLE,
    PREF_HAPTIC,
    PREF_NARRATE,
    PREF_LANGUAGE,
    PREF_COUNT
} PrefId;
#define XR_BINDINGS 12
typedef struct {
    int xr[XR_BINDINGS];
    int value[PREF_COUNT];
    SDL_Scancode binding[SDL_SCANCODE_COUNT];
} Preferences;
typedef struct {
    bool enabled, playing, needs_checkpoint, rebinding, guided, practice_guide, practice_dirty,
        dirty;
    DeskPage page, cancel_page;
    DeskAction pending;
    int row, slot, recent, write_slot, write_tick, queued_slot, queued_tick, pending_slot,
        document_line, practice_tick, saved_tick;
    char target[64], pending_name[160], slot_title[3][80], detail[512], diagnostic[400];
    bool occupied[3];
    char *practice, *document, *queued_save;
    Preferences current, edit;
} Session;
#define ORRA_SLOT_COUNT 3
#endif
