#pragma once
#include "MathTypes.h"  // Platform.h is included automatically
#include <string>
#include <vector>

// ============================================================================
// OSDSYS Asset Structures
// Based on RAM dumps and ICOB* format analysis
// ============================================================================

// Vertex structure (converted from fixed-point or raw floats)
struct OSDVertex {
    Vec3 position;
    Vec3 normal;
    float u, v;     // UV coordinates
    Color color;
};

// ============================================================================
// ICOB Format - 3D Icon Models
// Extracted from eeMemory.bin under ICOIMAGE section
// Files: ICOBDISC, ICOBCDDA, ICOBPS1M, ICOBPS2M, ICOBPKST, etc.
// ============================================================================

struct ICOBHeader {
    uint32_t magic;         // 0x00000100
    uint32_t field1;        // Unknown, varies
    uint32_t field2;        // Possibly vertex/triangle count
    uint32_t field3;        // Unknown
};

struct ICOBModel {
    ICOBHeader header;
    std::vector<OSDVertex> vertices;
    std::vector<uint16_t> indices;
    
    bool IsValid() const { return !vertices.empty(); }
};

// ============================================================================
// Asset Loader
// Loads ICOB* 3D models from assets/icons/
// ============================================================================
class AssetLoader {
public:
    AssetLoader();
    ~AssetLoader();

    // Load ICOB model by name (e.g., "ICOBDISC", "ICOBPS2M")
    bool LoadICOB(const std::string& name, ICOBModel& outModel);
    
    // Load ICOB from file path
    bool LoadICOBFromPath(const std::string& path, ICOBModel& outModel);

    // Set base directories
    void SetIconDirectory(const std::string& dir);
    void SetTextureDirectory(const std::string& dir);

private:
    std::string iconDirectory;
    std::string textureDirectory;

    // Parse ICOB binary data
    bool ParseICOBData(const uint8_t* data, size_t size, ICOBModel& outModel);
    
    // Helper to read file
    bool ReadFileToBuffer(const std::string& path, std::vector<uint8_t>& outData);
};

// ============================================================================
// Deprecated: Old _OSDPIC format
// Kept for reference - actual 3D models are ICOB* files
// ============================================================================

struct OSDPICHeader {
    uint32_t magic;
    uint32_t vertexCount;
    uint32_t stripType;
    float param;
};

struct OSDPICAsset {
    OSDPICHeader header;
    std::vector<OSDVertex> vertices;
    std::vector<uint16_t> indices;
    
    bool IsValid() const { return !vertices.empty(); }
};
