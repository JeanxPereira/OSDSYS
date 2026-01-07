#pragma once
#include <cstdint>

// ============================================================================
// PS2Constants.h
// Adaptado de 'ee-draw' (ps2sdk) para o emulador OSDSYS PC
// ============================================================================

// --- PSM (Pixel Storage Modes) - Formatos de Cor na VRAM ---
enum PS2_PSM {
    GS_PSM_32  = 0x00, // RGBA32
    GS_PSM_24  = 0x01, // RGB24
    GS_PSM_16  = 0x02, // RGBA16 (A1RGB5)
    GS_PSM_16S = 0x0A, // Signed 16 bit
    GS_PSM_8   = 0x13, // 8-bit Indexed
    GS_PSM_4   = 0x14, // 4-bit Indexed
    GS_PSM_8H  = 0x1B,
    GS_PSM_4HL = 0x24,
    GS_PSM_4HH = 0x2C
};

struct PS2Color16 {
    // Little Endian Layout na memoria
    uint16_t r : 5;
    uint16_t g : 5;
    uint16_t b : 5;
    uint16_t a : 1;
};

// --- Tipos de Primitivas ---
enum PS2_PRIM_TYPE {
    PRIM_POINT           = 0x00,
    PRIM_LINE            = 0x01,
    PRIM_LINE_STRIP      = 0x02,
    PRIM_TRIANGLE        = 0x03,
    PRIM_TRIANGLE_STRIP  = 0x04,
    PRIM_TRIANGLE_FAN    = 0x05,
    PRIM_SPRITE          = 0x06
};

// --- Teste Z (Profundidade) ---
enum PS2_ZTEST {
    ZTEST_METHOD_ALLFAIL       = 0,
    ZTEST_METHOD_ALLPASS       = 1,
    ZTEST_METHOD_GREATER_EQUAL = 2,
    ZTEST_METHOD_GREATER       = 3
};

// --- Alpha Blending Methods ---
// Formula: (A - B) * C + D
// A/B/D Sources:
// 0 = CS (Color Source/Pixel Shader)
// 1 = CD (Color Dest/Framebuffer)
// 2 = ZERO
// C Source:
// 0 = AS (Alpha Source)
// 1 = AD (Alpha Dest)
// 2 = FIX (Alpha Fixo)

struct PS2_BLEND_MODE {
    int color1; // A
    int color2; // B
    int alpha;  // C
    int color3; // D
    uint8_t fixed_alpha;
};

// --- Estrutura de Vértice em Ponto Fixo (GS nativo) ---
// O GS usa coordenadas 12.4 (divide por 16.0f para obter float)
// O centro da tela na PS2 é offsetado em 2048.0
struct gs_xyz_t {
    uint16_t x; // 12.4 fixpoint
    uint16_t y; // 12.4 fixpoint
    uint32_t z; // 32 bit zbuffer
    
    // Helpers para converter para PC
    float GetFloatX() const { return (float)x / 16.0f - 2048.0f; }
    float GetFloatY() const { return (float)y / 16.0f - 2048.0f; }
};

struct gs_rgbaq_t {
    uint8_t r, g, b, a;
    float q;
};

#define CLUT_width 16
#define CLUT_height 16