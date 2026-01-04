#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG

#include "stb_image.h"
#include "fastener/graphics/texture.h"
#include <vector>
#include <cstdio>

// OpenGL function types
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

// We need some GL functions that might not be in the default headers
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif

namespace fst {

Texture::Texture() = default;

Texture::~Texture() {
    destroy();
}

Texture::Texture(Texture&& other) noexcept 
    : m_handle(other.m_handle)
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_channels(other.m_channels)
{
    other.m_handle = 0;
    other.m_width = 0;
    other.m_height = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        destroy();
        m_handle = other.m_handle;
        m_width = other.m_width;
        m_height = other.m_height;
        m_channels = other.m_channels;
        other.m_handle = 0;
        other.m_width = 0;
        other.m_height = 0;
    }
    return *this;
}

bool Texture::create(int width, int height, const void* data, int channels) {
    destroy();
    
    m_width = width;
    m_height = height;
    m_channels = channels;
    
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    GLenum format = GL_RGBA;
    GLenum internalFormat = GL_RGBA;
    
    if (channels == 1) {
        format = GL_RED;
        internalFormat = GL_RED;
    } else if (channels == 3) {
        format = GL_RGB;
        internalFormat = GL_RGB;
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    m_handle = texture;
    return true;
}

bool Texture::loadFromFile(const std::string& path) {
    // Read file
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    std::vector<uint8_t> data(size);
    fread(data.data(), 1, size, f);
    fclose(f);
    
    return loadFromMemory(data.data(), size);
}

bool Texture::loadFromMemory(const void* data, size_t size) {
    int width, height, channels;
    stbi_uc* pixels = stbi_load_from_memory(
        static_cast<const stbi_uc*>(data), 
        static_cast<int>(size),
        &width, &height, &channels, 4
    );
    
    if (!pixels) return false;
    
    bool result = create(width, height, pixels, 4);
    stbi_image_free(pixels);
    
    return result;
}

void Texture::destroy() {
    if (m_handle != 0) {
        GLuint tex = m_handle;
        glDeleteTextures(1, &tex);
        m_handle = 0;
    }
    m_width = 0;
    m_height = 0;
}

void Texture::update(int x, int y, int width, int height, const void* data) {
    if (m_handle == 0) return;
    
    glBindTexture(GL_TEXTURE_2D, m_handle);
    
    GLenum format = GL_RGBA;
    if (m_channels == 1) format = GL_RED;
    else if (m_channels == 3) format = GL_RGB;
    
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, format, GL_UNSIGNED_BYTE, data);
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace fst
