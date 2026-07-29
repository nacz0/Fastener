#pragma once

#include "fastener/core/types.h"
#include <memory>

namespace fst {

// Forward declarations
class DrawList;
class Texture;

//=============================================================================
// Renderer - OpenGL rendering backend
//=============================================================================
class Renderer {
public:
    Renderer();
    ~Renderer();
    
    // Non-copyable
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    
    // Initialization
    bool init();
    void shutdown();

    /**
     * Release objects that belong only to the current GL context (currently
     * the VAO). Shared resources remain alive until shutdown().
     */
    [[nodiscard]] bool releaseCurrentContextResources();
    
    // Frame
    void beginFrame(int width, int height, float dpiScale);
    void endFrame();
    
    // Rendering
    void render(const DrawList& drawList);
    
    // White texture for solid color rendering
    uint32_t whiteTexture() const;
    
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace fst
