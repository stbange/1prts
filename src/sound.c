#include "app.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

void sound_open(App *a) {
    const char *names[] = {"shift-motif", "yard-air", "engine-room"};
    for (int i = 0; i < 3; i++) {
        char path[1300];
        SDL_AudioSpec spec;
        snprintf(path, sizeof path, "%s/assets/sounds/%s.wav", a->sim.root, names[i]);
        if (!SDL_LoadWAV(path, &spec, &a->bed[i], &a->bed_size[i]))
            continue;
        a->mix[i] = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
        if (a->mix[i])
            SDL_ResumeAudioStreamDevice(a->mix[i]);
    }
}
void sound_tick(App *a) {
    bool interior = a->camera.aboard >= 0;
    if (!interior) {
        for (int i = 0; i < a->sim.places; i++) {
            const Place *p = &a->sim.place[i];
            if (!p->station || p->hull)
                continue;
            V3 local = to_local(a->camera.pos, p->pos, p->yaw);
            if (fabsf(local.x) < p->size.x * .5f && fabsf(local.z) < p->size.z * .5f) {
                interior = true;
                break;
            }
        }
    }
    bool radio =
        a->voice && SDL_GetAudioStreamQueued(a->voice) > 0 && session_gain(a, PREF_RADIO) > 0;
    for (int i = 0; i < 3; i++) {
        if (!a->mix[i])
            continue;
        float gain = i == 0 ? .19f : i == (interior ? 2 : 1) ? .32f : .015f;
        gain *= session_gain(a, i == 0 ? PREF_MUSIC : PREF_FX);
        SDL_SetAudioStreamGain(a->mix[i], radio ? gain * .32f : gain);
        if (SDL_GetAudioStreamQueued(a->mix[i]) < (int)a->bed_size[i] / 16)
            SDL_PutAudioStreamData(a->mix[i], a->bed[i], (int)a->bed_size[i]);
    }
}
void sound_close(App *a) {
    for (int i = 0; i < 3; i++) {
        if (a->mix[i])
            SDL_DestroyAudioStream(a->mix[i]);
        SDL_free(a->bed[i]);
    }
}

void sound_levels(App *a) {
    if (a->audio)
        SDL_SetAudioStreamGain(a->audio, session_gain(a, PREF_FX));
    if (a->voice)
        SDL_SetAudioStreamGain(a->voice, session_gain(a, PREF_RADIO));
    sound_tick(a);
}
void sound_caption(App *a, const char *speaker, const char *key) {
    SDL_strlcpy(a->speaker, speaker, sizeof a->speaker);
    SDL_strlcpy(a->caption, lang_text(&a->renderer.lang, key), sizeof a->caption);
    a->caption_at = a->sim.tick;
    char *row = a->calls[a->call_count++ % 12];
    snprintf(row, 260, "%02d:%02d / %s / %s", a->sim.tick / 1800, (a->sim.tick / 30) % 60, speaker,
             a->caption);
}
