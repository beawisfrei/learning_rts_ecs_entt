#include "resource_loader.hpp"
#include <fstream>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION // Define this only in one .cpp file to implement the library
#include <stb_image.h>
#include "gl_loader.hpp"

bool ResourceLoader::load_config(const std::string& path, nlohmann::json& out_json) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << path << std::endl;
        return false;
    }
    try {
        file >> out_json;
    } catch (const std::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return false;
    }
    return true;
}

unsigned int ResourceLoader::load_texture(const std::string& path) {
    unsigned int textureID;
    // Note: glGenTextures is defined in SDL_opengl.h, but if we commented out our typedefs
    // we should rely on SDL's or the system's version. 
    // If we are on Windows, we need to ensure it's available.
    // Standard 1.1 functions are global symbols.
    
    // We commented out typedefs in gl_loader.hpp/cpp to avoid redefinition errors.
    // This means we should use the global functions provided by <SDL_opengl.h> (which includes <gl.h>).
    
    ::glGenTextures(1, &textureID);
    
    int width, height, nrChannels;
    // stbi_set_flip_vertically_on_load(true); 

    unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = GL_RGB;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        ::glBindTexture(GL_TEXTURE_2D, textureID);
        ::glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        
        // Use our namespaced function for modern GL
        if (RTS_GL::glGenerateMipmap)
            RTS_GL::glGenerateMipmap(GL_TEXTURE_2D);

        ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
        ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 

        stbi_image_free(data);
    } else {
        std::cerr << "Failed to load texture: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
