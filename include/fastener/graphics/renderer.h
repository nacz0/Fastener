#pragma once

#include "fastener/core/types.h"
#include <memory>

namespace fst {

// Forward declarations
class DrawList;
class Texture;

/**
 * Rendering boundary consumed by Context.
 *
 * Implementations own backend-specific GPU state. This interface also makes
 * Context frame and resource lifetime behavior testable without an OpenGL
 * context.
 */
class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual bool init() = 0;
    virtual void shutdown() = 0;
    [[nodiscard]] virtual bool releaseCurrentContextResources() = 0;
    virtual void beginFrame(int width, int height, float dpiScale) = 0;
    virtual void endFrame() = 0;
    virtual void render(const DrawList& drawList) = 0;
    virtual uint32_t whiteTexture() const = 0;
};

//=============================================================================
// Renderer - OpenGL rendering backend
//=============================================================================
class Renderer final : public IRenderer {
public:
    Renderer();
    ~Renderer() override;
    
    // Non-copyable
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    
    // Initialization
    bool init() override;
    void shutdown() override;

    /**
     * Release objects that belong only to the current GL context (currently
     * the VAO). Shared resources remain alive until shutdown().
     */
    [[nodiscard]] bool releaseCurrentContextResources() override;
    
    // Frame
    void beginFrame(int width, int height, float dpiScale) override;
    void endFrame() override;
    
    // Rendering
    void render(const DrawList& drawList) override;
    
    // White texture for solid color rendering
    uint32_t whiteTexture() const override;
    
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace fst
