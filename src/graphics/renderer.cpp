#include "fastener/graphics/renderer.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/core/log.h"

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <GL/glx.h>
#endif
#include <GL/gl.h>
#include <unordered_map>

// OpenGL 3.3 function types and constants
typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_STREAM_DRAW                    0x88E0
#define GL_TEXTURE0                       0x84C0
#define GL_CLAMP_TO_EDGE                  0x812F
#ifndef GL_TEXTURE_WRAP_S
#define GL_TEXTURE_WRAP_S                 0x2802
#endif
#ifndef GL_TEXTURE_WRAP_T
#define GL_TEXTURE_WRAP_T                 0x2803
#endif
#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE                    0x809D
#endif

// Function pointer types
typedef void (APIENTRY *PFNGLATTACHSHADERPROC)(GLuint, GLuint);
typedef void (APIENTRY *PFNGLBINDBUFFERPROC)(GLenum, GLuint);
typedef void (APIENTRY *PFNGLBINDVERTEXARRAYPROC)(GLuint);
typedef void (APIENTRY *PFNGLBUFFERDATAPROC)(GLenum, GLsizeiptr, const void*, GLenum);
typedef void (APIENTRY *PFNGLCOMPILESHADERPROC)(GLuint);
typedef GLuint (APIENTRY *PFNGLCREATEPROGRAMPROC)(void);
typedef GLuint (APIENTRY *PFNGLCREATESHADERPROC)(GLenum);
typedef void (APIENTRY *PFNGLDELETEBUFFERSPROC)(GLsizei, const GLuint*);
typedef void (APIENTRY *PFNGLDELETEPROGRAMPROC)(GLuint);
typedef void (APIENTRY *PFNGLDELETESHADERPROC)(GLuint);
typedef void (APIENTRY *PFNGLDELETEVERTEXARRAYSPROC)(GLsizei, const GLuint*);
typedef void (APIENTRY *PFNGLDETACHSHADERPROC)(GLuint, GLuint);
typedef void (APIENTRY *PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint);
typedef void (APIENTRY *PFNGLGENBUFFERSPROC)(GLsizei, GLuint*);
typedef void (APIENTRY *PFNGLGENVERTEXARRAYSPROC)(GLsizei, GLuint*);
typedef GLint (APIENTRY *PFNGLGETATTRIBLOCATIONPROC)(GLuint, const GLchar*);
typedef void (APIENTRY *PFNGLGETPROGRAMIVPROC)(GLuint, GLenum, GLint*);
typedef void (APIENTRY *PFNGLGETPROGRAMINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void (APIENTRY *PFNGLGETSHADERIVPROC)(GLuint, GLenum, GLint*);
typedef void (APIENTRY *PFNGLGETSHADERINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef GLint (APIENTRY *PFNGLGETUNIFORMLOCATIONPROC)(GLuint, const GLchar*);
typedef void (APIENTRY *PFNGLLINKPROGRAMPROC)(GLuint);
typedef void (APIENTRY *PFNGLSHADERSOURCEPROC)(GLuint, GLsizei, const GLchar* const*, const GLint*);
typedef void (APIENTRY *PFNGLUNIFORM1IPROC)(GLint, GLint);
typedef void (APIENTRY *PFNGLUNIFORM1FPROC)(GLint, GLfloat);
typedef void (APIENTRY *PFNGLUNIFORM2FPROC)(GLint, GLfloat, GLfloat);
typedef void (APIENTRY *PFNGLUNIFORMMATRIX4FVPROC)(GLint, GLsizei, GLboolean, const GLfloat*);
typedef void (APIENTRY *PFNGLUSEPROGRAMPROC)(GLuint);
typedef void (APIENTRY *PFNGLVERTEXATTRIBPOINTERPROC)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
typedef void (APIENTRY *PFNGLACTIVETEXTUREPROC)(GLenum);

namespace fst {

namespace {

bool hasCurrentGLContext() {
#ifdef _WIN32
    return wglGetCurrentContext() != nullptr;
#elif defined(__linux__)
    return glXGetCurrentContext() != nullptr;
#else
    return true;
#endif
}

void* currentGLContextHandle() {
#ifdef _WIN32
    return reinterpret_cast<void*>(wglGetCurrentContext());
#elif defined(__linux__)
    return reinterpret_cast<void*>(glXGetCurrentContext());
#else
    return nullptr;
#endif
}

} // namespace

struct Renderer::Impl {
    GLuint shaderProgram = 0;
    GLuint blurShaderProgram = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLuint whiteTexture = 0;
    GLuint screenTexture = 0;
    std::unordered_map<void*, GLuint> vaoByContext;
    
    GLint locPosition = -1;
    GLint locTexCoord = -1;
    GLint locColor = -1;
    GLint locProjection = -1;
    GLint locTexture = -1;
    
    GLint locBlurProjection = -1;
    GLint locBlurTexture = -1;
    GLint locBlurScreenSize = -1;
    GLint locBlurRadius = -1;
    GLint locBlurRectPos = -1;
    GLint locBlurRectSize = -1;
    GLint locBlurCornerRadius = -1;
    
    int viewportWidth = 0;
    int viewportHeight = 0;
    float dpiScale = 1.0f;
    int screenTexWidth = 0;
    int screenTexHeight = 0;
    
    // OpenGL function pointers
    PFNGLATTACHSHADERPROC glAttachShader;
    PFNGLBINDBUFFERPROC glBindBuffer;
    PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
    PFNGLBUFFERDATAPROC glBufferData;
    PFNGLCOMPILESHADERPROC glCompileShader;
    PFNGLCREATEPROGRAMPROC glCreateProgram;
    PFNGLCREATESHADERPROC glCreateShader;
    PFNGLDELETEBUFFERSPROC glDeleteBuffers;
    PFNGLDELETEPROGRAMPROC glDeleteProgram;
    PFNGLDELETESHADERPROC glDeleteShader;
    PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
    PFNGLDETACHSHADERPROC glDetachShader;
    PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
    PFNGLGENBUFFERSPROC glGenBuffers;
    PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
    PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
    PFNGLGETPROGRAMIVPROC glGetProgramiv;
    PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
    PFNGLGETSHADERIVPROC glGetShaderiv;
    PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
    PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
    PFNGLLINKPROGRAMPROC glLinkProgram;
    PFNGLSHADERSOURCEPROC glShaderSource;
    PFNGLUNIFORM1IPROC glUniform1i;
    PFNGLUNIFORM1FPROC glUniform1f;
    PFNGLUNIFORM2FPROC glUniform2f;
    PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
    PFNGLUSEPROGRAMPROC glUseProgram;
    PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
    PFNGLACTIVETEXTUREPROC glActiveTexture;
    
    bool loadFunctions();
    bool createShader();
    bool createBlurShader();
    void createWhiteTexture();
    void ensureScreenTexture(int width, int height);
    void setupVao(GLuint vao);
    void ensureVaoForCurrentContext();
};

bool Renderer::Impl::loadFunctions() {
#ifdef _WIN32
    #define LOAD_GL(name) name = (decltype(name))wglGetProcAddress(#name); if (!name) return false
#elif defined(__linux__)
    #define LOAD_GL(name) name = (decltype(name))glXGetProcAddressARB((const GLubyte*)#name); if (!name) return false
#else
    #define LOAD_GL(name) // TODO: Other platforms
#endif
    
    LOAD_GL(glAttachShader);
    LOAD_GL(glBindBuffer);
    LOAD_GL(glBindVertexArray);
    LOAD_GL(glBufferData);
    LOAD_GL(glCompileShader);
    LOAD_GL(glCreateProgram);
    LOAD_GL(glCreateShader);
    LOAD_GL(glDeleteBuffers);
    LOAD_GL(glDeleteProgram);
    LOAD_GL(glDeleteShader);
    LOAD_GL(glDeleteVertexArrays);
    LOAD_GL(glDetachShader);
    LOAD_GL(glEnableVertexAttribArray);
    LOAD_GL(glGenBuffers);
    LOAD_GL(glGenVertexArrays);
    LOAD_GL(glGetAttribLocation);
    LOAD_GL(glGetProgramiv);
    LOAD_GL(glGetProgramInfoLog);
    LOAD_GL(glGetShaderiv);
    LOAD_GL(glGetShaderInfoLog);
    LOAD_GL(glGetUniformLocation);
    LOAD_GL(glLinkProgram);
    LOAD_GL(glShaderSource);
    LOAD_GL(glUniform1i);
    LOAD_GL(glUniform1f);
    LOAD_GL(glUniform2f);
    LOAD_GL(glUniformMatrix4fv);
    LOAD_GL(glUseProgram);
    LOAD_GL(glVertexAttribPointer);
    LOAD_GL(glActiveTexture);
    
    #undef LOAD_GL
    return true;
}

bool Renderer::Impl::createShader() {
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;
        layout (location = 2) in vec4 aColor;
        
        out vec2 TexCoord;
        out vec4 Color;
        
        uniform mat4 uProjection;
        
        void main() {
            gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
            TexCoord = aTexCoord;
            Color = aColor;
        }
    )";
    
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec2 TexCoord;
        in vec4 Color;
        
        out vec4 FragColor;
        
        uniform sampler2D uTexture;
        
        void main() {
            vec4 texColor = texture(uTexture, TexCoord);
            FragColor = Color * texColor;
        }
    )";
    
    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        FST_LOG_ERROR("Vertex shader compilation failed");
        glDeleteShader(vertexShader);
        return false;
    }
    
    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        FST_LOG_ERROR("Fragment shader compilation failed");
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }
    
    // Link program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    
    glDetachShader(shaderProgram, vertexShader);
    glDetachShader(shaderProgram, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    if (!success) {
        FST_LOG_ERROR("Shader program linking failed");
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
        return false;
    }
    
    // Get uniform locations
    locProjection = glGetUniformLocation(shaderProgram, "uProjection");
    locTexture = glGetUniformLocation(shaderProgram, "uTexture");
    
    return true;
}

bool Renderer::Impl::createBlurShader() {
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;
        layout (location = 2) in vec4 aColor;
        
        out vec2 FragPos;
        out vec4 Color;
        
        uniform mat4 uProjection;
        
        void main() {
            gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
            FragPos = aPos;
            Color = aColor;
        }
    )";
    
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec2 FragPos;
        in vec4 Color;
        
        out vec4 FragColor;
        
        uniform sampler2D uTexture;
        uniform vec2 uScreenSize;
        uniform float uBlurRadius;
        uniform vec2 uRectPos;
        uniform vec2 uRectSize;
        uniform float uCornerRadius;
        
        void main() {
            vec2 screenUv = vec2(
                FragPos.x / uScreenSize.x,
                1.0 - (FragPos.y / uScreenSize.y)
            );
            
            vec2 step = uBlurRadius / uScreenSize;
            vec4 sum = vec4(0.0);
            sum += texture(uTexture, screenUv + step * vec2(-1.0, -1.0));
            sum += texture(uTexture, screenUv + step * vec2(0.0, -1.0));
            sum += texture(uTexture, screenUv + step * vec2(1.0, -1.0));
            sum += texture(uTexture, screenUv + step * vec2(-1.0, 0.0));
            sum += texture(uTexture, screenUv);
            sum += texture(uTexture, screenUv + step * vec2(1.0, 0.0));
            sum += texture(uTexture, screenUv + step * vec2(-1.0, 1.0));
            sum += texture(uTexture, screenUv + step * vec2(0.0, 1.0));
            sum += texture(uTexture, screenUv + step * vec2(1.0, 1.0));
            sum *= 1.0 / 9.0;
            
            float alpha = 1.0;
            if (uCornerRadius > 0.0) {
                vec2 halfSize = uRectSize * 0.5;
                vec2 p = FragPos - (uRectPos + halfSize);
                vec2 b = halfSize - vec2(uCornerRadius);
                vec2 q = abs(p) - b;
                float dist = length(max(q, vec2(0.0))) + min(max(q.x, q.y), 0.0) - uCornerRadius;
                float aa = max(fwidth(dist), 0.75);
                alpha = clamp(0.5 - dist / aa, 0.0, 1.0);
                if (alpha <= 0.0) {
                    discard;
                }
            }
            
            FragColor = sum * Color * alpha;
        }
    )";
    
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        FST_LOG_ERROR("Blur vertex shader compilation failed");
        glDeleteShader(vertexShader);
        return false;
    }
    
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        FST_LOG_ERROR("Blur fragment shader compilation failed");
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }
    
    blurShaderProgram = glCreateProgram();
    glAttachShader(blurShaderProgram, vertexShader);
    glAttachShader(blurShaderProgram, fragmentShader);
    glLinkProgram(blurShaderProgram);
    
    glGetProgramiv(blurShaderProgram, GL_LINK_STATUS, &success);
    
    glDetachShader(blurShaderProgram, vertexShader);
    glDetachShader(blurShaderProgram, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    if (!success) {
        FST_LOG_ERROR("Blur shader program linking failed");
        glDeleteProgram(blurShaderProgram);
        blurShaderProgram = 0;
        return false;
    }
    
    locBlurProjection = glGetUniformLocation(blurShaderProgram, "uProjection");
    locBlurTexture = glGetUniformLocation(blurShaderProgram, "uTexture");
    locBlurScreenSize = glGetUniformLocation(blurShaderProgram, "uScreenSize");
    locBlurRadius = glGetUniformLocation(blurShaderProgram, "uBlurRadius");
    locBlurRectPos = glGetUniformLocation(blurShaderProgram, "uRectPos");
    locBlurRectSize = glGetUniformLocation(blurShaderProgram, "uRectSize");
    locBlurCornerRadius = glGetUniformLocation(blurShaderProgram, "uCornerRadius");
    
    return true;
}

void Renderer::Impl::createWhiteTexture() {
    uint32_t white = 0xFFFFFFFF;
    
    glGenTextures(1, &whiteTexture);
    glBindTexture(GL_TEXTURE_2D, whiteTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &white);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::Impl::ensureScreenTexture(int width, int height) {
    if (width <= 0 || height <= 0) return;
    if (screenTexture && screenTexWidth == width && screenTexHeight == height) return;
    
    if (!screenTexture) {
        glGenTextures(1, &screenTexture);
    }
    
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    screenTexWidth = width;
    screenTexHeight = height;
}

void Renderer::Impl::setupVao(GLuint vao) {
    if (!vao) return;

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(DrawVertex),
                          reinterpret_cast<void*>(offsetof(DrawVertex, pos)));

    // TexCoord
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(DrawVertex),
                          reinterpret_cast<void*>(offsetof(DrawVertex, uv)));

    // Color (as normalized unsigned bytes)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(DrawVertex),
                          reinterpret_cast<void*>(offsetof(DrawVertex, color)));

    glBindVertexArray(0);
}

void Renderer::Impl::ensureVaoForCurrentContext() {
    void* ctxHandle = currentGLContextHandle();
    if (!ctxHandle) return;

    auto it = vaoByContext.find(ctxHandle);
    if (it == vaoByContext.end()) {
        GLuint newVao = 0;
        glGenVertexArrays(1, &newVao);
        setupVao(newVao);
        vaoByContext.emplace(ctxHandle, newVao);
        vao = newVao;
    } else {
        vao = it->second;
    }

    if (vao) {
        glBindVertexArray(vao);
    }
}

Renderer::Renderer() : m_impl(std::make_unique<Impl>()) {}

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::init() {
    if (!m_impl->loadFunctions()) return false;
    if (!m_impl->createShader()) return false;
    if (!m_impl->createBlurShader()) return false;
    
    // Create VBO
    m_impl->glGenBuffers(1, &m_impl->vbo);
    m_impl->glBindBuffer(GL_ARRAY_BUFFER, m_impl->vbo);
    
    // Create EBO
    m_impl->glGenBuffers(1, &m_impl->ebo);
    m_impl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_impl->ebo);

    // Create VAO for the current context and configure attributes.
    m_impl->glGenVertexArrays(1, &m_impl->vao);
    m_impl->setupVao(m_impl->vao);
    if (void* ctxHandle = currentGLContextHandle()) {
        m_impl->vaoByContext.emplace(ctxHandle, m_impl->vao);
    }
    
    // Create white texture for solid colors
    m_impl->createWhiteTexture();
    
    return true;
}

void Renderer::shutdown() {
    if (!hasCurrentGLContext()) {
        FST_LOG_WARN("Renderer::shutdown called without a current GL context; skipping GL deletes");
        m_impl->vao = 0;
        m_impl->vbo = 0;
        m_impl->ebo = 0;
        m_impl->shaderProgram = 0;
        m_impl->blurShaderProgram = 0;
        m_impl->whiteTexture = 0;
        m_impl->screenTexture = 0;
        m_impl->screenTexWidth = 0;
        m_impl->screenTexHeight = 0;
        m_impl->vaoByContext.clear();
        return;
    }

    if (m_impl->glDeleteVertexArrays) {
        for (auto& entry : m_impl->vaoByContext) {
            GLuint vao = entry.second;
            if (vao) {
                m_impl->glDeleteVertexArrays(1, &vao);
            }
        }
    }
    m_impl->vaoByContext.clear();
    m_impl->vao = 0;
    if (m_impl->vbo) {
        if (m_impl->glDeleteBuffers) {
            m_impl->glDeleteBuffers(1, &m_impl->vbo);
        }
        m_impl->vbo = 0;
    }
    if (m_impl->ebo) {
        if (m_impl->glDeleteBuffers) {
            m_impl->glDeleteBuffers(1, &m_impl->ebo);
        }
        m_impl->ebo = 0;
    }
    if (m_impl->shaderProgram) {
        if (m_impl->glDeleteProgram) {
            m_impl->glDeleteProgram(m_impl->shaderProgram);
        }
        m_impl->shaderProgram = 0;
    }
    if (m_impl->blurShaderProgram) {
        if (m_impl->glDeleteProgram) {
            m_impl->glDeleteProgram(m_impl->blurShaderProgram);
        }
        m_impl->blurShaderProgram = 0;
    }
    if (m_impl->whiteTexture) {
        glDeleteTextures(1, &m_impl->whiteTexture);
        m_impl->whiteTexture = 0;
    }
    if (m_impl->screenTexture) {
        glDeleteTextures(1, &m_impl->screenTexture);
        m_impl->screenTexture = 0;
        m_impl->screenTexWidth = 0;
        m_impl->screenTexHeight = 0;
    }
}

void Renderer::beginFrame(int width, int height, float dpiScale) {
    m_impl->viewportWidth = width;
    m_impl->viewportHeight = height;
    m_impl->dpiScale = dpiScale;
    m_impl->ensureScreenTexture(width, height);
    
    // Ensure a VAO exists for the current GL context (VAOs are not shared)
    m_impl->ensureVaoForCurrentContext();
    
    // MSAA improves edge smoothing when a multisample framebuffer is available.
    glEnable(GL_MULTISAMPLE);

    glViewport(0, 0, width, height);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::endFrame() {
    // Nothing to do
}

void Renderer::render(const DrawList& drawList) {
    if (drawList.vertices().empty()) return;
    
    // Setup render state
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    
    GLuint activeProgram = 0;
    auto useProgram = [&](GLuint program) {
        if (activeProgram != program) {
            m_impl->glUseProgram(program);
            activeProgram = program;
        }
    };
    
    // Setup projection matrix (orthographic)
    float L = 0.0f;
    float R = static_cast<float>(m_impl->viewportWidth);
    float T = 0.0f;
    float B = static_cast<float>(m_impl->viewportHeight);
    
    float projection[16] = {
        2.0f/(R-L),   0.0f,         0.0f, 0.0f,
        0.0f,         2.0f/(T-B),   0.0f, 0.0f,
        0.0f,         0.0f,        -1.0f, 0.0f,
        (R+L)/(L-R),  (T+B)/(B-T),  0.0f, 1.0f,
    };
    
    useProgram(m_impl->shaderProgram);
    m_impl->glUniformMatrix4fv(m_impl->locProjection, 1, GL_FALSE, projection);
    m_impl->glUniform1i(m_impl->locTexture, 0);
    
    useProgram(m_impl->blurShaderProgram);
    m_impl->glUniformMatrix4fv(m_impl->locBlurProjection, 1, GL_FALSE, projection);
    m_impl->glUniform1i(m_impl->locBlurTexture, 0);
    m_impl->glUniform2f(m_impl->locBlurScreenSize,
                        static_cast<float>(m_impl->viewportWidth),
                        static_cast<float>(m_impl->viewportHeight));
    
    m_impl->glActiveTexture(GL_TEXTURE0);
    
    // Upload vertex data
    m_impl->glBindVertexArray(m_impl->vao);
    m_impl->glBindBuffer(GL_ARRAY_BUFFER, m_impl->vbo);
    m_impl->glBufferData(GL_ARRAY_BUFFER, 
                          drawList.vertices().size() * sizeof(DrawVertex),
                          drawList.vertices().data(), GL_STREAM_DRAW);
    
    m_impl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_impl->ebo);
    m_impl->glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                          drawList.indices().size() * sizeof(uint32_t),
                          drawList.indices().data(), GL_STREAM_DRAW);
    
    // Render commands
    for (const auto& cmd : drawList.commands()) {
        if (cmd.indexCount == 0) continue;
        
        // Set clip rect
        Rect clip = cmd.clipRect;
        glScissor(
            static_cast<int>(clip.x()),
            static_cast<int>(m_impl->viewportHeight - clip.bottom()),
            static_cast<int>(clip.width()),
            static_cast<int>(clip.height())
        );
        
        if (cmd.type == DrawCommandType::Blur) {
            if (m_impl->screenTexture) {
                glBindTexture(GL_TEXTURE_2D, m_impl->screenTexture);
                glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, 
                                    m_impl->viewportWidth, m_impl->viewportHeight);
                
                useProgram(m_impl->blurShaderProgram);
                m_impl->glUniform1f(m_impl->locBlurRadius, cmd.blurRadius);
                m_impl->glUniform2f(m_impl->locBlurRectPos, cmd.rect.x(), cmd.rect.y());
                m_impl->glUniform2f(m_impl->locBlurRectSize, cmd.rect.width(), cmd.rect.height());
                m_impl->glUniform1f(m_impl->locBlurCornerRadius, cmd.rounding);
                
                glBindTexture(GL_TEXTURE_2D, m_impl->screenTexture);
                glDrawElements(GL_TRIANGLES, cmd.indexCount, GL_UNSIGNED_INT, 
                               reinterpret_cast<void*>(cmd.indexOffset * sizeof(uint32_t)));
            }
            continue;
        }
        
        // Bind texture
        useProgram(m_impl->shaderProgram);
        GLuint tex = cmd.textureId ? cmd.textureId : m_impl->whiteTexture;
        glBindTexture(GL_TEXTURE_2D, tex);
        
        // Draw
        glDrawElements(GL_TRIANGLES, cmd.indexCount, GL_UNSIGNED_INT, 
                       reinterpret_cast<void*>(cmd.indexOffset * sizeof(uint32_t)));
    }
    
    // Restore state
    m_impl->glBindVertexArray(0);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
}

uint32_t Renderer::whiteTexture() const {
    return m_impl->whiteTexture;
}

} // namespace fst
