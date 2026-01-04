#ifdef _WIN32

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "fastener/platform/window.h"
#include <windows.h>
#include <dwmapi.h>
#include <gl/GL.h>
#include <unordered_map>

// OpenGL types and functions we need
typedef HGLRC (WINAPI *PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int*);
typedef BOOL (WINAPI *PFNWGLSWAPINTERVALEXTPROC)(int);
typedef BOOL (WINAPI *PFNWGLCHOOSEPIXELFORMATARBPROC)(HDC, const int*, const FLOAT*, UINT, int*, UINT*);

#define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB      0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB  0x00000001
#define WGL_DRAW_TO_WINDOW_ARB            0x2001
#define WGL_SUPPORT_OPENGL_ARB            0x2010
#define WGL_DOUBLE_BUFFER_ARB             0x2011
#define WGL_PIXEL_TYPE_ARB                0x2013
#define WGL_TYPE_RGBA_ARB                 0x202B
#define WGL_COLOR_BITS_ARB                0x2014
#define WGL_DEPTH_BITS_ARB                0x2022
#define WGL_STENCIL_BITS_ARB              0x2023
#define WGL_SAMPLE_BUFFERS_ARB            0x2041
#define WGL_SAMPLES_ARB                   0x2042

namespace fst {

//=============================================================================
// Virtual Key Code to Key mapping
//=============================================================================
static Key vkToKey(WPARAM vk, LPARAM lParam) {
    // Extended key flag
    bool extended = (lParam & (1 << 24)) != 0;
    
    switch (vk) {
        case VK_BACK:       return Key::Backspace;
        case VK_TAB:        return Key::Tab;
        case VK_RETURN:     return extended ? Key::KPEnter : Key::Enter;
        case VK_PAUSE:      return Key::Pause;
        case VK_CAPITAL:    return Key::CapsLock;
        case VK_ESCAPE:     return Key::Escape;
        case VK_SPACE:      return Key::Space;
        case VK_PRIOR:      return Key::PageUp;
        case VK_NEXT:       return Key::PageDown;
        case VK_END:        return Key::End;
        case VK_HOME:       return Key::Home;
        case VK_LEFT:       return Key::Left;
        case VK_UP:         return Key::Up;
        case VK_RIGHT:      return Key::Right;
        case VK_DOWN:       return Key::Down;
        case VK_SNAPSHOT:   return Key::PrintScreen;
        case VK_INSERT:     return Key::Insert;
        case VK_DELETE:     return Key::Delete;
        
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            return static_cast<Key>(static_cast<int>(Key::Num0) + (vk - '0'));
        
        case 'A': case 'B': case 'C': case 'D': case 'E':
        case 'F': case 'G': case 'H': case 'I': case 'J':
        case 'K': case 'L': case 'M': case 'N': case 'O':
        case 'P': case 'Q': case 'R': case 'S': case 'T':
        case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
            return static_cast<Key>(static_cast<int>(Key::A) + (vk - 'A'));
        
        case VK_NUMPAD0: case VK_NUMPAD1: case VK_NUMPAD2:
        case VK_NUMPAD3: case VK_NUMPAD4: case VK_NUMPAD5:
        case VK_NUMPAD6: case VK_NUMPAD7: case VK_NUMPAD8:
        case VK_NUMPAD9:
            return static_cast<Key>(static_cast<int>(Key::KP0) + (vk - VK_NUMPAD0));
        
        case VK_MULTIPLY:   return Key::KPMultiply;
        case VK_ADD:        return Key::KPAdd;
        case VK_SUBTRACT:   return Key::KPSubtract;
        case VK_DECIMAL:    return Key::KPDecimal;
        case VK_DIVIDE:     return Key::KPDivide;
        
        case VK_F1:  case VK_F2:  case VK_F3:  case VK_F4:
        case VK_F5:  case VK_F6:  case VK_F7:  case VK_F8:
        case VK_F9:  case VK_F10: case VK_F11: case VK_F12:
            return static_cast<Key>(static_cast<int>(Key::F1) + (vk - VK_F1));
        
        case VK_NUMLOCK:    return Key::NumLock;
        case VK_SCROLL:     return Key::ScrollLock;
        
        case VK_LSHIFT:     return Key::LeftShift;
        case VK_RSHIFT:     return Key::RightShift;
        case VK_LCONTROL:   return Key::LeftControl;
        case VK_RCONTROL:   return Key::RightControl;
        case VK_LMENU:      return Key::LeftAlt;
        case VK_RMENU:      return Key::RightAlt;
        case VK_LWIN:       return Key::LeftSuper;
        case VK_RWIN:       return Key::RightSuper;
        case VK_APPS:       return Key::Menu;
        
        case VK_OEM_1:      return Key::Semicolon;
        case VK_OEM_PLUS:   return Key::Equal;
        case VK_OEM_COMMA:  return Key::Comma;
        case VK_OEM_MINUS:  return Key::Minus;
        case VK_OEM_PERIOD: return Key::Period;
        case VK_OEM_2:      return Key::Slash;
        case VK_OEM_3:      return Key::GraveAccent;
        case VK_OEM_4:      return Key::LeftBracket;
        case VK_OEM_5:      return Key::Backslash;
        case VK_OEM_6:      return Key::RightBracket;
        case VK_OEM_7:      return Key::Apostrophe;
        
        default:            return Key::Unknown;
    }
}

//=============================================================================
// Window Implementation
//=============================================================================
struct Window::Impl {
    HWND hwnd = nullptr;
    HDC hdc = nullptr;
    HGLRC hglrc = nullptr;
    
    bool isOpen = false;
    bool isMinimized = false;
    bool isMaximized = false;
    bool isFocused = true;
    
    int width = 0;
    int height = 0;
    int fbWidth = 0;
    int fbHeight = 0;
    float dpiScale = 1.0f;
    
    InputState inputState;
    
    // Callbacks
    Window::ResizeCallback resizeCallback;
    Window::CloseCallback closeCallback;
    Window::FocusCallback focusCallback;
    
    // Cursor
    HCURSOR cursors[10] = {};
    Cursor currentCursor = Cursor::Arrow;
    bool cursorVisible = true;
    
    // WGL functions
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;
    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = nullptr;
    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = nullptr;
    
    void loadWGLExtensions();
    bool createGLContext(int msaaSamples, bool vsync);
    void updateDPI();
    void updateModifiers();
    
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

// Global window map for WndProc
static std::unordered_map<HWND, Window::Impl*> g_windowMap;

void Window::Impl::loadWGLExtensions() {
    // Create dummy window to get WGL extensions
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"FastenerDummy";
    RegisterClassExW(&wc);
    
    HWND dummyHwnd = CreateWindowExW(
        0, L"FastenerDummy", L"", WS_OVERLAPPED,
        0, 0, 1, 1, nullptr, nullptr, wc.hInstance, nullptr
    );
    
    HDC dummyDC = GetDC(dummyHwnd);
    
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    
    int format = ChoosePixelFormat(dummyDC, &pfd);
    SetPixelFormat(dummyDC, format, &pfd);
    
    HGLRC dummyRC = wglCreateContext(dummyDC);
    wglMakeCurrent(dummyDC, dummyRC);
    
    wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)
        wglGetProcAddress("wglCreateContextAttribsARB");
    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)
        wglGetProcAddress("wglSwapIntervalEXT");
    wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)
        wglGetProcAddress("wglChoosePixelFormatARB");
    
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(dummyRC);
    ReleaseDC(dummyHwnd, dummyDC);
    DestroyWindow(dummyHwnd);
    UnregisterClassW(L"FastenerDummy", wc.hInstance);
}

bool Window::Impl::createGLContext(int msaaSamples, bool vsync) {
    if (wglChoosePixelFormatARB && msaaSamples > 0) {
        int attribs[] = {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
            WGL_COLOR_BITS_ARB, 32,
            WGL_DEPTH_BITS_ARB, 24,
            WGL_STENCIL_BITS_ARB, 8,
            WGL_SAMPLE_BUFFERS_ARB, 1,
            WGL_SAMPLES_ARB, msaaSamples,
            0
        };
        
        int format;
        UINT numFormats;
        if (wglChoosePixelFormatARB(hdc, attribs, nullptr, 1, &format, &numFormats) && numFormats > 0) {
            PIXELFORMATDESCRIPTOR pfd;
            DescribePixelFormat(hdc, format, sizeof(pfd), &pfd);
            SetPixelFormat(hdc, format, &pfd);
        }
    } else {
        PIXELFORMATDESCRIPTOR pfd = {};
        pfd.nSize = sizeof(pfd);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cDepthBits = 24;
        pfd.cStencilBits = 8;
        
        int format = ChoosePixelFormat(hdc, &pfd);
        SetPixelFormat(hdc, format, &pfd);
    }
    
    if (wglCreateContextAttribsARB) {
        int attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
            WGL_CONTEXT_MINOR_VERSION_ARB, 3,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            0
        };
        hglrc = wglCreateContextAttribsARB(hdc, nullptr, attribs);
    }
    
    if (!hglrc) {
        hglrc = wglCreateContext(hdc);
    }
    
    if (!hglrc) return false;
    
    wglMakeCurrent(hdc, hglrc);
    
    if (vsync && wglSwapIntervalEXT) {
        wglSwapIntervalEXT(1);
    }
    
    return true;
}

void Window::Impl::updateDPI() {
    // Get DPI
    HDC screen = GetDC(nullptr);
    dpiScale = GetDeviceCaps(screen, LOGPIXELSX) / 96.0f;
    ReleaseDC(nullptr, screen);
    
    // Update framebuffer size
    RECT rect;
    GetClientRect(hwnd, &rect);
    fbWidth = rect.right - rect.left;
    fbHeight = rect.bottom - rect.top;
}

void Window::Impl::updateModifiers() {
    inputState.onModifiersChanged(
        (GetKeyState(VK_SHIFT) & 0x8000) != 0,
        (GetKeyState(VK_CONTROL) & 0x8000) != 0,
        (GetKeyState(VK_MENU) & 0x8000) != 0,
        ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000) != 0
    );
}

LRESULT CALLBACK Window::Impl::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto it = g_windowMap.find(hwnd);
    if (it == g_windowMap.end()) {
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    
    Impl* impl = it->second;
    
    switch (msg) {
        case WM_CLOSE:
            if (impl->closeCallback) {
                impl->closeCallback({});
            }
            impl->isOpen = false;
            return 0;
        
        case WM_SIZE: {
            impl->width = LOWORD(lParam);
            impl->height = HIWORD(lParam);
            impl->isMinimized = (wParam == SIZE_MINIMIZED);
            impl->isMaximized = (wParam == SIZE_MAXIMIZED);
            impl->updateDPI();
            
            if (impl->resizeCallback) {
                impl->resizeCallback({impl->width, impl->height});
            }
            return 0;
        }
        
        case WM_SETFOCUS:
            impl->isFocused = true;
            if (impl->focusCallback) {
                impl->focusCallback({true});
            }
            return 0;
        
        case WM_KILLFOCUS:
            impl->isFocused = false;
            if (impl->focusCallback) {
                impl->focusCallback({false});
            }
            return 0;
        
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
            Key key = vkToKey(wParam, lParam);
            impl->inputState.onKeyDown(key);
            impl->updateModifiers();
            return 0;
        }
        
        case WM_KEYUP:
        case WM_SYSKEYUP: {
            Key key = vkToKey(wParam, lParam);
            impl->inputState.onKeyUp(key);
            impl->updateModifiers();
            return 0;
        }
        
        case WM_CHAR: {
            if (wParam >= 32 && wParam != 127) {
                impl->inputState.onTextInput(static_cast<char32_t>(wParam));
            }
            return 0;
        }
        
        case WM_MOUSEMOVE: {
            float x = static_cast<float>(LOWORD(lParam));
            float y = static_cast<float>(HIWORD(lParam));
            impl->inputState.onMouseMove(x, y);
            return 0;
        }
        
        case WM_LBUTTONDOWN:
            impl->inputState.onMouseDown(MouseButton::Left);
            SetCapture(hwnd);
            return 0;
        
        case WM_LBUTTONUP:
            impl->inputState.onMouseUp(MouseButton::Left);
            ReleaseCapture();
            return 0;
        
        case WM_RBUTTONDOWN:
            impl->inputState.onMouseDown(MouseButton::Right);
            SetCapture(hwnd);
            return 0;
        
        case WM_RBUTTONUP:
            impl->inputState.onMouseUp(MouseButton::Right);
            ReleaseCapture();
            return 0;
        
        case WM_MBUTTONDOWN:
            impl->inputState.onMouseDown(MouseButton::Middle);
            SetCapture(hwnd);
            return 0;
        
        case WM_MBUTTONUP:
            impl->inputState.onMouseUp(MouseButton::Middle);
            ReleaseCapture();
            return 0;
        
        case WM_MOUSEWHEEL: {
            float delta = GET_WHEEL_DELTA_WPARAM(wParam) / 120.0f;
            impl->inputState.onMouseScroll(0, delta);
            return 0;
        }
        
        case WM_MOUSEHWHEEL: {
            float delta = GET_WHEEL_DELTA_WPARAM(wParam) / 120.0f;
            impl->inputState.onMouseScroll(delta, 0);
            return 0;
        }
        
        case WM_SETCURSOR:
            if (LOWORD(lParam) == HTCLIENT) {
                SetCursor(impl->cursors[static_cast<int>(impl->currentCursor)]);
                return TRUE;
            }
            break;
    }
    
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

//=============================================================================
// Window Public API
//=============================================================================
Window::Window() : m_impl(std::make_unique<Impl>()) {}

Window::Window(const std::string& title, int width, int height) 
    : m_impl(std::make_unique<Impl>()) 
{
    WindowConfig config;
    config.title = title;
    config.width = width;
    config.height = height;
    create(config);
}

Window::Window(const WindowConfig& config) 
    : m_impl(std::make_unique<Impl>()) 
{
    create(config);
}

Window::~Window() {
    destroy();
}

Window::Window(Window&& other) noexcept = default;
Window& Window::operator=(Window&& other) noexcept = default;

bool Window::create(const WindowConfig& config) {
    if (m_impl->isOpen) {
        destroy();
    }
    
    // Load WGL extensions
    m_impl->loadWGLExtensions();
    
    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = Impl::WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"FastenerWindow";
    RegisterClassExW(&wc);
    
    // Convert title to wide string
    int titleLen = MultiByteToWideChar(CP_UTF8, 0, config.title.c_str(), -1, nullptr, 0);
    std::wstring wideTitle(titleLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, config.title.c_str(), -1, wideTitle.data(), titleLen);
    
    // Calculate window size
    DWORD style = WS_OVERLAPPEDWINDOW;
    if (!config.resizable) {
        style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    }
    if (!config.decorated) {
        style = WS_POPUP;
    }
    
    RECT rect = {0, 0, config.width, config.height};
    AdjustWindowRect(&rect, style, FALSE);
    
    // Create window
    m_impl->hwnd = CreateWindowExW(
        0,
        L"FastenerWindow",
        wideTitle.c_str(),
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr, nullptr,
        GetModuleHandleW(nullptr),
        nullptr
    );
    
    if (!m_impl->hwnd) {
        return false;
    }
    
    g_windowMap[m_impl->hwnd] = m_impl.get();
    
    m_impl->hdc = GetDC(m_impl->hwnd);
    
    // Create OpenGL context
    if (!m_impl->createGLContext(config.msaaSamples, config.vsync)) {
        destroy();
        return false;
    }
    
    // Load cursors
    m_impl->cursors[static_cast<int>(Cursor::Arrow)] = LoadCursorW(nullptr, IDC_ARROW);
    m_impl->cursors[static_cast<int>(Cursor::IBeam)] = LoadCursorW(nullptr, IDC_IBEAM);
    m_impl->cursors[static_cast<int>(Cursor::Hand)] = LoadCursorW(nullptr, IDC_HAND);
    m_impl->cursors[static_cast<int>(Cursor::ResizeH)] = LoadCursorW(nullptr, IDC_SIZEWE);
    m_impl->cursors[static_cast<int>(Cursor::ResizeV)] = LoadCursorW(nullptr, IDC_SIZENS);
    m_impl->cursors[static_cast<int>(Cursor::ResizeNESW)] = LoadCursorW(nullptr, IDC_SIZENESW);
    m_impl->cursors[static_cast<int>(Cursor::ResizeNWSE)] = LoadCursorW(nullptr, IDC_SIZENWSE);
    m_impl->cursors[static_cast<int>(Cursor::Move)] = LoadCursorW(nullptr, IDC_SIZEALL);
    m_impl->cursors[static_cast<int>(Cursor::NotAllowed)] = LoadCursorW(nullptr, IDC_NO);
    m_impl->cursors[static_cast<int>(Cursor::Wait)] = LoadCursorW(nullptr, IDC_WAIT);
    
    // Get initial size
    RECT clientRect;
    GetClientRect(m_impl->hwnd, &clientRect);
    m_impl->width = clientRect.right;
    m_impl->height = clientRect.bottom;
    m_impl->updateDPI();
    
    // Show window
    if (config.maximized) {
        ShowWindow(m_impl->hwnd, SW_SHOWMAXIMIZED);
    } else {
        ShowWindow(m_impl->hwnd, SW_SHOW);
    }
    
    m_impl->isOpen = true;
    
    return true;
}

void Window::destroy() {
    if (m_impl->hglrc) {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(m_impl->hglrc);
        m_impl->hglrc = nullptr;
    }
    
    if (m_impl->hdc) {
        ReleaseDC(m_impl->hwnd, m_impl->hdc);
        m_impl->hdc = nullptr;
    }
    
    if (m_impl->hwnd) {
        g_windowMap.erase(m_impl->hwnd);
        DestroyWindow(m_impl->hwnd);
        m_impl->hwnd = nullptr;
    }
    
    m_impl->isOpen = false;
}

bool Window::isOpen() const {
    return m_impl->isOpen;
}

void Window::close() {
    m_impl->isOpen = false;
}

void Window::pollEvents() {
    m_impl->inputState.beginFrame();
    
    MSG msg;
    while (PeekMessageW(&msg, m_impl->hwnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void Window::waitEvents() {
    m_impl->inputState.beginFrame();
    
    MSG msg;
    GetMessageW(&msg, m_impl->hwnd, 0, 0);
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
    
    while (PeekMessageW(&msg, m_impl->hwnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void Window::swapBuffers() {
    SwapBuffers(m_impl->hdc);
}

void Window::makeContextCurrent() {
    wglMakeCurrent(m_impl->hdc, m_impl->hglrc);
}

Vec2 Window::size() const {
    return {static_cast<float>(m_impl->width), static_cast<float>(m_impl->height)};
}

Vec2 Window::framebufferSize() const {
    return {static_cast<float>(m_impl->fbWidth), static_cast<float>(m_impl->fbHeight)};
}

float Window::dpiScale() const {
    return m_impl->dpiScale;
}

int Window::width() const {
    return m_impl->width;
}

int Window::height() const {
    return m_impl->height;
}

void Window::setTitle(const std::string& title) {
    int len = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, nullptr, 0);
    std::wstring wideTitle(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, wideTitle.data(), len);
    SetWindowTextW(m_impl->hwnd, wideTitle.c_str());
}

void Window::setSize(int width, int height) {
    RECT rect = {0, 0, width, height};
    AdjustWindowRect(&rect, GetWindowLong(m_impl->hwnd, GWL_STYLE), FALSE);
    SetWindowPos(m_impl->hwnd, nullptr, 0, 0, 
                 rect.right - rect.left, rect.bottom - rect.top,
                 SWP_NOMOVE | SWP_NOZORDER);
}

void Window::setMinSize(int minWidth, int minHeight) {
    // TODO: Implement via WM_GETMINMAXINFO
}

void Window::setMaxSize(int maxWidth, int maxHeight) {
    // TODO: Implement via WM_GETMINMAXINFO
}

void Window::setPosition(int x, int y) {
    SetWindowPos(m_impl->hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void Window::minimize() {
    ShowWindow(m_impl->hwnd, SW_MINIMIZE);
}

void Window::maximize() {
    ShowWindow(m_impl->hwnd, SW_MAXIMIZE);
}

void Window::restore() {
    ShowWindow(m_impl->hwnd, SW_RESTORE);
}

void Window::focus() {
    SetForegroundWindow(m_impl->hwnd);
    SetFocus(m_impl->hwnd);
}

bool Window::isMinimized() const {
    return m_impl->isMinimized;
}

bool Window::isMaximized() const {
    return m_impl->isMaximized;
}

bool Window::isFocused() const {
    return m_impl->isFocused;
}

void Window::setCursor(Cursor cursor) {
    m_impl->currentCursor = cursor;
}

void Window::hideCursor() {
    if (m_impl->cursorVisible) {
        ShowCursor(FALSE);
        m_impl->cursorVisible = false;
    }
}

void Window::showCursor() {
    if (!m_impl->cursorVisible) {
        ShowCursor(TRUE);
        m_impl->cursorVisible = true;
    }
}

std::string Window::getClipboardText() const {
    if (!OpenClipboard(m_impl->hwnd)) return "";
    
    std::string result;
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData) {
        wchar_t* wstr = static_cast<wchar_t*>(GlobalLock(hData));
        if (wstr) {
            int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
            result.resize(len - 1);
            WideCharToMultiByte(CP_UTF8, 0, wstr, -1, result.data(), len, nullptr, nullptr);
            GlobalUnlock(hData);
        }
    }
    
    CloseClipboard();
    return result;
}

void Window::setClipboardText(const std::string& text) {
    if (!OpenClipboard(m_impl->hwnd)) return;
    
    int len = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len * sizeof(wchar_t));
    if (hMem) {
        wchar_t* wstr = static_cast<wchar_t*>(GlobalLock(hMem));
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wstr, len);
        GlobalUnlock(hMem);
        
        EmptyClipboard();
        SetClipboardData(CF_UNICODETEXT, hMem);
    }
    
    CloseClipboard();
}

void Window::setResizeCallback(ResizeCallback callback) {
    m_impl->resizeCallback = std::move(callback);
}

void Window::setCloseCallback(CloseCallback callback) {
    m_impl->closeCallback = std::move(callback);
}

void Window::setFocusCallback(FocusCallback callback) {
    m_impl->focusCallback = std::move(callback);
}

InputState& Window::input() {
    return m_impl->inputState;
}

const InputState& Window::input() const {
    return m_impl->inputState;
}

void* Window::nativeHandle() const {
    return m_impl->hwnd;
}

} // namespace fst

#endif // _WIN32
