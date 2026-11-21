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
    const char *name=place->model;
    char path[1400];
    if(place->asset[0]) snprintf(path,sizeof path,"%s/%s",root,place->asset);
    else snprintf(path, sizeof path, "%s/assets/models/%s.mesh", root, name);
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
    Mesh *m = &r->meshes[r->mesh_count++];
    snprintf(m->name, sizeof m->name, "%s", name);
    m->count = (int)count;
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
    r->orbit = fGetUniformLocation(r->sky,"nightSky");
    r->sky_eye = fGetUniformLocation(r->sky, "eye");
    r->extent = fGetUniformLocation(r->panel, "extent");
    fGenVertexArrays(1, &r->empty);
    for (int i = 0; i < g->places; i++)
        if (!find(r, g->place[i].model) && !load_mesh(r, g->root, &g->place[i]))
            return false;
    if (!find(r, g->terrain))
        return problem(r, "site", "terrain model is required");
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
static bool visible(const float planes[6][4],V3 center,float radius) {
    for(int i=0;i<6;i++){
        const float *p=planes[i];
        if(p[0]*center.x+p[1]*center.y+p[2]*center.z+p[3] < -radius)return false;
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
    fDisable(GL_DEPTH_TEST);
    fUseProgram(r->sky);
    fBindVertexArray(r->empty);
    fUniformMatrix4fv(r->inverse, 1, GL_FALSE, vp.v);
    fUniform3f(r->sky_eye, eye.x, eye.y, eye.z);
    fUniform1i(r->orbit, !strcmp(g->terrain,"orbit") ? 2 : !strcmp(g->terrain,"airfield"));
    fDrawArrays(GL_TRIANGLES, 0, 3);
    fEnable(GL_DEPTH_TEST);
    /* A stereo compositor may use both eyes' depth after this frame. */
    fEnable(GL_SCISSOR_TEST);
    fScissor(x, y, w, h);
    fClear(GL_DEPTH_BUFFER_BIT);
    fDisable(GL_SCISSOR_TEST);
    fUseProgram(r->scene);
    fUniformMatrix4fv(r->vp, 1, GL_FALSE, vp.v);
    fUniform3f(r->eye, eye.x, eye.y, eye.z);
    fUniform1i(r->shadow, 0);
    fUniform1i(r->livery, 0);
    draw(r, find(r, g->terrain), (V3){0, 0, 0}, 0);
    float planes[6][4];
    for(int i=0;i<6;i++){
        float *plane=planes[i];
        for(int j=0;j<4;j++)plane[j]=vp.v[j*4+3]+(i%2?-1:1)*vp.v[j*4+i/2];
        float norm=sqrtf(plane[0]*plane[0]+plane[1]*plane[1]+plane[2]*plane[2]);
        if(norm>0)for(int j=0;j<4;j++)plane[j]/=norm;
    }
    for (int pass = 1; pass >= 0; pass--) {
        fUniform1i(r->shadow, pass);
        for (int i = 0; i < g->places; i++) {
            if (!strcmp(g->place[i].model, g->terrain))
                continue;
            V3 p;
            if (!world_object(g, i, &p))
                continue;
            Place const *o = &g->place[i];
            Shape bounds=world_asset_shape(g,i);
            float radius= fmaxf(4.f,bounds.d*.62f+bounds.w*1.85f+bounds.h+2);
            if(!visible(planes,p,radius+(pass?fabsf(p.y)*2:0)))continue;
            fUniform1i(r->livery, !strncmp(o->condition, "red", 3));
            Mesh *m = find(r, o->model);
            bool squad = strstr(o->model, "section") != NULL;
            bool sprite = squad || !strcmp(o->model, "worker") || !strcmp(o->family,"people");
            float a = sprite ? atan2f(p.x - eye.x, eye.z - p.z) : world_angle(g,i);
            if (!squad)
                draw(r, m, p, a);
            else
                for (int j = 0; j < 4; j++) {
                    V3 q = {p.x + (j % 2) * .95f, p.y, p.z + (j / 2) * 1.25f};
                    draw(r, m, q, a);
                }
        }
    }
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
    for (size_t i = 0; t[i]; i++) {
        unsigned ch = (unsigned)toupper((unsigned char)t[i]);
        if (ch >= 128)
            continue;
        for (int row = 0; row < 7; row++) {
            int bits = r->glyph[ch][row];
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
void render_overlay(Renderer *r, int w, int h) {
    fDisable(GL_DEPTH_TEST);
    fEnable(GL_BLEND);
    fBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    fUseProgram(r->panel);
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
    free(r->ui);
    memset(r, 0, sizeof *r);
}
