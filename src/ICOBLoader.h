#pragma once
#include <string>
#include <vector>
#include <cstdint>

/**
 * @brief ICOB Model Loader for PS2 OSDSYS Icon Objects (.icn / .ico)
 * 
 * Re-implemented based on ps2iconsys reference logic.
 */
class ICOBLoader {
public:
    // Output vertex format for OpenGL
    struct Vertex {
        float position[3];  // XYZ
        float texcoord[2];  // UV
        float color[4];     // RGBA (Normalized 0.0 - 1.0)
        float normal[3];    // Normal XYZ
    };

    ICOBLoader();
    ~ICOBLoader();

    /**
     * @brief Load ICOB file from disk
     */
    bool Load(const std::string& filepath);

    const std::vector<Vertex>& GetVertices() const { return converted_vertices_; }
    const std::vector<uint32_t>& GetIndices() const { return indices_; }

    size_t GetTriangleCount() const { return indices_.size() / 3; }
    size_t GetVertexCount() const { return converted_vertices_.size(); }
    bool IsLoaded() const { return loaded_; }

private:
    // --- Estruturas Internas do Arquivo (Binário do PS2) ---
    // Referência: PS2Icon::Icon_Header em ps2_ps2icon.hpp
    
    #pragma pack(push, 1)
    
    struct ICOBHeader {
        uint32_t file_id;           // 0x00010000
        uint32_t animation_shapes;  // Número de keyframes por vértice (Morph Targets)
        uint32_t texture_type;      // Tipo de textura (comprimida ou não)
        uint32_t reserved;          // Geralmente 0x3F800000 (1.0f)
        uint32_t n_vertices;        // Número total de vértices (deve ser divisível por 3)
    };

    // Estrutura de Coordenada comprimida (4x int16)
    // Usada para Vértices e Normais
    struct FixedCoord {
        int16_t x, y, z, w;
    };

    // Estrutura de Textura e Cor
    struct TextureDataRaw {
        int16_t u, v;       // Coordenadas UV
        uint32_t color;     // Cor 32-bit (RGBA ou ABGR)
    };

    #pragma pack(pop)

    // Data Holders
    ICOBHeader header_;
    std::vector<Vertex> converted_vertices_;
    std::vector<uint32_t> indices_;
    bool loaded_;

    // Conversores
    float FixedToFloat(int16_t val);
};