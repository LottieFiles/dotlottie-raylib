# dotLottie-raylib

A Lottie/dotLottie animation component for Raylib. Drop the supplied `dlrl.c`/`dlrl.h` pair into your project and you can treat `.lottie` archives or raw Lottie JSON as animated sprites that sync with markers and respond to gameplay input.

<video src="./demo.mp4" width="640" controls muted loop playsinline>
  Your browser does not support embedded video. <a href="./demo.mp4">Download demo.mp4</a> instead.
</video>

## Quick start
- Make sure you have Raylib 5.x headers/libs plus Clang or GCC.
- For macOS users, you can install Raylib via Homebrew: `brew install raylib`
- The dotLottie runtime in `libs/dotlottie_player` currently targets macOS ARM64 arch. Linux/Windows/Android/iOS users can swap in the dotLottie-rs prebuilts from the dotLottie-rs release artifacts and adjust the `Makefile`.

```sh
make build                # compile dlrl + the sample
make run                  # launches build/example with super-man.lottie
make run ASSET=foo.lottie # point at any other .lottie/.json file
``` 