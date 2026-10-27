#pragma once

// Where the PNGs and the font live. The desktop build runs from build/, so the
// assets sit one level up; the web build packs them into /assets in the
// Emscripten virtual filesystem (see --preload-file in CMakeLists.txt).
#ifdef __EMSCRIPTEN__
#define ASSET_DIR "/assets/"
#else
#define ASSET_DIR "../assets/"
#endif
