#include "render.h"
#include <SDL3/SDL_opengl.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* SDL resolves these core-profile symbols on all native platforms and WebGL 2. */
#define GL_CALLS(X) \
    X(GLuint, CreateShader, (GLenum)) \
    X(void, ShaderSource, (GLuint, GLsizei, const GLchar *const *, const GLint *)) \
    X(void, CompileShader, (GLuint)) \
    X(void, GetShaderiv, (GLuint, GLenum, GLint *)) \
    X(void, GetShaderInfoLog, (GLuint, GLsizei, GLsizei *, GLchar *)) \
    X(void, DeleteShader, (GLuint)) \
    X(GLuint, CreateProgram, (void)) \
    X(void, AttachShader, (GLuint, GLuint)) \
    X(void, LinkProgram, (GLuint)) \
    X(void, GetProgramiv, (GLuint, GLenum, GLint *)) \
    X(void, GetProgramInfoLog, (GLuint, GLsizei, GLsizei *, GLchar *)) \
    X(void, DeleteProgram, (GLuint)) \
    X(void, UseProgram, (GLuint)) \
    X(GLint, GetUniformLocation, (GLuint, const GLchar *)) \
    X(void, UniformMatrix4fv, (GLint, GLsizei, GLboolean, const GLfloat *)) \
    X(void, Uniform4f, (GLint, GLfloat, GLfloat, GLfloat, GLfloat)) \
    X(void, Uniform1i, (GLint, GLint)) \
    X(void, Uniform3f, (GLint, GLfloat, GLfloat, GLfloat)) \
    X(void, Uniform2f, (GLint, GLfloat, GLfloat)) \
    X(void, GenVertexArrays, (GLsizei, GLuint *)) \
    X(void, BindVertexArray, (GLuint)) \
    X(void, DeleteVertexArrays, (GLsizei, const GLuint *)) \
    X(void, GenTextures, (GLsizei, GLuint *)) \
    X(void, BindTexture, (GLenum, GLuint)) \
    X(void, DeleteTextures, (GLsizei, const GLuint *)) \
    X(void, TexParameteri, (GLenum, GLenum, GLint)) \
    X(void, TexImage2D, (GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void *)) \
    X(void, GenBuffers, (GLsizei, GLuint *)) \
    X(void, BindBuffer, (GLenum, GLuint)) \
    X(void, BufferData, (GLenum, GLsizeiptr, const void *, GLenum)) \
    X(void, DeleteBuffers, (GLsizei, const GLuint *)) \
    X(void, EnableVertexAttribArray, (GLuint)) \
    X(void, VertexAttribPointer, (GLuint, GLint, GLenum, GLboolean, GLsizei, const void *)) \
    X(void, DrawArrays, (GLenum, GLint, GLsizei)) \
    X(void, Enable, (GLenum)) \
    X(void, Disable, (GLenum)) \
    X(void, BlendFunc, (GLenum, GLenum)) \
    X(void, Viewport, (GLint, GLint, GLsizei, GLsizei)) \
    X(void, Scissor, (GLint, GLint, GLsizei, GLsizei)) \
    X(void, Clear, (GLbitfield)) \
    X(void, PixelStorei, (GLenum, GLint)) \
    X(void, ReadPixels, (GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void *))
#define DECL(ret, name, args) static ret(APIENTRY *f##name) args;
GL_CALLS(DECL)

static bool problem(Renderer *r, const char *where, const char *why) {
    snprintf(r->error, sizeof r->error, "%.180s: %.210s", where, why);
    return false;
}
static unsigned shader(Renderer *r, const char *root, const char *name, const char *stage,
                       GLenum type) {
    char path[1400];
    snprintf(path, sizeof path, "%s/assets/shaders/%s.%s.glsl", root, name, stage);
    char *body = SDL_LoadFile(path, NULL);
    if (!body) {
        problem(r, path, "missing shader");
        return 0;
    }
#ifdef __EMSCRIPTEN__
    const char *version = "#version 300 es\nprecision highp float;\nprecision highp int;\n";
#else
    const char *version = "#version 330 core\n";
#endif
    const char *parts[] = {version, body};
    GLuint id = fCreateShader(type);
    fShaderSource(id, 2, parts, NULL);
    fCompileShader(id);
    SDL_free(body);
    GLint ok = 0;
    fGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char info[256] = {0};
        fGetShaderInfoLog(id, sizeof info, NULL, info);
        problem(r, path, info);
        fDeleteShader(id);
        return 0;
    }
    return id;
}
static unsigned program(Renderer *r, const char *root, const char *name) {
    GLuint v = shader(r, root, name, "vert", GL_VERTEX_SHADER);
    if (!v)
        return 0;
    GLuint f = shader(r, root, name, "frag", GL_FRAGMENT_SHADER);
    if (!f) {
        fDeleteShader(v);
        return 0;
    }
    GLuint p = fCreateProgram();
    fAttachShader(p, v);
    fAttachShader(p, f);
    fLinkProgram(p);
    fDeleteShader(v);
    fDeleteShader(f);
    GLint ok = 0;
    fGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char info[256] = {0};
        fGetProgramInfoLog(p, sizeof info, NULL, info);
        problem(r, name, info);
        fDeleteProgram(p);
        return 0;
    }
    return p;
}
static void attributes(unsigned *vao, unsigned *buffer) {
    fGenVertexArrays(1, vao);
    fBindVertexArray(*vao);
    fGenBuffers(1, buffer);
    fBindBuffer(GL_ARRAY_BUFFER, *buffer);
}
static Mesh *find(Renderer *r, const char *id) {
    for (int i = 0; i < r->mesh_count; i++)
        if (!strcmp(r->meshes[i].name, id))
            return &r->meshes[i];
    return NULL;
}
static bool load_mesh(Renderer *r, const char *root, const Place *place) {
    const char *name = place->model;
    char path[1400];
    if (place->asset[0])
        snprintf(path, sizeof path, "%s/%s", root, place->asset);
    else
        snprintf(path, sizeof path, "%s/assets/models/%s.mesh", root, name);
    size_t size = 0;
    unsigned char *bytes = SDL_LoadFile(path, &size);
    if (!bytes)
        return problem(r, path, "missing mesh");
    uint32_t count = 0;
    if (size >= 8)
        memcpy(&count, bytes + 4, 4);
    count = SDL_Swap32LE(count);
    if (size < 8 || memcmp(bytes, "ORRA", 4) || !count || count > 1000000 ||
        size != 8 + (size_t)count * 28) {
        SDL_free(bytes);
        return problem(r, path, "invalid mesh header or length");
    }
    if (r->mesh_count>=r->mesh_capacity) {
        int capacity=r->mesh_capacity ? 2*r->mesh_capacity : 16;
        Mesh *space=realloc(r->meshes,(size_t)capacity*sizeof *space);
        if (!space) {SDL_free(bytes);return problem(r,path,"mesh allocation failed");}
        r->meshes=space;r->mesh_capacity=capacity;
    }
    Mesh *m = &r->meshes[r->mesh_count++];
    memset(m,0,sizeof *m);
    snprintf(m->name, sizeof m->name, "%s", name);
    m->count = (int)count;
    float squared = 0;
    for (uint32_t i = 0; i < count; i++) {
        float xyz[3];
        memcpy(xyz, bytes + 8 + (size_t)i * 28, sizeof xyz);
        squared = fmaxf(squared, xyz[0] * xyz[0] + xyz[1] * xyz[1] + xyz[2] * xyz[2]);
    }
    m->radius = sqrtf(squared);
    attributes(&m->array, &m->buffer);
    fBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(size - 8), bytes + 8, GL_STATIC_DRAW);
    SDL_free(bytes);
    fEnableVertexAttribArray(0);
    fVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 28, NULL);
    fEnableVertexAttribArray(1);
    fVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 28, (void *)12);
    fEnableVertexAttribArray(2);
    fVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 28, (void *)24);
    return true;
}
static bool map_mesh(Renderer *r, const Sim *g, int i) {
    Mesh *mesh = find(r, g->place[i].model);
    if (!mesh) {
        if (!load_mesh(r, g->root, &g->place[i]))
            return false;
        mesh = &r->meshes[r->mesh_count - 1];
    }
    r->places[i].mesh = (int)(mesh - r->meshes);
    return true;
}
static bool scene_geometry(Renderer *r, const Sim *g) {
    if (g->places > r->place_capacity) {
        RenderPlace *rows = realloc(r->places, (size_t)g->capacity * sizeof *rows);
        if (!rows)
            return problem(r, "scene", "cannot allocate visible geometry");
        r->places = rows;
        r->place_capacity = g->capacity;
    }
    for (int i = r->loaded_places; i < g->places; i++) {
        r->places[i].mesh = -1;
        if (strncmp(g->place[i].model, "work-", 5) && !map_mesh(r, g, i))
            return false;
    }
    r->loaded_places = g->places;
    return true;
}
bool render_open(Renderer *r, const Sim *g) {
    memset(r, 0, sizeof *r);
#define RESOLVE(ret, name, args)                                                                       \
    do {                                                                                               \
        SDL_FunctionPointer fp = SDL_GL_GetProcAddress("gl" #name);                                    \
        if (!fp) {                                                                                     \
            return problem(r, "gl" #name, "unavailable");                                              \
        }                                                                                              \
        memcpy(&f##name, &fp, sizeof fp);                                                              \
    } while (0);
    GL_CALLS(RESOLVE)
    char path[1300];
    snprintf(path, sizeof path, "%s/assets/font.txt", g->root);
    FILE *file = fopen(path, "r");
    if (!file)
        return problem(r, path, "missing font");
    lang_open(&r->lang, g->root);
    lang_add_list(&r->lang, sim_locales(g));
    char ch, hex[24];
    while (fscanf(file, " %c %23s", &ch, hex) == 2) {
        if (strlen(hex) != 14 || (unsigned char)ch >= 128) {
            fclose(file);
            return problem(r, path, "bad glyph");
        }
        for (int i = 0; i < 7; i++) {
            char pair[] = {hex[i * 2], hex[i * 2 + 1], 0};
            r->glyph[(int)ch][i] = (unsigned char)strtol(pair, NULL, 16);
        }
    }
    fclose(file);
    snprintf(path, sizeof path, "%s/assets/radio.txt", g->root);
    file = fopen(path, "r");
    if (!file)
        return problem(r, path, "missing radio portrait");
    for (int row = 0; row < 18; row++) {
        if (fscanf(file, "%19s", r->portrait[row]) != 1) {
            fclose(file);
            return problem(r, path, "short portrait");
        }
    }
    fclose(file);
    if (!(r->scene = program(r, g->root, "world")) || !(r->sky = program(r, g->root, "sky")) ||
        !(r->panel = program(r, g->root, "panel")))
        return false;
    r->vp = fGetUniformLocation(r->scene, "vp");
    r->pose = fGetUniformLocation(r->scene, "objectPose");
    r->livery = fGetUniformLocation(r->scene, "compactLivery");
    r->shadow = fGetUniformLocation(r->scene, "shadowPass");
    r->eye = fGetUniformLocation(r->scene, "eye");
    r->inverse = fGetUniformLocation(r->sky, "sceneVP");
    r->exposure = fGetUniformLocation(r->scene, "exposure");
    r->sky_exposure = fGetUniformLocation(r->sky, "exposure");
    r->night_light = fGetUniformLocation(r->scene, "nightLight");
    r->orbit = fGetUniformLocation(r->sky, "nightSky");
    r->sky_eye = fGetUniformLocation(r->sky, "eye");
    r->extent = fGetUniformLocation(r->panel, "extent");
    r->tracked = fGetUniformLocation(r->panel, "tracked");
    r->wrist_projection = fGetUniformLocation(r->panel, "wristProjection");
    fGenVertexArrays(1, &r->empty);
    if (!scene_geometry(r, g))
        return false;
    if (!find(r, g->terrain))
        return problem(r, "site", "terrain model is required");
    r->loaded_places=g->places;
    if (find(r,"worker")) {
        const char *poses[]={"operator-side","operator-work"};
        for (int n=0;n<2;n++) {
            Place sprite={0};
            snprintf(sprite.model,sizeof sprite.model,"%s",poses[n]);
            if (!load_mesh(r,g->root,&sprite)) return false;
        }
        r->side_pose = (int)(find(r, "operator-side") - r->meshes);
        r->work_pose = (int)(find(r, "operator-work") - r->meshes);
    }
    char tile_path[1350];
    snprintf(tile_path,sizeof tile_path,"%s/assets/surface.txt",g->root);
    char *tile=SDL_LoadFile(tile_path,NULL);
    if (!tile) return problem(r,tile_path,"missing surface tile");
    unsigned char pixels[256];
    int count=0;
    for (const char *q=tile;*q && count<256;q++) if (*q>='0' && *q<='9') pixels[count++]=(unsigned char)((*q-'0')*28);
    SDL_free(tile);
    if (count!=256) return problem(r,tile_path,"expected a 16 by 16 surface tile");
    fGenTextures(1,&r->surface);
    fBindTexture(GL_TEXTURE_2D,r->surface);
    fTexImage2D(GL_TEXTURE_2D,0,GL_R8,16,16,0,GL_RED,GL_UNSIGNED_BYTE,pixels);
    fTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    fTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    attributes(&r->ui_array, &r->ui_buffer);
    fEnableVertexAttribArray(0);
    fVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(UiVertex), NULL);
    fEnableVertexAttribArray(1);
    fVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(UiVertex), (void *)8);
    return true;
}
static void draw(Renderer *r, Mesh *m, V3 p, float angle) {
    fUniform4f(r->pose, p.x, p.y, p.z, angle);
    fBindVertexArray(m->array);
    fDrawArrays(GL_TRIANGLES, 0, m->count);
}
/* Test world-space bounds against the six homogeneous clip planes, including projected shadows. */
static bool visible(float planes[6][4], V3 center, float radius) {
    for (int i = 0; i < 6; i++) {
        const float *p = planes[i];
        if (p[0] * center.x + p[1] * center.y + p[2] * center.z + p[3] < -radius)
            return false;
    }
    return true;
}
void render_view(Renderer *r, const Sim *g, const Camera *c, Mat vp, V3 eye, int x, int y, int w,
                 int h) {
    (void)c;
    r->width = w;
    r->height = h;
    fViewport(x, y, w, h);
    fDisable(GL_BLEND);
    fEnable(GL_DEPTH_TEST);
    if (!scene_geometry(r, g))
        return;
    for (int i = 0; i < g->places; i++) {
        RenderPlace *part = &r->places[i];
        const Place *row = &g->place[i];
        part->visible = strcmp(row->model, g->terrain) && world_object(g, i, &part->position);
        if (!part->visible)
            continue;
        const V3 p = part->position;
        float dx = p.x - eye.x, dy = p.y - eye.y, dz = p.z - eye.z;
        float distance = dx * dx + dy * dy + dz * dz;
        if (part->mesh < 0 && distance < 120 * 120 && !map_mesh(r, g, i))
            return;
        if (part->mesh < 0 || (distance > 85 * 85 &&
                               (!strcmp(row->family, "fixture") || !strcmp(row->family, "operator"))))
            part->visible = false;
    }
    /* A stereo compositor may use both eyes' depth after this frame. */
    fEnable(GL_SCISSOR_TEST);
    fScissor(x, y, w, h);
    fClear(GL_DEPTH_BUFFER_BIT);
    fDisable(GL_SCISSOR_TEST);
    fUseProgram(r->scene);
    fBindTexture(GL_TEXTURE_2D,r->surface);
    fUniformMatrix4fv(r->vp, 1, GL_FALSE, vp.v);
    fUniform3f(r->eye, eye.x, eye.y, eye.z);
    fUniform1i(r->exposure, (int)r->brightness);
    fUniform1i(r->night_light, !strcmp(g->terrain, "orbit") ? 2 : !strcmp(g->terrain, "airfield"));
    fUniform1i(r->shadow, 0);
    fUniform1i(r->livery, 0);
    draw(r, find(r, g->terrain), (V3){0, 0, 0}, 0);
    float planes[6][4];
    for (int i = 0; i < 6; i++) {
        float *plane = planes[i];
        for (int j = 0; j < 4; j++)
            plane[j] = vp.v[j * 4 + 3] + (i % 2 ? -1 : 1) * vp.v[j * 4 + i / 2];
        float norm = sqrtf(plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2]);
        if (norm > 0)
            for (int j = 0; j < 4; j++)
                plane[j] /= norm;
    }
    for (int pass = r->quality ? 0 : 1; pass >= 0; pass--) {
        fUniform1i(r->shadow, pass);
        for (int i = 0; i < g->places; i++) {
            const RenderPlace *part = &r->places[i];
            if (!part->visible)
                continue;
            const Place *o = &g->place[i];
            V3 p = part->position;
            Mesh *m = &r->meshes[part->mesh];
            float radius = m->radius + 2.3f;
            V3 center = pass ? (V3){p.x + .62f * p.y, .073f, p.z - .32f * p.y} : p;
            if (!visible(planes, center, radius * (pass ? 1.28f : 1.f)))
                continue;
            fUniform1i(r->livery, !strcmp(o->owner,"rival"));
            bool squad = strstr(o->model, "section") != NULL;
            bool sprite = squad || !strcmp(o->model, "worker") || !strcmp(o->family, "people") ||
                          !strcmp(o->family, "sprite");
            float a = sprite ? atan2f(p.x - eye.x, eye.z - p.z) : world_angle(g, i);
            if (!strcmp(o->family, "operator") && r->side_pose) {
                float relative=world_angle(g,i)-a;
                if (fabsf(sinf(relative)) > .68f)
                    m = &r->meshes[r->side_pose];
                else if (((g->tick / 38 + i) % 3) == 1)
                    m = &r->meshes[r->work_pose];
            }
            if (!squad)
                draw(r, m, p, a);
            else
                for (int j = 0; j < 4; j++) {
                    V3 q = {p.x + (j % 2) * .95f, p.y, p.z + (j / 2) * 1.25f};
                    draw(r, m, q, a);
                }
        }
    }
    /* The depth buffer rejects sky fragments hidden by the terrain or an occupied compartment. */
    fUseProgram(r->sky);
    fBindVertexArray(r->empty);
    fUniformMatrix4fv(r->inverse, 1, GL_FALSE, vp.v);
    fUniform3f(r->sky_eye, eye.x, eye.y, eye.z);
    fUniform1i(r->sky_exposure, (int)r->brightness);
    fUniform1i(r->orbit, !strcmp(g->terrain, "orbit") ? 2 : !strcmp(g->terrain, "airfield"));
    fDrawArrays(GL_TRIANGLES, 0, 3);
}
void render_rect(Renderer *r, float x, float y, float w, float h, uint32_t col) {
    if (r->ui_count + 6 > r->ui_cap) {
        int cap = r->ui_cap ? r->ui_cap * 2 : 16384;
        void *v = realloc(r->ui, (size_t)cap * sizeof(UiVertex));
        if (!v)
            return;
        r->ui = v;
        r->ui_cap = cap;
    }
    const float p[6][2] = {{x, y}, {x + w, y}, {x + w, y + h}, {x, y}, {x + w, y + h}, {x, y + h}};
    for (int j = 0; j < 6; j++) {
        UiVertex *v = &r->ui[r->ui_count++];
        v->x = p[j][0];
        v->y = p[j][1];
        for (int k = 0; k < 4; k++)
            v->rgba[k] = (uint8_t)(col >> (24 - k * 8));
    }
}
void render_text(Renderer *r, float x, float y, float s, const char *t, uint32_t col) {
    char translated[800];
    t = lang_format(&r->lang, t, translated, sizeof translated);
    for (size_t i = 0; *t; i++) {
        Uint32 ch = SDL_StepUTF8(&t, NULL);
        const unsigned char *glyph =
            ch < 128 ? r->glyph[toupper((unsigned char)ch)] : lang_glyph(&r->lang, ch);
        if (!glyph)
            glyph = r->glyph[(int)'?'];
        for (int row = 0; row < 7; row++) {
            int bits = glyph[row];
            for (int colx = 0; colx < 5;) {
                if (!(bits & (16 >> colx))) {
                    colx++;
                    continue;
                }
                int start = colx++;
                while (colx < 5 && (bits & (16 >> colx)))
                    colx++;
                render_rect(r, x + (i * 6 + start) * s, y + row * s, (colx - start) * s, s, col);
            }
        }
    }
}
void render_overlay(Renderer *r, int w, int h) { render_slate(r, w, h, NULL); }
void render_slate(Renderer *r, int w, int h, const Mat *wrist) {
    if (wrist)
        fEnable(GL_DEPTH_TEST);
    else
        fDisable(GL_DEPTH_TEST);
    fEnable(GL_BLEND);
    fBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    fUseProgram(r->panel);
    fUniform1i(r->tracked, wrist != NULL);
    if (wrist)
        fUniformMatrix4fv(r->wrist_projection, 1, GL_FALSE, wrist->v);
    fUniform2f(r->extent, (float)w, (float)h);
    fBindVertexArray(r->ui_array);
    fBindBuffer(GL_ARRAY_BUFFER, r->ui_buffer);
    fBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)((size_t)r->ui_count * sizeof(UiVertex)), r->ui,
                GL_STREAM_DRAW);
    fDrawArrays(GL_TRIANGLES, 0, r->ui_count);
    r->ui_count = 0;
}
bool render_capture(Renderer *r, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f)
        return false;
    size_t row = (size_t)r->width * 4;
    unsigned char *pixels = malloc(row * r->height);
    if (!pixels) {
        fclose(f);
        return false;
    }
    fPixelStorei(GL_PACK_ALIGNMENT, 1);
    fReadPixels(0, 0, r->width, r->height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    fprintf(f, "P6\n%d %d\n255\n", r->width, r->height);
    for (int y = r->height - 1; y >= 0; y--)
        for (int x = 0; x < r->width; x++)
            fwrite(pixels + y * row + x * 4, 1, 3, f);
    free(pixels);
    return fclose(f) == 0;
}
void render_close(Renderer *r) {
    lang_close(&r->lang);
    for (int i = 0; i < r->mesh_count; i++) {
        fDeleteBuffers(1, &r->meshes[i].buffer);
        fDeleteVertexArrays(1, &r->meshes[i].array);
    }
    if (r->ui_array) {
        fDeleteBuffers(1, &r->ui_buffer);
        fDeleteVertexArrays(1, &r->ui_array);
    }
    if (r->empty)
        fDeleteVertexArrays(1, &r->empty);
    if (r->scene)
        fDeleteProgram(r->scene);
    if (r->sky)
        fDeleteProgram(r->sky);
    if (r->panel)
        fDeleteProgram(r->panel);
    if (r->surface) fDeleteTextures(1,&r->surface);
    free(r->places);
    free(r->meshes);
    free(r->ui);
    memset(r, 0, sizeof *r);
}
