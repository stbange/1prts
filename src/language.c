#include "language.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *asset(const Language *l, const char *relative) {
    if (!*relative || *relative == '/' || strchr(relative, ':') || strstr(relative, "..") ||
        strchr(relative, '\\'))
        return NULL;
    char path[1280];
    snprintf(path, sizeof path, "%s/%s", l->root, relative);
    return fopen(path, "rb");
}
static void trim(char *s) { s[strcspn(s, "\r\n")] = 0; }

static void phrases(Language *l, int index) {
    FILE *f = asset(l, l->files[index]);
    if (!f)
        return;
    char line[1024];
    while (fgets(line, sizeof line, f)) {
        trim(line);
        char *value = strchr(line, '|');
        if (!value)
            continue;
        *value++ = 0;
        if (index) {
            for (int i = 0; i < l->count; i++)
                if (!strcmp(l->entries[i].key, line)) {
                    SDL_strlcpy(l->entries[i].value, value, sizeof l->entries[i].value);
                    break;
                }
        } else {
            if (l->count == l->capacity) {
                int next = l->capacity + 64;
                Phrase *rows = realloc(l->entries, (size_t)next * sizeof *rows);
                if (!rows)
                    break;
                l->entries = rows;
                l->capacity = next;
            }
            Phrase *p = &l->entries[l->count++];
            SDL_strlcpy(p->key, line, sizeof p->key);
            SDL_strlcpy(p->english, value, sizeof p->english);
            p->value[0] = 0;
        }
    }
    fclose(f);
}
void lang_select(Language *l, int index) {
    if (index < 0 || index >= l->available)
        index = 0;
    l->count = 0;
    l->letters = 0;
    l->selected = index;
    if (!l->available)
        return;
    phrases(l, 0);
    if (index)
        phrases(l, index);
    FILE *f = asset(l, l->fonts[index]);
    if (!f)
        return;
    unsigned code;
    char bitmap[16];
    while (fscanf(f, "%x %15s", &code, bitmap) == 2 && strlen(bitmap) == 14) {
        Letter *rows = realloc(l->extra, (size_t)(l->letters + 1) * sizeof *rows);
        if (!rows)
            break;
        l->extra = rows;
        Letter *glyph = &rows[l->letters++];
        glyph->code = code;
        for (int y = 0; y < 7; y++) {
            unsigned bits = 0;
            sscanf(bitmap + 2 * y, "%2x", &bits);
            glyph->rows[y] = (unsigned char)bits;
        }
    }
    fclose(f);
}
static void add_language(Language *l, const char *row) {
    if (l->available >= 16)
        return;
    int i = l->available;
    int fields = sscanf(row, "%63[^|]|%199[^|]|%199[^|\r\n]|%35[a-zA-Z0-9-]", l->names[i],
                        l->files[i], l->fonts[i], l->tags[i]);
    if (fields >= 3) {
        if (fields == 3)
            SDL_strlcpy(l->tags[i], i ? "und" : "en", sizeof l->tags[i]);
        l->available++;
    }
}
void lang_add_list(Language *l, const char *rows) {
    while (*rows) {
        size_t len = strcspn(rows, "\n");
        char line[540];
        snprintf(line, sizeof line, "%.*s", (int)len, rows);
        add_language(l, line);
        rows += len;
        if (*rows)
            rows++;
    }
}
void lang_open(Language *l, const char *root) {
    SDL_strlcpy(l->root, root, sizeof l->root);
    FILE *f = asset(l, "assets/languages.txt");
    if (!f)
        return;
    char line[540];
    while (l->available < 16 && fgets(line, sizeof line, f)) {
        trim(line);
        add_language(l, line);
    }
    fclose(f);
    lang_select(l, 0);
}
void lang_close(Language *l) {
    free(l->entries);
    free(l->extra);
    memset(l, 0, sizeof *l);
}
const char *lang_text(const Language *l, const char *text) {
    if (!l->selected && !strchr(text, '.'))
        return text;
    for (int i = 0; i < l->count; i++) {
        const Phrase *p = &l->entries[i];
        if (!strcmp(text, p->key) || (l->selected && !strcmp(text, p->english)))
            return p->value[0] ? p->value : p->english;
    }
    return text;
}
const unsigned char *lang_glyph(const Language *l, unsigned code) {
    for (int i = 0; i < l->letters; i++)
        if (l->extra[i].code == code)
            return l->extra[i].rows;
    return NULL;
}

/* Translations can reorder {1}..{9}. The English template delimits each field. */
const char *lang_format(const Language *l, const char *source, char *out, size_t capacity) {
    const char *plain = lang_text(l, source);
    if (!l->selected || plain != source)
        return plain;
    for (int i = 0; i < l->count; i++) {
        const Phrase *entry = &l->entries[i];
        if (!*entry->value || !strstr(entry->english, "{1}"))
            continue;
        const char *pattern = entry->english, *input = source, *values[10] = {0};
        size_t lengths[10] = {0};
        bool matches = true;
        while (*pattern) {
            if (*pattern != '{' || pattern[1] < '1' || pattern[1] > '9' || pattern[2] != '}') {
                if (*input != *pattern) {
                    matches = false;
                    break;
                }
                input++;
                pattern++;
                continue;
            }
            int field = pattern[1] - '0';
            pattern += 3;
            size_t separator = strcspn(pattern, "{");
            const char *end = input + strlen(input);
            if (separator) {
                char sep[500];
                snprintf(sep, sizeof sep, "%.*s", (int)separator, pattern);
                end = strstr(input, sep);
                if (!end) {
                    matches = false;
                    break;
                }
            }
            values[field] = input;
            lengths[field] = (size_t)(end - input);
            input = end;
        }
        if (!matches || *input || !capacity)
            continue;
        size_t used = 0;
        for (const char *p = entry->value; *p && used + 1 < capacity; p++) {
            if (*p == '{' && p[1] >= '1' && p[1] <= '9' && p[2] == '}' && values[p[1] - '0']) {
                int field = p[1] - '0';
                size_t n = SDL_min(lengths[field], capacity - used - 1);
                memcpy(out + used, values[field], n);
                used += n;
                p += 2;
            } else
                out[used++] = *p;
        }
        out[used] = 0;
        return out;
    }
    return source;
}
