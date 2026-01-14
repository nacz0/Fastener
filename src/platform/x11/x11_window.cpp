/**
 * @file x11_window.cpp
 * @brief X11/Linux implementation of the Window class.
 * 
 * This file provides the Linux platform backend for Fastener windows using
 * native X11 APIs and GLX for OpenGL context management.
 */

#if defined(__linux__) && !defined(__ANDROID__)

#include "fastener/platform/window.h"
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include <unistd.h>
#include <unordered_map>
#include <cstring>

namespace fst {

//=============================================================================
// X11 Key Code to Key mapping
//=============================================================================
static Key xkeyToKey(KeySym keysym) {
    switch (keysym) {
        case XK_BackSpace:      return Key::Backspace;
        case XK_Tab:            return Key::Tab;
        case XK_Return:         return Key::Enter;
        case XK_KP_Enter:       return Key::KPEnter;
        case XK_Pause:          return Key::Pause;
        case XK_Caps_Lock:      return Key::CapsLock;
        case XK_Escape:         return Key::Escape;
        case XK_space:          return Key::Space;
        case XK_Page_Up:        return Key::PageUp;
        case XK_Page_Down:      return Key::PageDown;
        case XK_End:            return Key::End;
        case XK_Home:           return Key::Home;
        case XK_Left:           return Key::Left;
        case XK_Up:             return Key::Up;
        case XK_Right:          return Key::Right;
        case XK_Down:           return Key::Down;
        case XK_Print:          return Key::PrintScreen;
        case XK_Insert:         return Key::Insert;
        case XK_Delete:         return Key::Delete;
        
        case XK_0: case XK_1: case XK_2: case XK_3: case XK_4:
        case XK_5: case XK_6: case XK_7: case XK_8: case XK_9:
            return static_cast<Key>(static_cast<int>(Key::Num0) + (keysym - XK_0));
        
        case XK_a: case XK_b: case XK_c: case XK_d: case XK_e:
        case XK_f: case XK_g: case XK_h: case XK_i: case XK_j:
        case XK_k: case XK_l: case XK_m: case XK_n: case XK_o:
        case XK_p: case XK_q: case XK_r: case XK_s: case XK_t:
        case XK_u: case XK_v: case XK_w: case XK_x: case XK_y: case XK_z:
            return static_cast<Key>(static_cast<int>(Key::A) + (keysym - XK_a));
        
        case XK_A: case XK_B: case XK_C: case XK_D: case XK_E:
        case XK_F: case XK_G: case XK_H: case XK_I: case XK_J:
        case XK_K: case XK_L: case XK_M: case XK_N: case XK_O:
        case XK_P: case XK_Q: case XK_R: case XK_S: case XK_T:
        case XK_U: case XK_V: case XK_W: case XK_X: case XK_Y: case XK_Z:
            return static_cast<Key>(static_cast<int>(Key::A) + (keysym - XK_A));
        
        case XK_KP_0: case XK_KP_1: case XK_KP_2:
        case XK_KP_3: case XK_KP_4: case XK_KP_5:
        case XK_KP_6: case XK_KP_7: case XK_KP_8: case XK_KP_9:
            return static_cast<Key>(static_cast<int>(Key::KP0) + (keysym - XK_KP_0));
        
        case XK_KP_Multiply:    return Key::KPMultiply;
        case XK_KP_Add:         return Key::KPAdd;
        case XK_KP_Subtract:    return Key::KPSubtract;
        case XK_KP_Decimal:     return Key::KPDecimal;
        case XK_KP_Divide:      return Key::KPDivide;
        
        case XK_F1:  case XK_F2:  case XK_F3:  case XK_F4:
        case XK_F5:  case XK_F6:  case XK_F7:  case XK_F8:
        case XK_F9:  case XK_F10: case XK_F11: case XK_F12:
            return static_cast<Key>(static_cast<int>(Key::F1) + (keysym - XK_F1));
        
        case XK_Num_Lock:       return Key::NumLock;
        case XK_Scroll_Lock:    return Key::ScrollLock;
        
        case XK_Shift_L:        return Key::LeftShift;
        case XK_Shift_R:        return Key::RightShift;
        case XK_Control_L:      return Key::LeftControl;
        case XK_Control_R:      return Key::RightControl;
        case XK_Alt_L:          return Key::LeftAlt;
        case XK_Alt_R:          return Key::RightAlt;
        case XK_Super_L:        return Key::LeftSuper;
        case XK_Super_R:        return Key::RightSuper;
        case XK_Menu:           return Key::Menu;
        
        case XK_semicolon:      return Key::Semicolon;
        case XK_equal:          return Key::Equal;
        case XK_comma:          return Key::Comma;
        case XK_minus:          return Key::Minus;
        case XK_period:         return Key::Period;
        case XK_slash:          return Key::Slash;
        case XK_grave:          return Key::GraveAccent;
        case XK_bracketleft:    return Key::LeftBracket;
        case XK_backslash:      return Key::Backslash;
        case XK_bracketright:   return Key::RightBracket;
        case XK_apostrophe:     return Key::Apostrophe;
        
        default:                return Key::Unknown;
    }
}

//=============================================================================
// GLX Extension function pointers
//=============================================================================
typedef GLXContext (*PFNGLXCREATECONTEXTATTRIBSARBPROC)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
typedef void (*PFNGLXSWAPINTERVALEXTPROC)(Display*, GLXDrawable, int);
typedef int (*PFNGLXSWAPINTERVALMESAPROC)(int);

#define GLX_CONTEXT_MAJOR_VERSION_ARB      0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB      0x2092
#define GLX_CONTEXT_PROFILE_MASK_ARB       0x9126
#define GLX_CONTEXT_CORE_PROFILE_BIT_ARB   0x00000001

//=============================================================================
// Window Implementation
//=============================================================================
struct Window::Impl {
    Display* display = nullptr;
    ::Window window = 0;
    GLXContext glxContext = nullptr;
    Colormap colormap = 0;
    XIM xim = nullptr;
    XIC xic = nullptr;
    
    bool isOpen = false;
    bool isMinimized = false;
    bool isMaximized = false;
    bool isFocused = true;
    
    int width = 0;
    int height = 0;
    int fbWidth = 0;
    int fbHeight = 0;
    float dpiScale = 1.0f;
    int posX = 0;
    int posY = 0;
    
    InputState inputState;
    
    // Callbacks
    Window::ResizeCallback resizeCallback;
    Window::CloseCallback closeCallback;
    Window::FocusCallback focusCallback;
    Window::RefreshCallback refreshCallback;
    Window::FileDropCallback fileDropCallback;
    
    // Dropped files
    std::vector<std::string> droppedFiles;
    
    // Cursors
    ::Cursor cursors[10] = {};
    fst::Cursor currentCursor = fst::Cursor::Arrow;
    bool cursorVisible = true;
    
    // Atoms for window manager communication
    Atom wmDeleteWindow;
    Atom wmProtocols;
    Atom netWmState;
    Atom netWmStateMaximizedVert;
    Atom netWmStateMaximizedHorz;
    Atom clipboard;
    Atom utf8String;
    Atom targets;
    
    // GLX functions
    PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = nullptr;
    PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT = nullptr;
    PFNGLXSWAPINTERVALMESAPROC glXSwapIntervalMESA = nullptr;
    
    void loadGLXExtensions();
    bool createGLContext(int msaaSamples, bool vsync, GLXContext shareContext);
    void updateDPI();
    void updateModifiers(unsigned int state);
    void initAtoms();
    void initCursors();
};

// Global window map for event dispatch
static std::unordered_map<::Window, Window::Impl*> g_windowMap;

void Window::Impl::loadGLXExtensions() {
    glXCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC)
        glXGetProcAddressARB((const GLubyte*)"glXCreateContextAttribsARB");
    glXSwapIntervalEXT = (PFNGLXSWAPINTERVALEXTPROC)
        glXGetProcAddressARB((const GLubyte*)"glXSwapIntervalEXT");
    glXSwapIntervalMESA = (PFNGLXSWAPINTERVALMESAPROC)
        glXGetProcAddressARB((const GLubyte*)"glXSwapIntervalMESA");
}

bool Window::Impl::createGLContext(int msaaSamples, bool vsync, GLXContext shareContext) {
    // Choose visual with GLX
    int visualAttribs[] = {
        GLX_X_RENDERABLE,   True,
        GLX_DRAWABLE_TYPE,  GLX_WINDOW_BIT,
        GLX_RENDER_TYPE,    GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE,  GLX_TRUE_COLOR,
        GLX_RED_SIZE,       8,
        GLX_GREEN_SIZE,     8,
        GLX_BLUE_SIZE,      8,
        GLX_ALPHA_SIZE,     8,
        GLX_DEPTH_SIZE,     24,
        GLX_STENCIL_SIZE,   8,
        GLX_DOUBLEBUFFER,   True,
        GLX_SAMPLE_BUFFERS, msaaSamples > 0 ? 1 : 0,
        GLX_SAMPLES,        msaaSamples,
        None
    };
    
    int fbCount;
    GLXFBConfig* fbConfigs = glXChooseFBConfig(display, DefaultScreen(display), visualAttribs, &fbCount);
    if (!fbConfigs || fbCount == 0) {
        // Try without MSAA
        visualAttribs[22] = 0; // GLX_SAMPLE_BUFFERS = 0
        visualAttribs[24] = 0; // GLX_SAMPLES = 0
        fbConfigs = glXChooseFBConfig(display, DefaultScreen(display), visualAttribs, &fbCount);
        if (!fbConfigs || fbCount == 0) {
            return false;
        }
    }
    
    GLXFBConfig fbConfig = fbConfigs[0];
    XFree(fbConfigs);
    
    XVisualInfo* vi = glXGetVisualFromFBConfig(display, fbConfig);
    if (!vi) return false;
    
    // Create colormap
    colormap = XCreateColormap(display, RootWindow(display, vi->screen), vi->visual, AllocNone);
    
    // Set window attributes
    XSetWindowAttributes swa;
    swa.colormap = colormap;
    swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | 
                     ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
                     StructureNotifyMask | FocusChangeMask | EnterWindowMask | LeaveWindowMask;
    
    // Create window
    window = XCreateWindow(display, RootWindow(display, vi->screen),
                          0, 0, width, height, 0,
                          vi->depth, InputOutput, vi->visual,
                          CWColormap | CWEventMask, &swa);
    
    XFree(vi);
    
    if (!window) return false;
    
    // Create GLX context
    if (glXCreateContextAttribsARB) {
        int contextAttribs[] = {
            GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
            GLX_CONTEXT_MINOR_VERSION_ARB, 3,
            GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
            None
        };
        glxContext = glXCreateContextAttribsARB(display, fbConfig, shareContext, True, contextAttribs);
    }
    
    if (!glxContext) {
        // Fallback to legacy context
        glxContext = glXCreateNewContext(display, fbConfig, GLX_RGBA_TYPE, shareContext, True);
    }
    
    if (!glxContext) return false;
    
    glXMakeCurrent(display, window, glxContext);
    
    // Set vsync
    if (vsync) {
        if (glXSwapIntervalEXT) {
            glXSwapIntervalEXT(display, window, 1);
        } else if (glXSwapIntervalMESA) {
            glXSwapIntervalMESA(1);
        }
    }
    
    return true;
}

void Window::Impl::updateDPI() {
    // Try to get DPI from X resources
    char* resourceString = XResourceManagerString(display);
    if (resourceString) {
        XrmDatabase db = XrmGetStringDatabase(resourceString);
        if (db) {
            char* type = nullptr;
            XrmValue value;
            if (XrmGetResource(db, "Xft.dpi", "Xft.Dpi", &type, &value)) {
                if (type && strcmp(type, "String") == 0) {
                    dpiScale = atof(value.addr) / 96.0f;
                }
            }
            XrmDestroyDatabase(db);
        }
    }
    
    // Fallback: assume 96 DPI
    if (dpiScale <= 0) dpiScale = 1.0f;
    
    fbWidth = width;
    fbHeight = height;
}

void Window::Impl::updateModifiers(unsigned int state) {
    inputState.onModifiersChanged(
        (state & ShiftMask) != 0,
        (state & ControlMask) != 0,
        (state & Mod1Mask) != 0,   // Alt
        (state & Mod4Mask) != 0    // Super
    );
}

void Window::Impl::initAtoms() {
    wmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", False);
    wmProtocols = XInternAtom(display, "WM_PROTOCOLS", False);
    netWmState = XInternAtom(display, "_NET_WM_STATE", False);
    netWmStateMaximizedVert = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
    netWmStateMaximizedHorz = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    clipboard = XInternAtom(display, "CLIPBOARD", False);
    utf8String = XInternAtom(display, "UTF8_STRING", False);
    targets = XInternAtom(display, "TARGETS", False);
}

void Window::Impl::initCursors() {
    cursors[static_cast<int>(fst::Cursor::Arrow)] = XCreateFontCursor(display, XC_left_ptr);
    cursors[static_cast<int>(fst::Cursor::IBeam)] = XCreateFontCursor(display, XC_xterm);
    cursors[static_cast<int>(fst::Cursor::Hand)] = XCreateFontCursor(display, XC_hand2);
    cursors[static_cast<int>(fst::Cursor::ResizeH)] = XCreateFontCursor(display, XC_sb_h_double_arrow);
    cursors[static_cast<int>(fst::Cursor::ResizeV)] = XCreateFontCursor(display, XC_sb_v_double_arrow);
    cursors[static_cast<int>(fst::Cursor::ResizeNESW)] = XCreateFontCursor(display, XC_bottom_left_corner);
    cursors[static_cast<int>(fst::Cursor::ResizeNWSE)] = XCreateFontCursor(display, XC_bottom_right_corner);
    cursors[static_cast<int>(fst::Cursor::Move)] = XCreateFontCursor(display, XC_fleur);
    cursors[static_cast<int>(fst::Cursor::NotAllowed)] = XCreateFontCursor(display, XC_X_cursor);
    cursors[static_cast<int>(fst::Cursor::Wait)] = XCreateFontCursor(display, XC_watch);
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
    
    // Open display
    m_impl->display = XOpenDisplay(nullptr);
    if (!m_impl->display) {
        return false;
    }

    XrmInitialize();
    
    m_impl->width = config.width;
    m_impl->height = config.height;
    
    // Initialize
    m_impl->initAtoms();
    m_impl->loadGLXExtensions();
    
    // Create GL context and window
    if (!m_impl->createGLContext(config.msaaSamples, config.vsync, nullptr)) {
        XCloseDisplay(m_impl->display);
        m_impl->display = nullptr;
        return false;
    }
    
    g_windowMap[m_impl->window] = m_impl.get();
    
    // Set window title
    XStoreName(m_impl->display, m_impl->window, config.title.c_str());
    
    // Register for WM_DELETE_WINDOW
    XSetWMProtocols(m_impl->display, m_impl->window, &m_impl->wmDeleteWindow, 1);
    
    // Initialize cursors
    m_impl->initCursors();
    
    // Create input method for text input
    m_impl->xim = XOpenIM(m_impl->display, nullptr, nullptr, nullptr);
    if (m_impl->xim) {
        m_impl->xic = XCreateIC(m_impl->xim,
                                XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
                                XNClientWindow, m_impl->window,
                                nullptr);
    }
    
    m_impl->updateDPI();
    
    // Show window
    if (config.maximized) {
        maximize();
    }
    XMapWindow(m_impl->display, m_impl->window);
    XFlush(m_impl->display);
    
    m_impl->isOpen = true;
    
    return true;
}

bool Window::createWithSharedContext(const WindowConfig& config, Window* shareWindow) {
    if (m_impl->isOpen) {
        destroy();
    }
    
    if (!shareWindow || !shareWindow->m_impl->display) {
        return create(config);
    }
    
    // Use same display as share window
    m_impl->display = shareWindow->m_impl->display;
    m_impl->width = config.width;
    m_impl->height = config.height;
    
    // Initialize atoms (using same display)
    m_impl->initAtoms();
    m_impl->loadGLXExtensions();
    
    // Create GL context with sharing
    if (!m_impl->createGLContext(config.msaaSamples, config.vsync, shareWindow->m_impl->glxContext)) {
        m_impl->display = nullptr;
        return false;
    }
    
    g_windowMap[m_impl->window] = m_impl.get();
    
    XStoreName(m_impl->display, m_impl->window, config.title.c_str());
    XSetWMProtocols(m_impl->display, m_impl->window, &m_impl->wmDeleteWindow, 1);
    m_impl->initCursors();
    
    m_impl->xim = XOpenIM(m_impl->display, nullptr, nullptr, nullptr);
    if (m_impl->xim) {
        m_impl->xic = XCreateIC(m_impl->xim,
                                XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
                                XNClientWindow, m_impl->window,
                                nullptr);
    }
    
    m_impl->updateDPI();
    
    if (config.maximized) {
        maximize();
    }
    XMapWindow(m_impl->display, m_impl->window);
    XFlush(m_impl->display);
    
    m_impl->isOpen = true;
    
    return true;
}

void Window::destroy() {
    if (m_impl->xic) {
        XDestroyIC(m_impl->xic);
        m_impl->xic = nullptr;
    }
    if (m_impl->xim) {
        XCloseIM(m_impl->xim);
        m_impl->xim = nullptr;
    }
    
    // Free cursors
    for (int i = 0; i < 10; ++i) {
        if (m_impl->cursors[i]) {
            XFreeCursor(m_impl->display, m_impl->cursors[i]);
            m_impl->cursors[i] = 0;
        }
    }
    
    if (m_impl->glxContext) {
        glXMakeCurrent(m_impl->display, None, nullptr);
        glXDestroyContext(m_impl->display, m_impl->glxContext);
        m_impl->glxContext = nullptr;
    }
    
    if (m_impl->window) {
        g_windowMap.erase(m_impl->window);
        XDestroyWindow(m_impl->display, m_impl->window);
        m_impl->window = 0;
    }
    
    if (m_impl->colormap) {
        XFreeColormap(m_impl->display, m_impl->colormap);
        m_impl->colormap = 0;
    }
    
    if (m_impl->display) {
        XCloseDisplay(m_impl->display);
        m_impl->display = nullptr;
    }
    
    m_impl->isOpen = false;
}

bool Window::isOpen() const {
    return m_impl->isOpen;
}

void Window::close() {
    if (m_impl->window) {
        XUnmapWindow(m_impl->display, m_impl->window);
    }
    m_impl->isOpen = false;
}

void Window::pollEvents() {
    m_impl->inputState.beginFrame();
    
    while (XPending(m_impl->display) > 0) {
        XEvent event;
        XNextEvent(m_impl->display, &event);
        
        // Filter for input method
        if (m_impl->xic && XFilterEvent(&event, m_impl->window)) {
            continue;
        }
        
        switch (event.type) {
            case ClientMessage:
                if (static_cast<Atom>(event.xclient.data.l[0]) == m_impl->wmDeleteWindow) {
                    if (m_impl->closeCallback) {
                        m_impl->closeCallback({});
                    }
                    m_impl->isOpen = false;
                }
                break;
                
            case ConfigureNotify:
                if (event.xconfigure.width != m_impl->width || event.xconfigure.height != m_impl->height) {
                    m_impl->width = event.xconfigure.width;
                    m_impl->height = event.xconfigure.height;
                    m_impl->updateDPI();
                    if (m_impl->resizeCallback) {
                        m_impl->resizeCallback({m_impl->width, m_impl->height});
                    }
                }
                m_impl->posX = event.xconfigure.x;
                m_impl->posY = event.xconfigure.y;
                break;
                
            case FocusIn:
                m_impl->isFocused = true;
                if (m_impl->focusCallback) {
                    m_impl->focusCallback({true});
                }
                break;
                
            case FocusOut:
                m_impl->isFocused = false;
                if (m_impl->focusCallback) {
                    m_impl->focusCallback({false});
                }
                break;
                
            case KeyPress: {
                KeySym keysym;
                char buffer[32];
                int count = 0;
                
                if (m_impl->xic) {
                    Status status;
                    count = Xutf8LookupString(m_impl->xic, &event.xkey, buffer, sizeof(buffer) - 1, &keysym, &status);
                } else {
                    count = XLookupString(&event.xkey, buffer, sizeof(buffer) - 1, &keysym, nullptr);
                }
                
                Key key = xkeyToKey(keysym);
                m_impl->inputState.onKeyDown(key);
                m_impl->updateModifiers(event.xkey.state);
                
                // Text input
                if (count > 0) {
                    buffer[count] = '\0';
                    // Convert UTF-8 to char32_t (simplified - handles ASCII + basic multibyte)
                    const unsigned char* p = reinterpret_cast<const unsigned char*>(buffer);
                    while (*p) {
                        char32_t codepoint;
                        if ((*p & 0x80) == 0) {
                            codepoint = *p++;
                        } else if ((*p & 0xE0) == 0xC0) {
                            codepoint = (*p++ & 0x1F) << 6;
                            codepoint |= (*p++ & 0x3F);
                        } else if ((*p & 0xF0) == 0xE0) {
                            codepoint = (*p++ & 0x0F) << 12;
                            codepoint |= (*p++ & 0x3F) << 6;
                            codepoint |= (*p++ & 0x3F);
                        } else if ((*p & 0xF8) == 0xF0) {
                            codepoint = (*p++ & 0x07) << 18;
                            codepoint |= (*p++ & 0x3F) << 12;
                            codepoint |= (*p++ & 0x3F) << 6;
                            codepoint |= (*p++ & 0x3F);
                        } else {
                            ++p;
                            continue;
                        }
                        if (codepoint >= 32 && codepoint != 127) {
                            m_impl->inputState.onTextInput(codepoint);
                        }
                    }
                }
                break;
            }
                
            case KeyRelease: {
                // Check for key repeat
                if (XEventsQueued(m_impl->display, QueuedAfterReading)) {
                    XEvent next;
                    XPeekEvent(m_impl->display, &next);
                    if (next.type == KeyPress && 
                        next.xkey.time == event.xkey.time &&
                        next.xkey.keycode == event.xkey.keycode) {
                        // Key repeat - skip the release
                        break;
                    }
                }
                
                KeySym keysym = XLookupKeysym(&event.xkey, 0);
                Key key = xkeyToKey(keysym);
                m_impl->inputState.onKeyUp(key);
                m_impl->updateModifiers(event.xkey.state);
                break;
            }
                
            case MotionNotify:
                m_impl->inputState.onMouseMove(
                    static_cast<float>(event.xmotion.x),
                    static_cast<float>(event.xmotion.y)
                );
                break;
                
            case ButtonPress:
                switch (event.xbutton.button) {
                    case Button1:
                        m_impl->inputState.onMouseDown(MouseButton::Left);
                        break;
                    case Button2:
                        m_impl->inputState.onMouseDown(MouseButton::Middle);
                        break;
                    case Button3:
                        m_impl->inputState.onMouseDown(MouseButton::Right);
                        break;
                    case Button4: // Scroll up
                        m_impl->inputState.onMouseScroll(0, 1);
                        break;
                    case Button5: // Scroll down
                        m_impl->inputState.onMouseScroll(0, -1);
                        break;
                    case 6: // Scroll left (horizontal)
                        m_impl->inputState.onMouseScroll(-1, 0);
                        break;
                    case 7: // Scroll right (horizontal)
                        m_impl->inputState.onMouseScroll(1, 0);
                        break;
                }
                break;
                
            case ButtonRelease:
                switch (event.xbutton.button) {
                    case Button1:
                        m_impl->inputState.onMouseUp(MouseButton::Left);
                        break;
                    case Button2:
                        m_impl->inputState.onMouseUp(MouseButton::Middle);
                        break;
                    case Button3:
                        m_impl->inputState.onMouseUp(MouseButton::Right);
                        break;
                }
                break;
                
            case Expose:
                if (m_impl->refreshCallback && event.xexpose.count == 0) {
                    m_impl->refreshCallback();
                }
                break;
        }
    }
}

void Window::waitEvents() {
    m_impl->inputState.beginFrame();
    
    XEvent event;
    XNextEvent(m_impl->display, &event);
    XPutBackEvent(m_impl->display, &event);
    
    pollEvents();
}

void Window::swapBuffers() {
    glXSwapBuffers(m_impl->display, m_impl->window);
}

void Window::makeContextCurrent() {
    glXMakeCurrent(m_impl->display, m_impl->window, m_impl->glxContext);
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
    XStoreName(m_impl->display, m_impl->window, title.c_str());
}

void Window::setSize(int width, int height) {
    XResizeWindow(m_impl->display, m_impl->window, width, height);
}

void Window::setMinSize(int minWidth, int minHeight) {
    XSizeHints hints;
    hints.flags = PMinSize;
    hints.min_width = minWidth;
    hints.min_height = minHeight;
    XSetWMNormalHints(m_impl->display, m_impl->window, &hints);
}

void Window::setMaxSize(int maxWidth, int maxHeight) {
    XSizeHints hints;
    hints.flags = PMaxSize;
    hints.max_width = maxWidth;
    hints.max_height = maxHeight;
    XSetWMNormalHints(m_impl->display, m_impl->window, &hints);
}

void Window::setPosition(int x, int y) {
    XMoveWindow(m_impl->display, m_impl->window, x, y);
}

void Window::minimize() {
    XIconifyWindow(m_impl->display, m_impl->window, DefaultScreen(m_impl->display));
    m_impl->isMinimized = true;
}

void Window::maximize() {
    XEvent event = {};
    event.type = ClientMessage;
    event.xclient.window = m_impl->window;
    event.xclient.message_type = m_impl->netWmState;
    event.xclient.format = 32;
    event.xclient.data.l[0] = 1; // _NET_WM_STATE_ADD
    event.xclient.data.l[1] = m_impl->netWmStateMaximizedHorz;
    event.xclient.data.l[2] = m_impl->netWmStateMaximizedVert;
    
    XSendEvent(m_impl->display, DefaultRootWindow(m_impl->display), False,
               SubstructureRedirectMask | SubstructureNotifyMask, &event);
    m_impl->isMaximized = true;
}

void Window::restore() {
    XEvent event = {};
    event.type = ClientMessage;
    event.xclient.window = m_impl->window;
    event.xclient.message_type = m_impl->netWmState;
    event.xclient.format = 32;
    event.xclient.data.l[0] = 0; // _NET_WM_STATE_REMOVE
    event.xclient.data.l[1] = m_impl->netWmStateMaximizedHorz;
    event.xclient.data.l[2] = m_impl->netWmStateMaximizedVert;
    
    XSendEvent(m_impl->display, DefaultRootWindow(m_impl->display), False,
               SubstructureRedirectMask | SubstructureNotifyMask, &event);
    
    XMapWindow(m_impl->display, m_impl->window);
    m_impl->isMaximized = false;
    m_impl->isMinimized = false;
}

void Window::focus() {
    XRaiseWindow(m_impl->display, m_impl->window);
    XSetInputFocus(m_impl->display, m_impl->window, RevertToParent, CurrentTime);
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

void Window::setCursor(fst::Cursor cursor) {
    m_impl->currentCursor = cursor;
    if (m_impl->cursorVisible) {
        XDefineCursor(m_impl->display, m_impl->window, m_impl->cursors[static_cast<int>(cursor)]);
    }
}

void Window::hideCursor() {
    if (m_impl->cursorVisible) {
        // Create invisible cursor
        Pixmap pixmap = XCreatePixmap(m_impl->display, m_impl->window, 1, 1, 1);
        XColor color = {};
        ::Cursor invisible = XCreatePixmapCursor(m_impl->display, pixmap, pixmap, &color, &color, 0, 0);
        XDefineCursor(m_impl->display, m_impl->window, invisible);
        XFreeCursor(m_impl->display, invisible);
        XFreePixmap(m_impl->display, pixmap);
        m_impl->cursorVisible = false;
    }
}

void Window::showCursor() {
    if (!m_impl->cursorVisible) {
        XDefineCursor(m_impl->display, m_impl->window, m_impl->cursors[static_cast<int>(m_impl->currentCursor)]);
        m_impl->cursorVisible = true;
    }
}

std::string Window::getClipboardText() const {
    // Request clipboard data
    ::Window owner = XGetSelectionOwner(m_impl->display, m_impl->clipboard);
    if (owner == None) {
        // Try PRIMARY selection
        owner = XGetSelectionOwner(m_impl->display, XA_PRIMARY);
    }
    if (owner == None) return "";
    
    Atom property = XInternAtom(m_impl->display, "FST_CLIPBOARD", False);
    XConvertSelection(m_impl->display, m_impl->clipboard, m_impl->utf8String, property, m_impl->window, CurrentTime);
    XFlush(m_impl->display);
    
    // Wait for SelectionNotify
    XEvent event;
    for (int i = 0; i < 100; ++i) {
        if (XCheckTypedWindowEvent(m_impl->display, m_impl->window, SelectionNotify, &event)) {
            if (event.xselection.property == None) return "";
            
            Atom actualType;
            int actualFormat;
            unsigned long nitems, bytesAfter;
            unsigned char* data = nullptr;
            
            XGetWindowProperty(m_impl->display, m_impl->window, property,
                              0, (~0L), True, AnyPropertyType,
                              &actualType, &actualFormat, &nitems, &bytesAfter, &data);
            
            std::string result;
            if (data) {
                result = reinterpret_cast<char*>(data);
                XFree(data);
            }
            return result;
        }
        usleep(1000); // 1ms
    }
    
    return "";
}

void Window::setClipboardText(const std::string& text) {
    // Store text locally and claim ownership
    static std::string s_clipboardText;
    s_clipboardText = text;
    
    XSetSelectionOwner(m_impl->display, m_impl->clipboard, m_impl->window, CurrentTime);
    XFlush(m_impl->display);
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

void Window::setRefreshCallback(RefreshCallback callback) {
    m_impl->refreshCallback = std::move(callback);
}

void Window::setFileDropCallback(FileDropCallback callback) {
    m_impl->fileDropCallback = std::move(callback);
}

const std::vector<std::string>& Window::droppedFiles() const {
    return m_impl->droppedFiles;
}

void Window::clearDroppedFiles() {
    m_impl->droppedFiles.clear();
}

InputState& Window::input() {
    return m_impl->inputState;
}

const InputState& Window::input() const {
    return m_impl->inputState;
}

Vec2 Window::screenPosition() const {
    return {static_cast<float>(m_impl->posX), static_cast<float>(m_impl->posY)};
}

Vec2 Window::position() const {
    return screenPosition();
}

void* Window::glContext() const {
    return m_impl->glxContext;
}

void* Window::nativeHandle() const {
    return reinterpret_cast<void*>(m_impl->window);
}

} // namespace fst

#endif // __linux__
