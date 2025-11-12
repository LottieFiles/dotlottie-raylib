#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#include "raylib.h"
#include "dlrl.h"

typedef struct {
    int key;
    const char* markerName;
    float moveDir;
    bool available;
} MarkerBinding;

static float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static bool equals_ignore_case(const char* a, const char* b) {
    if (!a || !b) return false;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return false;
        }
        ++a;
        ++b;
    }
    return *a == 0 && *b == 0;
}

static bool marker_exists(dlrl_Player* player, const char* name) {
    if (!player || !name) return false;
    int count = dlrl_MarkerCount(player);
    for (int i = 0; i < count; ++i) {
        const char* candidate = dlrl_MarkerName(player, i);
        if (candidate && equals_ignore_case(candidate, name)) {
            return true;
        }
    }
    return false;
}

int main(int argc, char** argv) {
    const char* asset_path = (argc > 1 && argv[1]) ? argv[1] : "super-man.lottie";
    InitWindow(960, 540, "dotLottie Raylib");
    SetTargetFPS(60);

    dlrl_Config cfg = {
        .width = 0, .height = 0,
        .speed = 1.0f, .loop = true,
        .mode = DLRL_MODE_FORWARD,
        .interpolate = true,
        .fit = DLRL_FIT_CONTAIN,
        .align = {0.5f, 0.5f},
        .background = BLANK,
        .animation_id = NULL,
        .theme_id = NULL,
        .state_machine_id = NULL
    };

    dlrl_Player* p = dlrl_LoadDotLottieFile(asset_path, &cfg);
    if (!p) {
        TraceLog(LOG_ERROR, "Failed to load dotLottie asset: %s", asset_path);
        CloseWindow();
        return 1;
    }
    dlrl_Play(p);

    MarkerBinding bindings[] = {
        { KEY_J, "Punch", 0.f, false },
        { KEY_K, "Lazer", 0.f, false },
        { KEY_W, "UpperCut", 0.f, false },
        { KEY_D, "Right", 1.f, false },
        { KEY_A, "Left", -1.f, false },
    };
    const int bindingCount = (int)(sizeof(bindings) / sizeof(bindings[0]));
    for (int i = 0; i < bindingCount; ++i) {
        bindings[i].available = marker_exists(p, bindings[i].markerName);
    }

    const char* idleMarker = marker_exists(p, "Idle") ? "Idle" : NULL;
    const char* idleLabel = idleMarker ? idleMarker : "Full Timeline";
    const char* currentMarker = idleMarker;
    const char* currentLabel = idleLabel;
    if (idleMarker) dlrl_SetMarker(p, idleMarker);
    else dlrl_SetMarker(p, NULL);

    Vector2 heroCenter = { GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f };
    const Vector2 animSize = { 260, 200 };
    const float heroSpeed = 220.0f;
    const float halfWidth = animSize.x * 0.5f;
    const float minX = halfWidth + 20.f;
    const float maxX = GetScreenWidth() - halfWidth - 20.f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        const MarkerBinding* engagedBinding = NULL;
        float moveIntent = 0.f;
        for (int i = 0; i < bindingCount; ++i) {
            MarkerBinding* binding = &bindings[i];
            if (!binding->available) continue;
            if (binding->moveDir != 0.f && IsKeyDown(binding->key)) {
                moveIntent += binding->moveDir;
            }
            if (!engagedBinding && IsKeyDown(binding->key)) {
                engagedBinding = binding;
            }
        }
        heroCenter.x = clampf(heroCenter.x + moveIntent * heroSpeed * dt, minX, maxX);

        const char* desiredMarker = engagedBinding ? engagedBinding->markerName : idleMarker;
        const char* desiredLabel = engagedBinding ? engagedBinding->markerName : idleLabel;
        if (desiredMarker != currentMarker) {
            if (dlrl_SetMarker(p, desiredMarker)) {
                currentMarker = desiredMarker;
                currentLabel = desiredLabel;
            }
        } else {
            currentLabel = desiredLabel;
        }

        dlrl_Update(p, dt);

        Rectangle area = {
            heroCenter.x - animSize.x * 0.5f,
            heroCenter.y - animSize.y * 0.5f,
            animSize.x,
            animSize.y
       };

        BeginDrawing();
        ClearBackground(RAYWHITE);
        dlrl_Draw(p, area, 0.0f, WHITE);

        DrawText("Controls: A=Left  D=Right  W=UpperCut  J=Punch  K=Lazer", 20, 20, 18, DARKGRAY);
        DrawText(TextFormat("State: %s", currentLabel), 20, 46, 24, MAROON);
        if (!idleMarker) {
            DrawText("No idle marker in this asset; releasing keys replays the full clip.", 20, 78, 16, DARKGRAY);
        }
        EndDrawing();
    }

    dlrl_Unload(p);
    CloseWindow();
    return 0;
}
