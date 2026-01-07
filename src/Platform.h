#pragma once

// ============================================================================
// Platform.h - Pure C++ cross-platform layer  
// CRITICAL: Include FIRST in every .cpp file!
// ============================================================================

// 1. Windows/MSVC specific fixes FIRST
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
#endif

// 2. Math defines BEFORE any includes
#define _USE_MATH_DEFINES

// 3. C++ headers ONLY (no mixing with C headers!)
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>

// 4. Bring C standard library types and functions into global namespace
// Types from <cstdint>
using std::int8_t;
using std::int16_t;
using std::int32_t;
using std::int64_t;
using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;

// Math functions from <cmath>
using std::sqrt;
using std::sin;
using std::cos;
using std::tan;
using std::asin;
using std::acos;
using std::atan;
using std::atan2;
using std::pow;
using std::abs;
using std::fabs;

// 5. C++ STL headers
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <fstream>

// 6. Windows headers (MUST come before OpenGL on Windows)
#ifdef _WIN32
    #include <windows.h>
#endif

// 7. SDL2 (after all standard headers)
#include <SDL2/SDL.h>

// 8. OpenGL (after windows.h on Windows)
// NOTE: When using GLEW, do NOT include gl.h here!
// GLEW must be included FIRST in .cpp files, then it includes gl.h
#ifdef OSDSYS_USE_OPENGL
    // OpenGL headers will be included by GLEW in source files
    // Do not include gl.h here to avoid conflicts
#endif

// ============================================================================
// USAGE: Include Platform.h FIRST in every .cpp file
// ============================================================================
