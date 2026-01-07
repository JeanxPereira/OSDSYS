#include "ICOBLoader.h"
#include <fstream>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <iostream>

// Baseado em ps2_ps2icon.cpp: convert_f16_to_f32
// PS2 usa ponto fixo onde 4096 = 1.0
static constexpr float F16_SCALE = 1.0f / 4096.0f;

ICOBLoader::ICOBLoader() : loaded_(false) {
    memset(&header_, 0, sizeof(header_));
}

ICOBLoader::~ICOBLoader() {
    converted_vertices_.clear();
    indices_.clear();
}

float ICOBLoader::FixedToFloat(int16_t val) {
    return static_cast<float>(val) * F16_SCALE;
}

bool ICOBLoader::Load(const std::string& filepath) {
    loaded_ = false;
    converted_vertices_.clear();
    indices_.clear();

    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        printf("[ICOBLoader] ERROR: Cannot open file: %s\n", filepath.c_str());
        return false;
    }

    // 1. Ler Cabeçalho (20 bytes)
    // ps2_ps2icon struct tem 5 uint32s = 20 bytes
    file.read(reinterpret_cast<char*>(&header_), sizeof(ICOBHeader));

    // Validação Básica
    if (header_.file_id != 0x00010000) {
        printf("[ICOBLoader] WARNING: Magic bytes mismatch in %s (Got 0x%X). Might be corrupted.\n", 
               filepath.c_str(), header_.file_id);
    }

    // A referência diz: n_vertices tem que ser divisível por 3 (GL_TRIANGLES puro)
    if (header_.n_vertices == 0 || (header_.n_vertices % 3 != 0)) {
        printf("[ICOBLoader] ERROR: Invalid vertex count (%u) in %s\n", header_.n_vertices, filepath.c_str());
        return false;
    }

    printf("[ICOBLoader] Loading %s: %u verts, %u shapes per vert\n", 
           filepath.c_str(), header_.n_vertices, header_.animation_shapes);

    // 2. Ler Dados de Vértices
    // O arquivo .icn organiza dados "Por Vértice", mas com Shapes intercalados
    converted_vertices_.reserve(header_.n_vertices);

    // Buffers temporários para leitura
    std::vector<FixedCoord> shapeBuffer(header_.animation_shapes);
    FixedCoord normalRaw;
    TextureDataRaw textureRaw;

    for (uint32_t i = 0; i < header_.n_vertices; ++i) {
        Vertex outVert;

        // A. Ler posições para todos os "Animation Shapes" (Morph Targets)
        // O PS2 guarda: Shape0_Pos, Shape1_Pos, ... ShapeN_Pos
        // Nós vamos ler todos, mas usar apenas o Shape 0 para o mesh estático.
        file.read(reinterpret_cast<char*>(shapeBuffer.data()), sizeof(FixedCoord) * header_.animation_shapes);

        // Usar Shape 0 (Geometria Base)
        outVert.position[0] = FixedToFloat(shapeBuffer[0].x);
        outVert.position[1] = FixedToFloat(shapeBuffer[0].y);
        outVert.position[2] = FixedToFloat(shapeBuffer[0].z);

        // B. Ler Normal (uma por vértice, compartilhada por todos shapes)
        file.read(reinterpret_cast<char*>(&normalRaw), sizeof(FixedCoord));
        outVert.normal[0] = FixedToFloat(normalRaw.x);
        outVert.normal[1] = FixedToFloat(normalRaw.y);
        outVert.normal[2] = FixedToFloat(normalRaw.z);

        // C. Ler Textura e Cor
        file.read(reinterpret_cast<char*>(&textureRaw), sizeof(TextureDataRaw));
        
        // Converter UV
        outVert.texcoord[0] = FixedToFloat(textureRaw.u);
        outVert.texcoord[1] = FixedToFloat(textureRaw.v); // OpenGL pode precisar de 1.0 - v

        // Converter Cor (0xAABBGGRR ou 0xAARRGGBB - PS2 geralmente é Little Endian)
        // ps2_ps2icon seta default color como 0xFFFFFFFF no loader, indicando que a cor do arquivo
        // talvez seja ignorada ou seja multiplicativa. Vamos extrair por via das dúvidas.
        uint8_t r = (textureRaw.color) & 0xFF;
        uint8_t g = (textureRaw.color >> 8) & 0xFF;
        uint8_t b = (textureRaw.color >> 16) & 0xFF;
        uint8_t a = (textureRaw.color >> 24) & 0xFF; // As vezes 0x80 = 100% no PS2

        // No OSD, normalmente vértices têm cor fixa, a textura dá a cor real.
        // Se alpha for 0 mas rgb tiver cor, pode ser o formato 0x80 bug.
        // Vamos forçar alpha 1.0 se for suspeito ou normalizar 0-128->0-255
        float alpha = (a >= 0x80) ? 1.0f : (a / 128.0f);
        
        outVert.color[0] = r / 255.0f;
        outVert.color[1] = g / 255.0f;
        outVert.color[2] = b / 255.0f;
        outVert.color[3] = alpha; 

        converted_vertices_.push_back(outVert);
    }

    // 3. Gerar Índices
    // Como o ICOB é uma lista bruta de triângulos, não há indexação (glDrawArrays),
    // mas nosso renderer usa glDrawElements. Vamos gerar índices sequenciais: 0, 1, 2...
    indices_.resize(header_.n_vertices);
    for (uint32_t i = 0; i < header_.n_vertices; ++i) {
        indices_[i] = i;
    }

    if (file.fail()) {
        printf("[ICOBLoader] ERROR: Unexpected EOF while reading vertices\n");
        return false;
    }

    // Nota: O arquivo continua com cabeçalhos de animação e dados de textura (TIM2/Raw)
    // Por enquanto, só precisamos da malha. O arquivo será fechado e o resto ignorado.
    
    file.close();
    loaded_ = true;
    return true;
}