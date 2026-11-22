#ifndef ORRA_LANGUAGE_H
#define ORRA_LANGUAGE_H
#include <stddef.h>
#include <stdint.h>
typedef struct {
    char key[80], english[500], value[500];
} Phrase;
typedef struct {
    unsigned code;
    unsigned char rows[7];
} Letter;
typedef struct {
    Phrase *entries;
    int count, capacity, selected, available;
    char root[1024], names[16][64], files[16][200], fonts[16][200], tags[16][36];
    Letter *extra;
    int letters;
} Language;
void lang_open(Language *, const char *);
void lang_add_list(Language *, const char *);
void lang_select(Language *, int);
void lang_close(Language *);
const char *lang_text(const Language *, const char *);
const char *lang_format(const Language *, const char *, char *, size_t);
const unsigned char *lang_glyph(const Language *, unsigned);
#endif
