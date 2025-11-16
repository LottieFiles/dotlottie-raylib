#ifndef __DLRL_H__
#define __DLRL_H__

/**
 * @file dlrl.h
 * @brief dotLottie-to-raylib bridge.
 *
 * Load .lottie archives or raw JSON, step them each frame, and draw into Raylib textures.
 * Link with dotlottie_player + raylib; see README for platform linker flags.
 */

#include "raylib.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque animation player instance. */
typedef struct dlrl_Player dlrl_Player;

/** @brief How the animation fits the destination rectangle. */
typedef enum {
    DLRL_FIT_CONTAIN,
    DLRL_FIT_COVER,
    DLRL_FIT_FILL,
    DLRL_FIT_FIT_WIDTH,
    DLRL_FIT_FIT_HEIGHT
} dlrl_Fit;

/** @brief Playback direction and wrap style. */
typedef enum {
    DLRL_MODE_FORWARD,
    DLRL_MODE_REVERSE,
    DLRL_MODE_BOUNCE,
    DLRL_MODE_REVERSE_BOUNCE
} dlrl_Mode;

/** @brief Optional settings when creating a player. */
typedef struct {
    int         width;              /**< Output width in pixels; 0 uses the asset's width. */
    int         height;             /**< Output height in pixels; 0 uses the asset's height. */
    float       speed;              /**< Playback speed multiplier (1.0 is normal). */
    bool        loop;               /**< Restart when reaching the end if true. */
    dlrl_Mode   mode;               /**< Direction/looping style. */
    bool        interpolate;        /**< Enable frame interpolation if supported by the runtime. */
    dlrl_Fit    fit;                /**< How to scale content into the destination rectangle. */
    Vector2     align;              /**< Normalized anchor inside dest rect (0..1 for X/Y). */
    Color       background;         /**< Clear color behind the animation quad. */

    const char* animation_id;       /**< Optional animation id from the bundle; NULL for default. */
    const char* theme_id;           /**< Optional theme id from the bundle; NULL for default. */
    const char* state_machine_id;   /**< Optional state machine id; NULL to ignore. */
    const char* marker;             /**< Optional marker label to start from; NULL plays whole clip. */
} dlrl_Config;

/**
 * @brief Initialize global state.
 * @return true on success.
 */
bool dlrl_Init(void);

/**
 * @brief Tear down global state; reserved for future cleanup.
 */
void dlrl_Shutdown(void);

/**
 * @brief Load a .lottie file from disk and create a player.
 * @param path Filesystem path to a .lottie archive.
 * @param cfg Optional config; pass NULL for defaults.
 * @return New player instance, or NULL on failure.
 */
dlrl_Player* dlrl_LoadDotLottieFile(const char* path, const dlrl_Config* cfg);

/**
 * @brief Load Lottie JSON from memory and create a player.
 * @param json Pointer to the JSON buffer.
 * @param len Length of the JSON buffer in bytes.
 * @param cfg Optional config; pass NULL for defaults.
 * @return New player instance, or NULL on failure.
 */
dlrl_Player* dlrl_LoadLottieJSON(const char* json, size_t len, const dlrl_Config* cfg);

/**
 * @brief Destroy a player and free resources.
 * @param p Player instance; safe to pass NULL.
 */
void dlrl_Unload(dlrl_Player* p);

/**
 * @brief Begin playback from the current time.
 * @param p Player instance.
 */
void dlrl_Play(dlrl_Player* p);

/**
 * @brief Pause playback without resetting the time cursor.
 * @param p Player instance.
 */
void dlrl_Pause(dlrl_Player* p);

/**
 * @brief Stop playback and reset to the start of the current segment.
 * @param p Player instance.
 */
void dlrl_Stop(dlrl_Player* p);

/**
 * @brief Query whether the player is actively playing.
 * @param p Player instance.
 * @return true if playing, false otherwise.
 */
bool dlrl_IsPlaying(const dlrl_Player* p);

/**
 * @brief Adjust playback speed multiplier.
 * @param p Player instance.
 * @param speed Speed multiplier (1.0 is normal).
 */
void dlrl_SetSpeed(dlrl_Player* p, float speed);

/**
 * @brief Enable or disable looping at the end of the current segment.
 * @param p Player instance.
 * @param loop True to loop, false to stop at the end.
 */
void dlrl_SetLoop(dlrl_Player* p, bool loop);

/**
 * @brief Change playback direction/wrap mode.
 * @param p Player instance.
 * @param mode New playback mode.
 */
void dlrl_SetMode(dlrl_Player* p, dlrl_Mode mode);

/**
 * @brief Advance the animation by the given delta time.
 * @param p Player instance.
 * @param dt_seconds Delta time in seconds; call once per frame.
 */
void dlrl_Update(dlrl_Player* p, float dt_seconds);

/**
 * @brief Draw the current frame into a destination rectangle.
 * @param p Player instance.
 * @param dest Destination rectangle in screen space.
 * @param rotation Rotation angle in degrees.
 * @param tint Tint color applied to the quad.
 */
void dlrl_Draw(const dlrl_Player* p, Rectangle dest, float rotation, Color tint);

/**
 * @brief Total duration in seconds of the active animation/segment.
 * @param p Player instance.
 * @return Duration in seconds.
 */
float dlrl_Duration(const dlrl_Player* p);

/**
 * @brief Current playback time in seconds.
 * @param p Player instance.
 * @return Time in seconds.
 */
float dlrl_CurrentTime(const dlrl_Player* p);

/**
 * @brief Total frame count of the active animation/segment.
 * @param p Player instance.
 * @return Number of frames.
 */
int dlrl_TotalFrames(const dlrl_Player* p);

/**
 * @brief Current frame index (0-based).
 * @param p Player instance.
 * @return Frame index.
 */
int dlrl_CurrentFrame(const dlrl_Player* p);

/**
 * @brief Natural width/height of the active animation in pixels.
 * @param p Player instance.
 * @return Vector of (width, height).
 */
Vector2 dlrl_NaturalSize(const dlrl_Player* p);

/**
 * @brief Texture backing the currently rendered frame.
 * @param p Player instance.
 * @return Live texture; valid while the player lives.
 */
Texture2D dlrl_GetTexture(const dlrl_Player* p);

/**
 * @brief Switch the active theme.
 * @param p Player instance.
 * @param theme_id Theme identifier; NULL for default.
 * @return true on success.
 */
bool dlrl_SetTheme(dlrl_Player* p, const char* theme_id);

/**
 * @brief Switch the active animation.
 * @param p Player instance.
 * @param animation_id Animation identifier; NULL for default.
 * @return true on success.
 */
bool dlrl_SetAnimation(dlrl_Player* p, const char* animation_id);

/**
 * @brief Jump to a marker or clear marker playback.
 * @param p Player instance.
 * @param marker_name Marker label; pass NULL to play the full timeline.
 * @return true on success.
 */
bool dlrl_SetMarker(dlrl_Player* p, const char* marker_name);

/**
 * @brief Number of markers embedded in the active animation.
 * @param p Player instance.
 * @return Marker count.
 */
int dlrl_MarkerCount(const dlrl_Player* p);

/**
 * @brief Marker name at the given index.
 * @param p Player instance.
 * @param index 0-based marker index.
 * @return Marker label string; owned by the player.
 */
const char* dlrl_MarkerName(const dlrl_Player* p, int index);

#ifdef __cplusplus
}
#endif

#endif
