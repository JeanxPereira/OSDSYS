#pragma once

// ============================================================================
// MathTypes.h - OSDSYS math utilities (renamed from Math.h to avoid conflicts)
// Automatically includes Platform.h for required types and functions
// ============================================================================
#include "Platform.h"

// ============================================================================
// Vec3 - 3D Vector
// ============================================================================
struct Vec3 {
    float x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    Vec3(float v) : x(v), y(v), z(v) {}

    Vec3 operator+(const Vec3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vec3 operator*(float s) const { return {x*s, y*s, z*s}; }
    Vec3 operator/(float s) const { return {x/s, y/s, z/s}; }

    float Length() const { return sqrt(x*x + y*y + z*z); }  // Uses global sqrt from Platform.h
    Vec3 Normalize() const { 
        float len = Length(); 
        return len > 0 ? *this / len : Vec3(0, 0, 0);
    }
};

// ============================================================================
// Color - RGBA
// ============================================================================
struct Color {
    float r, g, b, a;

    Color() : r(1), g(1), b(1), a(1) {}
    Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
    Color(uint8_t r8, uint8_t g8, uint8_t b8, uint8_t a8 = 255) 
        : r(r8/255.0f), g(g8/255.0f), b(b8/255.0f), a(a8/255.0f) {}
};

// ============================================================================
// Easing Functions - Based on PS2 curves
// ============================================================================
namespace Easing {
    inline float Linear(float t) { 
        return t; 
    }
    
    inline float EaseInQuad(float t) { 
        return t * t; 
    }
    
    inline float EaseOutQuad(float t) { 
        return t * (2.0f - t); 
    }
    
    inline float EaseInOutQuad(float t) {
        return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
    }
    
    inline float EaseInCubic(float t) {
        return t * t * t;
    }

    inline float EaseOutCubic(float t) {
        float f = t - 1.0f;
        return f * f * f + 1.0f;
    }
    
    inline float SmoothStep(float t) {
        return t * t * (3.0f - 2.0f * t);
    }

    inline float SmootherStep(float t) {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }
}

// ============================================================================
// Convert - OSDSYS coordinate conversions
// Based on analysis of sub_2376C0 (GeometryRenderer)
// ============================================================================
namespace Convert {
    // Position: int16 → float (divide by 1024)
    inline float Position(int16_t val) {
        return (float)val * 0.0009765625f; // 1/1024
    }

    // UV: int16 → float (divide by 4096)
    inline float UV(int16_t val) {
        return (float)val * 0.00024414062f; // 1/4096
    }

    // Color: uint8 → float [0-1]
    inline float ColorComponent(uint8_t val) {
        return (float)val / 255.0f;
    }

    // W component (always 1.0 in OSDSYS)
    inline float W() {
        return 1.0f; // 0x3F800000
    }
}

// ============================================================================
// Math Utils
// ============================================================================
namespace Math {
    constexpr float PI = 3.14159265359f;
    constexpr float TWO_PI = 6.28318530718f;
    constexpr float HALF_PI = 1.57079632679f;

    inline float Deg2Rad(float degrees) { 
        return degrees * (PI / 180.0f); 
    }

    inline float Rad2Deg(float radians) { 
        return radians * (180.0f / PI); 
    }

    inline float Clamp(float value, float min, float max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }

    inline float Lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }

    inline Vec3 Lerp(const Vec3& a, const Vec3& b, float t) {
        return Vec3(
            Lerp(a.x, b.x, t),
            Lerp(a.y, b.y, t),
            Lerp(a.z, b.z, t)
        );
    }
}
