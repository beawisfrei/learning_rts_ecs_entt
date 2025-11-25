#pragma once
#include <string>
#include <entt/entt.hpp>
#include <nlohmann/json.hpp>
#include "../components/components.hpp"

class ResourceLoader {
public:
    static bool load_config(const std::string& path, nlohmann::json& out_json);
    static unsigned int load_texture(const std::string& path);
    
    // Find project root directory containing data folder and set as working directory
    static bool SetDataDirectory();
    
    // Get image dimensions without loading the full texture
    static bool GetImageDimensions(const std::string& path, int& width, int& height);
    
    // Helper to apply config to registry/game state?
    // For now, just specific loaders
};

