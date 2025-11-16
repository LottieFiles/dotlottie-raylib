# dotLottie-raylib Usage

## Setup
- Use the bundled raylib and dotLottie prebuilts in `third_party/raylib` and `third_party/dotlottie_player` (macOS arm64, Linux x86_64 by default). Replace them with the correct `lib`/`include` for your target as needed.
- Copy `dlrl.c` / `dlrl.h` into your project.
- Add `third_party/dotlottie_player/include` and `third_party/raylib/include` to compiler includes; add `third_party/dotlottie_player/lib/<os>/<arch>` and `third_party/raylib/lib/<os>/<arch>` to linker search paths.
- Link flags: `-ldotlottie_player -lraylib` plus platform libs (macOS: `-framework Cocoa -framework IOKit -framework OpenGL`; Linux: `-lGL -lm -lpthread -ldl -lrt -lX11`).
- Android/iOS/other targets: fetch matching dotlottie_player/raylib prebuilts from their releases and drop them into the `lib/<os>/<arch>` layout.

## Creating a Player
```c
dlrl_Config cfg = {
    .width = 0, .height = 0,           /* keep 0 to auto-size to asset */
    .speed = 1.0f, .loop = true,
    .mode = DLRL_MODE_FORWARD,
    .interpolate = true,
    .fit = DLRL_FIT_CONTAIN, .align = (Vector2){0.5f, 0.5f},
    .background = BLANK,
    .animation_id = NULL,
    .theme_id = NULL,
    .state_machine_id = NULL,
    .marker = NULL
};
dlrl_Player* p = dlrl_LoadDotLottieFile("super-man.lottie", &cfg);
dlrl_Play(p);
```

## Driving the Animation
Call once per frame:
```c
float dt = GetFrameTime();
dlrl_Update(p, dt);
Rectangle area = { 200, 120, 260, 200 }; // destination rect in screen space
dlrl_Draw(p, area, 0.0f, WHITE);         // rotation in degrees, tint color
```
Textures are updated internally; grab `dlrl_GetTexture` if you need custom batching.

## Markers and Variations
- Enumerate markers: `dlrl_MarkerCount` + `dlrl_MarkerName`.
- Jump to a marker: `dlrl_SetMarker(p, "Punch")`; pass `NULL` to play the whole timeline.
- Switch animation via `dlrl_SetAnimation`, theme via `dlrl_SetTheme`, or change playback style with `dlrl_SetMode` and `dlrl_SetLoop`.

## Platform Notes
- `third_party/dotlottie_player` ships test prebuilts for macOS arm64 and Linux x86_64/arm64 only. Replace them with binaries from the dotlottie_player releases for your actual target, then `make clean && make build`.
- Keep your compiler target triple aligned with the dotLottie library architecture to avoid undefined symbol errors.
