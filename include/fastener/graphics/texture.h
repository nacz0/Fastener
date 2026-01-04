#pragma once

#include "fastener/core/types.h"
#include <memory>
#include <string>

namespace fst {

//=============================================================================
// Texture
//=============================================================================
class Texture {
public:
    Texture();
    ~Texture();
    
    // Non-copyable, movable
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;
    
    // Creation
    bool create(int width, int height, const void* data = nullptr, int channels = 4);
    bool loadFromFile(const std::string& path);
    bool loadFromMemory(const void* data, size_t size);
    void destroy();
    
    // Update
    void update(int x, int y, int width, int height, const void* data);
    
    // Properties
    int width() const { return m_width; }
    int height() const { return m_height; }
    Vec2 size() const { return {static_cast<float>(m_width), static_cast<float>(m_height)}; }
    uint32_t handle() const { return m_handle; }
    bool isValid() const { return m_handle != 0; }
    
private:
    uint32_t m_handle = 0;
    int m_width = 0;
    int m_height = 0;
    int m_channels = 4;
};

} // namespace fst
