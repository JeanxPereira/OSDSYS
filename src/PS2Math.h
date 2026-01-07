#pragma once
#include <cmath>
#include <cstring>
#include <cstdio>

// ============================================================================
// PS2Math - Simulação da Matemática da BIOS/SDK Sony (VU0 Macros)
// ============================================================================

namespace PS2Math {

    static constexpr float PI = 3.14159265359f;

    // Estrutura de Matriz 4x4 Compatível com OpenGL mas lógica PS2
    struct MATRIX {
        float m[4][4]; // Row-major para alinhamento mental C++, mas cuidado com GL
    };

    struct VECTOR {
        float x, y, z, w;
    };

    // --- Funções Clássicas "sceVu0" ---

    inline void UnitMatrix(MATRIX& mat) {
        memset(mat.m, 0, sizeof(mat.m));
        mat.m[0][0] = 1.0f; mat.m[1][1] = 1.0f;
        mat.m[2][2] = 1.0f; mat.m[3][3] = 1.0f;
    }

    // A BIOS do PS2 usa ordem de rotação específica (geralmente ZXY ou XYZ dependendo da lib)
    // Aqui replicamos a criação manual de matrizes de rotação
    inline void RotMatrix(MATRIX& mat, const VECTOR& rot) {
        float cx = cosf(rot.x), sx = sinf(rot.x);
        float cy = cosf(rot.y), sy = sinf(rot.y);
        float cz = cosf(rot.z), sz = sinf(rot.z);

        // Z * Y * X Rotation (Standard PS2 Model logic)
        // Calculado para resultar numa matriz combinada
        // Rz * Ry * Rx
        
        float sxsy = sx * sy;
        float cxsy = cx * sy;

        mat.m[0][0] = cy * cz;
        mat.m[0][1] = sxsy * cz + cx * sz;
        mat.m[0][2] = -cxsy * cz + sx * sz;
        mat.m[0][3] = 0.0f;

        mat.m[1][0] = -cy * sz;
        mat.m[1][1] = -sxsy * sz + cx * cz;
        mat.m[1][2] = cxsy * sz + sx * cz;
        mat.m[1][3] = 0.0f;

        mat.m[2][0] = sy;
        mat.m[2][1] = -sx * cy;
        mat.m[2][2] = cx * cy;
        mat.m[2][3] = 0.0f;

        mat.m[3][0] = 0.0f;
        mat.m[3][1] = 0.0f;
        mat.m[3][2] = 0.0f;
        mat.m[3][3] = 1.0f;
    }

    inline void TransMatrix(MATRIX& mat, const VECTOR& trans) {
        // Aplica translação a uma matriz existente
        // No PS2, isso apenas soma ao vetor de translação (Row 3 ou Column 3)
        // Aqui assumimos formato OpenGL (Coluna 3) para facilitar o upload
        mat.m[3][0] += trans.x;
        mat.m[3][1] += trans.y;
        mat.m[3][2] += trans.z;
    }

    // "sceVu0ApplyMatrix" - Transforma um vetor por uma matriz
    inline VECTOR ApplyMatrix(const MATRIX& m, const VECTOR& v) {
        VECTOR out;
        out.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
        out.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
        out.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
        out.w = 1.0f; // Ignorando projeção W aqui para transformações afim
        return out;
    }

    // Lissajous Curve (A Base de todas as luzes da OSDSYS)
    // T = tempo, A/B = frequencias, D = fase
    inline float Lissajous(float t, float amp, float freq, float phase) {
        return amp * sinf(t * freq + phase);
    }
}