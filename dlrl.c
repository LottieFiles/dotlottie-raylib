#include "dlrl.h"
#include "dotlottie_player.h"  
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define DLRL_FALLBACK_SURFACE 512u
#define DLRL_MAX_MARKER_NAME 64

typedef struct {
    char name[DLRL_MAX_MARKER_NAME];
    float startFrame;
    float frameCount;
    float durationSeconds;
} dlrl_Marker;

struct dlrl_Player {
    struct DotLottiePlayer* core;
    Texture2D tex;
    int texW, texH;
    uint32_t requestedW;
    uint32_t requestedH;
    bool autoWidth;
    bool autoHeight;
    float time;
    float duration;      
    float assetDuration; 
    int totalFrames;
    float segmentStartFrame;
    float segmentFrameCount;
    bool playing;
    float speed;
    bool loop;
    dlrl_Mode mode;
    bool interpolate;
    dlrl_Fit fit;
    Vector2 align;
    Color bg;
    Vector2 natural; 
    dlrl_Marker* markers;
    int markerCount;
    int activeMarker;
    float direction;
};

static void fit_rect(int srcW, int srcH, Rectangle dst, dlrl_Fit fit, Vector2 align, Rectangle* out)
{
    float sx = dst.width  / (float)srcW;
    float sy = dst.height / (float)srcH;
    float s = 1.f;

    switch (fit) {
        case DLRL_FIT_FILL:        s = (sx + sy) * 0.5f; break;
        case DLRL_FIT_CONTAIN:     s = (sx < sy) ? sx : sy; break;
        case DLRL_FIT_COVER:       s = (sx > sy) ? sx : sy; break;
        case DLRL_FIT_FIT_WIDTH:   s = sx; break;
        case DLRL_FIT_FIT_HEIGHT:  s = sy; break;
    }

    float w = srcW * s;
    float h = srcH * s;
    float ox = dst.x + (dst.width  - w) * align.x;
    float oy = dst.y + (dst.height - h) * align.y;
    out->x = ox; out->y = oy; out->width = w; out->height = h;
}

static void copy_capped(char* dst, size_t cap, const char* src)
{
    if (!dst || cap == 0) return;
    size_t i = 0;
    if (src) {
        for (; src[i] && i < cap - 1; ++i) {
            dst[i] = src[i];
        }
    }
    if (i < cap) dst[i] = '\0';
}

static void set_dotlottie_string(struct DotLottieString* dst, const char* src)
{
    if (!dst) return;
    memset(dst->value, 0, sizeof(dst->value));
    if (!src) return;
    copy_capped(dst->value, sizeof(dst->value), src);
}

static float seconds_per_frame(const dlrl_Player* p)
{
    if (!p || p->assetDuration <= 0.f) return 0.f;
    int frames = (p->totalFrames > 1) ? (p->totalFrames - 1) : p->totalFrames;
    if (frames <= 0) return 0.f;
    return p->assetDuration / (float)frames;
}

static void apply_segment(dlrl_Player* p, float startFrame, float frameCount, float durationSeconds)
{
    if (!p) return;
    p->segmentStartFrame = (startFrame >= 0.f) ? startFrame : 0.f;
    float frames = (frameCount > 1.f) ? frameCount : 1.f;
    p->segmentFrameCount = frames;
    if (durationSeconds > 0.f) {
        p->duration = durationSeconds;
    } else {
        float spf = seconds_per_frame(p);
        float span = (frames > 1.f) ? (frames - 1.f) : frames;
        p->duration = (spf > 0.f) ? spf * span : p->assetDuration;
    }
    if (p->duration <= 0.f) p->duration = p->assetDuration > 0.f ? p->assetDuration : 1.f;
    p->time = 0.f;
}

static bool equals_ignore_case(const char* a, const char* b)
{
    if (!a || !b) return false;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return false;
        }
        ++a; ++b;
    }
    return (*a == '\0' && *b == '\0');
}

static void load_markers(dlrl_Player* p)
{
    if (!p) return;
    size_t needed = 0;
    if (dotlottie_markers(p->core, NULL, &needed) != DOTLOTTIE_SUCCESS || needed == 0) {
        return;
    }
    struct DotLottieMarker* raw = MemAlloc(sizeof(struct DotLottieMarker) * needed);
    if (!raw) return;
    size_t count = needed;
    if (dotlottie_markers(p->core, raw, &count) != DOTLOTTIE_SUCCESS || count == 0) {
        MemFree(raw);
        return;
    }
    dlrl_Marker* markers = MemAlloc(sizeof(dlrl_Marker) * count);
    if (!markers) {
        MemFree(raw);
        return;
    }
    float spf = seconds_per_frame(p);
    for (size_t i = 0; i < count; ++i) {
        dlrl_Marker* m = &markers[i];
        copy_capped(m->name, sizeof(m->name), raw[i].name.value);
        m->startFrame = raw[i].time;
        m->frameCount = (raw[i].duration > 0.f) ? raw[i].duration : 1.f;
        float span = (m->frameCount > 1.f) ? (m->frameCount - 1.f) : m->frameCount;
        m->durationSeconds = (spf > 0.f) ? spf * span : 0.f;
    }
    
    for (size_t i = 0; i < count; ++i) {
        size_t minIndex = i;
        for (size_t j = i + 1; j < count; ++j) {
            if (markers[j].startFrame < markers[minIndex].startFrame) {
                minIndex = j;
            }
        }
        if (minIndex != i) {
            dlrl_Marker tmp = markers[i];
            markers[i] = markers[minIndex];
            markers[minIndex] = tmp;
        }
    }
    p->markers = markers;
    p->markerCount = (int)count;
    p->activeMarker = -1;
    MemFree(raw);
}

bool dlrl_Init(void)   { return true; }
void dlrl_Shutdown(void){}

static struct DotLottiePlayer* make_core(const dlrl_Config* cfg)
{
    struct DotLottieConfig c;
    dotlottie_init_config(&c);

    c.mode = (cfg && cfg->mode == DLRL_MODE_REVERSE) ? Reverse :
        (cfg && cfg->mode == DLRL_MODE_BOUNCE) ? Bounce :
        (cfg && cfg->mode == DLRL_MODE_REVERSE_BOUNCE) ? ReverseBounce : Forward;

    c.loop_animation = cfg ? cfg->loop : true;
    c.speed = cfg ? (cfg->speed != 0.f ? cfg->speed : 1.f) : 1.f;
    c.use_frame_interpolation = cfg ? cfg->interpolate : true;
    c.background_color = (cfg ? ColorToInt(cfg->background) : 0);

    if (cfg && cfg->animation_id) {
        set_dotlottie_string(&c.animation_id, cfg->animation_id);
    }
    if (cfg && cfg->theme_id) {
        set_dotlottie_string(&c.theme_id, cfg->theme_id);
    }
    if (cfg && cfg->state_machine_id) {
        set_dotlottie_string(&c.state_machine_id, cfg->state_machine_id);
    }

    struct DotLottiePlayer* core = dotlottie_new_player(&c);
    return core;
}

static dlrl_Player* alloc_player(void)
{
    dlrl_Player* p = (dlrl_Player*)MemAlloc(sizeof(dlrl_Player));
    if (p) *p = (dlrl_Player){0};
    return p;
}

static bool create_texture(dlrl_Player* p)
{
    if (!p) return false;
    Image img = {
        .data = NULL,
        .width = p->texW,
        .height = p->texH,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };
    p->tex = LoadTextureFromImage(img); 
    return (p->tex.id != 0);
}

static bool recreate_texture(dlrl_Player* p, uint32_t w, uint32_t h)
{
    if (!p) return false;
    if ((int)w == p->texW && (int)h == p->texH && p->tex.id != 0) {
        return true;
    }
    if (p->tex.id) {
        UnloadTexture(p->tex);
        p->tex = (Texture2D){0};
    }
    p->texW = (int)w;
    p->texH = (int)h;
    return create_texture(p);
}

static bool ends_with_ci(const char* str, const char* suffix)
{
    if (!str || !suffix) return false;
    size_t len = strlen(str);
    size_t suflen = strlen(suffix);
    if (suflen == 0 || suflen > len) return false;
    const char* cmp = str + (len - suflen);
    for (size_t i = 0; i < suflen; ++i) {
        if (tolower((unsigned char)cmp[i]) != tolower((unsigned char)suffix[i])) {
            return false;
        }
    }
    return true;
}

static bool read_binary_file(const char* path, unsigned char** outData, size_t* outSize)
{
    if (!path || !outData || !outSize) return false;
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return false; }
    long len = ftell(f);
    if (len < 0) { fclose(f); return false; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return false; }
    unsigned char* data = (unsigned char*)MemAlloc((size_t)len);
    if (!data) { fclose(f); return false; }
    size_t read = fread(data, 1, (size_t)len, f);
    fclose(f);
    if (read != (size_t)len) {
        MemFree(data);
        return false;
    }
    *outData = data;
    *outSize = (size_t)len;
    return true;
}

dlrl_Player* dlrl_LoadDotLottieFile(const char* path, const dlrl_Config* cfg)
{
    if (!path) return NULL;
    struct DotLottiePlayer* core = make_core(cfg);
    if (!core) return NULL;

    uint32_t requestedW = (uint32_t)(cfg && cfg->width  > 0 ? cfg->width  : 0);
    uint32_t requestedH = (uint32_t)(cfg && cfg->height > 0 ? cfg->height : 0);
    uint32_t loadW = requestedW ? requestedW : DLRL_FALLBACK_SURFACE;
    uint32_t loadH = requestedH ? requestedH : DLRL_FALLBACK_SURFACE;

    bool isDotLottie = ends_with_ci(path, ".lottie");
    int status = DOTLOTTIE_ERROR;
    unsigned char* blob = NULL;
    size_t blobSize = 0;
    if (isDotLottie) {
        if (read_binary_file(path, &blob, &blobSize)) {
            status = dotlottie_load_dotlottie_data(core, (const char*)blob, blobSize, loadW, loadH);
            MemFree(blob);
        }
    } else {
        status = dotlottie_load_animation_path(core, path, loadW, loadH);
    }

    if (status != DOTLOTTIE_SUCCESS) {
        dotlottie_destroy(core);
        return NULL;
    }

    float naturalW = 0.f, naturalH = 0.f, duration = 0.f, totalFrames = 0.f;
    dotlottie_animation_size(core, &naturalW, &naturalH);
    dotlottie_duration(core, &duration);
    dotlottie_total_frames(core, &totalFrames);

    uint32_t targetW = requestedW ? requestedW : (naturalW > 0.f ? (uint32_t)naturalW : loadW);
    uint32_t targetH = requestedH ? requestedH : (naturalH > 0.f ? (uint32_t)naturalH : loadH);

    if ((targetW != loadW || targetH != loadH) &&
            dotlottie_resize(core, targetW, targetH) != DOTLOTTIE_SUCCESS) {
        dotlottie_destroy(core);
        return NULL;
    }

    dlrl_Player* p = alloc_player();
    if (!p) { dotlottie_destroy(core); return NULL; }
    p->core = core;
    p->texW = (int)targetW; p->texH = (int)targetH;
    p->requestedW = requestedW;
    p->requestedH = requestedH;
    p->autoWidth = (requestedW == 0);
    p->autoHeight = (requestedH == 0);
    p->assetDuration = duration;
    p->duration = duration;
    p->totalFrames = totalFrames > 0.f ? (int)totalFrames : 1;
    p->segmentStartFrame = 0.f;
    p->segmentFrameCount = (float)p->totalFrames;
    p->speed = cfg ? (cfg->speed ? cfg->speed : 1.f) : 1.f;
    p->loop = cfg ? cfg->loop : true;
    p->mode = cfg ? cfg->mode : DLRL_MODE_FORWARD;
    p->interpolate = cfg ? cfg->interpolate : true;
    p->fit = cfg ? cfg->fit : DLRL_FIT_CONTAIN;
    p->align = cfg ? cfg->align : (Vector2){0.5f, 0.5f};
    p->bg = cfg ? cfg->background : BLANK;
    p->direction = (p->mode == DLRL_MODE_REVERSE || p->mode == DLRL_MODE_REVERSE_BOUNCE) ? -1.f : 1.f;
    if (p->speed < 0.f) { p->direction = -p->direction; p->speed = -p->speed; }

    p->natural = (Vector2){naturalW, naturalH};
    p->markers = NULL;
    p->markerCount = 0;
    p->activeMarker = -1;

    if (!create_texture(p)) { dlrl_Unload(p); return NULL; }

    load_markers(p);
    if (cfg && cfg->marker && cfg->marker[0]) {
        dlrl_SetMarker(p, cfg->marker);
    }

    return p;
}

dlrl_Player* dlrl_LoadLottieJSON(const char* json, size_t len, const dlrl_Config* cfg)
{
    if (!json || len == 0) return NULL;
    struct DotLottiePlayer* core = make_core(cfg);
    if (!core) return NULL;

    uint32_t requestedW = (uint32_t)(cfg && cfg->width  > 0 ? cfg->width  : 0);
    uint32_t requestedH = (uint32_t)(cfg && cfg->height > 0 ? cfg->height : 0);
    uint32_t loadW = requestedW ? requestedW : DLRL_FALLBACK_SURFACE;
    uint32_t loadH = requestedH ? requestedH : DLRL_FALLBACK_SURFACE;
    char* buf = (char*)MemAlloc(len + 1);
    if (!buf) {
        dotlottie_destroy(core);
        return NULL;
    }
    memcpy(buf, json, len);
    buf[len] = '\0';

    int status = dotlottie_load_animation_data(core, buf, loadW, loadH);
    MemFree(buf);
    if (status != DOTLOTTIE_SUCCESS) {
        dotlottie_destroy(core);
        return NULL;
    }

    float naturalW = 0.f, naturalH = 0.f, duration = 0.f, totalFrames = 0.f;
    dotlottie_animation_size(core, &naturalW, &naturalH);
    dotlottie_duration(core, &duration);
    dotlottie_total_frames(core, &totalFrames);

    uint32_t targetW = requestedW ? requestedW : (naturalW > 0.f ? (uint32_t)naturalW : loadW);
    uint32_t targetH = requestedH ? requestedH : (naturalH > 0.f ? (uint32_t)naturalH : loadH);

    if ((targetW != loadW || targetH != loadH) &&
            dotlottie_resize(core, targetW, targetH) != DOTLOTTIE_SUCCESS) {
        dotlottie_destroy(core);
        return NULL;
    }

    dlrl_Player* p = alloc_player();
    if (!p) { dotlottie_destroy(core); return NULL; }
    p->core = core;
    p->texW = (int)targetW; p->texH = (int)targetH;
    p->requestedW = requestedW;
    p->requestedH = requestedH;
    p->autoWidth = (requestedW == 0);
    p->autoHeight = (requestedH == 0);
    p->assetDuration = duration;
    p->duration = duration;
    p->totalFrames = totalFrames > 0.f ? (int)totalFrames : 1;
    p->segmentStartFrame = 0.f;
    p->segmentFrameCount = (float)p->totalFrames;
    p->speed = cfg ? (cfg->speed ? cfg->speed : 1.f) : 1.f;
    p->loop = cfg ? cfg->loop : true;
    p->mode = cfg ? cfg->mode : DLRL_MODE_FORWARD;
    p->interpolate = cfg ? cfg->interpolate : true;
    p->fit = cfg ? cfg->fit : DLRL_FIT_CONTAIN;
    p->align = cfg ? cfg->align : (Vector2){0.5f, 0.5f};
    p->bg = cfg ? cfg->background : BLANK;
    p->direction = (p->mode == DLRL_MODE_REVERSE || p->mode == DLRL_MODE_REVERSE_BOUNCE) ? -1.f : 1.f;
    if (p->speed < 0.f) { p->direction = -p->direction; p->speed = -p->speed; }

    p->natural = (Vector2){naturalW, naturalH};
    p->markers = NULL;
    p->markerCount = 0;
    p->activeMarker = -1;

    if (!create_texture(p)) { dlrl_Unload(p); return NULL; }

    load_markers(p);
    if (cfg && cfg->marker && cfg->marker[0]) {
        dlrl_SetMarker(p, cfg->marker);
    }

    return p;
}

void dlrl_Unload(dlrl_Player* p)
{
    if (!p) return;
    if (p->tex.id) UnloadTexture(p->tex);
    if (p->core) dotlottie_destroy(p->core);
    if (p->markers) MemFree(p->markers);
    MemFree(p);
}

void dlrl_Play(dlrl_Player* p){ if(!p) return; dotlottie_play(p->core); p->playing = true; }
void dlrl_Pause(dlrl_Player* p){ if(!p) return; dotlottie_pause(p->core); p->playing = false; }
void dlrl_Stop(dlrl_Player* p){
    if(!p) return;
    dotlottie_stop(p->core);
    p->playing=false;
    p->time=0.f;
    p->direction = (p->mode == DLRL_MODE_REVERSE || p->mode == DLRL_MODE_REVERSE_BOUNCE) ? -1.f : 1.f;
    dotlottie_set_frame(p->core, 0.f);
}
bool dlrl_IsPlaying(const dlrl_Player* p){ return p && p->playing; }

void dlrl_SetSpeed(dlrl_Player* p, float speed){
    if (!p) return;
    if (speed < 0.f) {
        p->direction = -1.f;
        p->speed = -speed;
    } else {
        p->speed = speed;
        if (p->direction == 0.f) p->direction = 1.f;
    }
}
void dlrl_SetLoop(dlrl_Player* p, bool loop){ if(p) p->loop = loop; }
void dlrl_SetMode(dlrl_Player* p, dlrl_Mode m){
    if(!p) return;
    p->mode = m;
    p->direction = (m == DLRL_MODE_REVERSE || m == DLRL_MODE_REVERSE_BOUNCE) ? -1.f : 1.f;
    if (p->direction == 0.f) p->direction = 1.f;
}

static float wrap_time(dlrl_Player* p, float t)
{
    if (t < 0.f || t > p->duration) {
        if (!p->loop) return (t < 0.f) ? 0.f : p->duration;
        while (t < 0.f) t += p->duration;
        while (t > p->duration) t -= p->duration;
    }
    return t;
}

void dlrl_Update(dlrl_Player* p, float dt)
{
    if (!p) return;
    if (p->playing) {
        float dir = (p->direction == 0.f) ? 1.f : p->direction;
        switch (p->mode) {
            case DLRL_MODE_FORWARD:
                dir = 1.f;
                break;
            case DLRL_MODE_REVERSE:
                dir = -1.f;
                break;
            case DLRL_MODE_BOUNCE:
            case DLRL_MODE_REVERSE_BOUNCE:
                if ((p->time <= 0.f && dir < 0.f) || (p->time >= p->duration && dir > 0.f)) {
                    dir = -dir;
                }
                break;
        }
        p->direction = (dir == 0.f) ? 1.f : dir;
        float s = p->speed * p->direction;
        p->time = wrap_time(p, p->time + dt * s);
    }

    float normalized = (p->duration > 0.f) ? (p->time / p->duration) : 0.f;
    if (normalized < 0.f) normalized = 0.f;
    if (normalized > 1.f) normalized = 1.f;
    float segmentSpan = (p->segmentFrameCount > 1.f) ? (p->segmentFrameCount - 1.f) : 0.f;
    float targetFrame = p->segmentStartFrame + normalized * segmentSpan;
    dotlottie_set_frame(p->core, targetFrame);
    dotlottie_render(p->core);

    const uint32_t* ptr = NULL;
    dotlottie_buffer_ptr(p->core, &ptr);
    if (ptr && p->tex.id) {
        UpdateTexture(p->tex, (const void*)ptr);
    }
}

void dlrl_Draw(const dlrl_Player* p, Rectangle dest, float rotation, Color tint)
{
    if (!p || p->tex.id == 0) return;

    if (p->bg.a > 0) {
        DrawRectangleRec(dest, p->bg);
    }

    Rectangle src = { 0, 0, (float)p->texW, (float)p->texH };
    Rectangle fit = {0};
    fit_rect(p->texW, p->texH, dest, p->fit, p->align, &fit);
    Vector2 origin = { fit.width * 0.5f, fit.height * 0.5f };
    Vector2 center = { fit.x + origin.x, fit.y + origin.y };

    DrawTexturePro(p->tex, src,
            (Rectangle){ center.x, center.y, fit.width, fit.height },
            origin, rotation, tint);
}

float dlrl_Duration(const dlrl_Player* p){ return p ? p->duration : 0.f; }
float dlrl_CurrentTime(const dlrl_Player* p){ return p ? p->time : 0.f; }
int   dlrl_TotalFrames(const dlrl_Player* p){ float f=0; return (p && dotlottie_total_frames(p->core, &f)==0) ? (int)f : 0; }
int   dlrl_CurrentFrame(const dlrl_Player* p){ float f=0; return (p && dotlottie_current_frame(p->core, &f)==0) ? (int)f : 0; }
Vector2 dlrl_NaturalSize(const dlrl_Player* p){ return p ? p->natural : (Vector2){0,0}; }
Texture2D dlrl_GetTexture(const dlrl_Player* p){ return p ? p->tex : (Texture2D){0}; }

bool dlrl_SetTheme(dlrl_Player* p, const char* theme_id)
{
    if (!p) return false;
    if (!theme_id || !theme_id[0]) return (dotlottie_reset_theme(p->core) == DOTLOTTIE_SUCCESS);
    return (dotlottie_set_theme(p->core, theme_id) == DOTLOTTIE_SUCCESS);
}

bool dlrl_SetAnimation(dlrl_Player* p, const char* animation_id)
{
    if (!p || !animation_id) return false;
    uint32_t loadW = p->requestedW ? p->requestedW : DLRL_FALLBACK_SURFACE;
    uint32_t loadH = p->requestedH ? p->requestedH : DLRL_FALLBACK_SURFACE;
    if (dotlottie_load_animation(p->core, animation_id, loadW, loadH) != DOTLOTTIE_SUCCESS) {
        return false;
    }
    float naturalW = 0.f, naturalH = 0.f, duration = 0.f, totalFrames = 0.f;
    dotlottie_animation_size(p->core, &naturalW, &naturalH);
    dotlottie_duration(p->core, &duration);
    dotlottie_total_frames(p->core, &totalFrames);

    uint32_t targetW = p->requestedW ? p->requestedW : (naturalW > 0.f ? (uint32_t)naturalW : loadW);
    uint32_t targetH = p->requestedH ? p->requestedH : (naturalH > 0.f ? (uint32_t)naturalH : loadH);

    if ((targetW != loadW || targetH != loadH) &&
            dotlottie_resize(p->core, targetW, targetH) != DOTLOTTIE_SUCCESS) {
        return false;
    }
    if (!recreate_texture(p, targetW, targetH)) {
        return false;
    }

    p->assetDuration = duration;
    p->duration = duration;
    p->totalFrames = totalFrames > 0.f ? (int)totalFrames : 1;
    p->segmentStartFrame = 0.f;
    p->segmentFrameCount = (float)p->totalFrames;
    p->time = 0.f;
    p->natural = (Vector2){naturalW, naturalH};
    if (p->markers) {
        MemFree(p->markers);
        p->markers = NULL;
    }
    p->markerCount = 0;
    p->activeMarker = -1;
    load_markers(p);
    return true;
}

bool dlrl_SetMarker(dlrl_Player* p, const char* marker_name)
{
    if (!p) return false;
    if (!marker_name || !marker_name[0]) {
        apply_segment(p, 0.f, (float)p->totalFrames, p->assetDuration);
        p->activeMarker = -1;
        return true;
    }
    if (!p->markers || p->markerCount == 0) return false;
    for (int i = 0; i < p->markerCount; ++i) {
        if (equals_ignore_case(p->markers[i].name, marker_name)) {
            apply_segment(p, p->markers[i].startFrame, p->markers[i].frameCount,
                          p->markers[i].durationSeconds);
            p->activeMarker = i;
            return true;
        }
    }
    return false;
}

int dlrl_MarkerCount(const dlrl_Player* p)
{
    return p ? p->markerCount : 0;
}

const char* dlrl_MarkerName(const dlrl_Player* p, int index)
{
    if (!p || index < 0 || index >= p->markerCount || !p->markers) return NULL;
    return p->markers[index].name;
}
