#include "Platform.h"  // MUST BE FIRST
#include "Assets.h"
#include "ICOBLoader.h"
#include <cstring>
#include <cstdlib>  // for abs()

// ============================================================================
// AssetLoader Implementation
// ============================================================================

AssetLoader::AssetLoader() {
    iconDirectory = "assets/icons/";
    textureDirectory = "assets/textures/";
}

AssetLoader::~AssetLoader() {
}

void AssetLoader::SetIconDirectory(const std::string& dir) {
    iconDirectory = dir;
    if (!iconDirectory.empty() && iconDirectory.back() != '/' && iconDirectory.back() != '\\') {
        iconDirectory += '/';
    }
}

void AssetLoader::SetTextureDirectory(const std::string& dir) {
    textureDirectory = dir;
    if (!textureDirectory.empty() && textureDirectory.back() != '/' && textureDirectory.back() != '\\') {
        textureDirectory += '/';
    }
}

bool AssetLoader::LoadICOB(const std::string& name, ICOBModel& outModel) {
    // ICOB naming: ICOBDISC, ICOBPS2M, etc.
    // Files are in assets/icons/<name>.bin
    
    std::string filename = name + ".bin";
    std::string fullPath = iconDirectory + filename;
    
    printf("[AssetLoader] Loading ICOB: %s\n", fullPath.c_str());
    
    return LoadICOBFromPath(fullPath, outModel);
}

bool AssetLoader::LoadICOBFromPath(const std::string& path, ICOBModel& outModel) {
    // Use ICOBLoader to load the file
    ICOBLoader loader;
    
    if (!loader.Load(path)) {
        printf("[AssetLoader] Failed to load ICOB: %s\n", path.c_str());
        return false;
    }
    
    printf("[AssetLoader] ICOB loaded: %zu vertices, %zu triangles\n",
           loader.GetVertexCount(), loader.GetTriangleCount());
    
    // Convert from ICOBLoader format to ICOBModel format
    const auto& icobVertices = loader.GetVertices();
    const auto& icobIndices = loader.GetIndices();
    
    // Clear output model
    outModel.vertices.clear();
    outModel.indices.clear();
    
    // Set header (dummy values for now since ICOBLoader handles the real header internally)
    outModel.header.magic = 0x00010000;
    outModel.header.field1 = 0x00000001;
    outModel.header.field2 = static_cast<uint32_t>(icobVertices.size());
    outModel.header.field3 = static_cast<uint32_t>(icobIndices.size() / 3);
    
    // Convert vertices
    for (const auto& v : icobVertices) {
        OSDVertex vertex;
        vertex.position = Vec3{v.position[0], v.position[1], v.position[2]};
        vertex.normal = Vec3{v.normal[0], v.normal[1], v.normal[2]};
        vertex.u = v.texcoord[0];
        vertex.v = v.texcoord[1];
        vertex.color = Color{v.color[0], v.color[1], v.color[2], v.color[3]};
        
        outModel.vertices.push_back(vertex);
    }
    
    // Convert indices (from uint32_t to uint16_t)
    for (uint32_t idx : icobIndices) {
        outModel.indices.push_back(static_cast<uint16_t>(idx));
    }
    
    printf("[AssetLoader] Converted to ICOBModel: %zu vertices, %zu indices\n",
           outModel.vertices.size(), outModel.indices.size());
    
    return true;
}

bool AssetLoader::ParseICOBData(const uint8_t* data, size_t size, ICOBModel& outModel) {
    // This function is no longer used since ICOBLoader handles binary parsing
    // Kept for compatibility but not implemented
    printf("[AssetLoader] ParseICOBData is deprecated, use LoadICOBFromPath instead\n");
    return false;
}

bool AssetLoader::ReadFileToBuffer(const std::string& path, std::vector<uint8_t>& outData) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    
    if (!file.is_open()) {
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    outData.resize(size);
    if (!file.read((char*)outData.data(), size)) {
        return false;
    }

    return true;
}
