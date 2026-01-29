#include "fastener/graphics/renderer.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/core/log.h"

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <GL/glx.h>
#endif
#include <GL/gl.h>

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

} // namespace

struct Renderer::Impl {
    GLuint shaderProgram = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLuint whiteTexture = 0;
    
    GLint locPosition = -1;
    GLint locTexCoord = -1;
    GLint locColor = -1;
    GLint locProjection = -1;
    GLint locTexture = -1;
    
    int viewportWidth = 0;
    int viewportHeight = 0;
    float dpiScale = 1.0f;
    
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
    PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
    PFNGLUSEPROGRAMPROC glUseProgram;
    PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
    PFNGLACTIVETEXTUREPROC glActiveTexture;
    
    bool loadFunctions();
    bool createShader();
    void createWhiteTexture();
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

void Renderer::Impl::createWhiteTexture() {
    uint32_t white = 0xFFFFFFFF;
    
    glGenTextures(1, &whiteTexture);
    glBindTexture(GL_TEXTURE_2D, whiteTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &white);
    glBindTexture(GL_TEXTURE_2D, 0);
}

Renderer::Renderer() : m_impl(std::make_unique<Impl>()) {}

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::init() {
    if (!m_impl->loadFunctions()) return false;
    if (!m_impl->createShader()) return false;
    
    // Create VAO
    m_impl->glGenVertexArrays(1, &m_impl->vao);
    m_impl->glBindVertexArray(m_impl->vao);
    
    // Create VBO
    m_impl->glGenBuffers(1, &m_impl->vbo);
    m_impl->glBindBuffer(GL_ARRAY_BUFFER, m_impl->vbo);
    
    // Create EBO
    m_impl->glGenBuffers(1, &m_impl->ebo);
    m_impl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_impl->ebo);
    
    // Vertex attributes
    // Position
    m_impl->glEnableVertexAttribArray(0);
    m_impl->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(DrawVertex), 
                                   reinterpret_cast<void*>(offsetof(DrawVertex, pos)));
    
    // TexCoord
    m_impl->glEnableVertexAttribArray(1);
    m_impl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(DrawVertex),
                                   reinterpret_cast<void*>(offsetof(DrawVertex, uv)));
    
    // Color (as normalized unsigned bytes)
    m_impl->glEnableVertexAttribArray(2);
    m_impl->glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(DrawVertex),
                                   reinterpret_cast<void*>(offsetof(DrawVertex, color)));
    
    m_impl->glBindVertexArray(0);
    
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
        m_impl->whiteTexture = 0;
        return;
    }

    if (m_impl->vao) {
        if (m_impl->glDeleteVertexArrays) {
            m_impl->glDeleteVertexArrays(1, &m_impl->vao);
        }
        m_impl->vao = 0;
    }
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
    if (m_impl->whiteTexture) {
        glDeleteTextures(1, &m_impl->whiteTexture);
        m_impl->whiteTexture = 0;
    }
}

void Renderer::beginFrame(int width, int height, float dpiScale) {
    m_impl->viewportWidth = width;
    m_impl->viewportHeight = height;
    m_impl->dpiScale = dpiScale;
    
    // Rebind VAO since it may not be valid in shared contexts
    // VAOs are NOT shared between GL contexts, only textures/buffers/shaders are
    if (m_impl->vao) {
        m_impl->glBindVertexArray(m_impl->vao);
    }
    
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
    
    m_impl->glUseProgram(m_impl->shaderProgram);
    
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
    
    m_impl->glUniformMatrix4fv(m_impl->locProjection, 1, GL_FALSE, projection);
    m_impl->glUniform1i(m_impl->locTexture, 0);
    
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
        
        // Bind texture
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
