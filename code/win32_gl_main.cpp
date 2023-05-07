#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#if 1
// #define _X86_
#define _AMD64_
// #include <minwindef.h>
// #include <winnt.h>
#include <windef.h>
#include <processenv.h>
#include <fileapi.h>

#define FILE_FLAG_WRITE_THROUGH         0x80000000
#define FILE_FLAG_OVERLAPPED            0x40000000
#define FILE_FLAG_NO_BUFFERING          0x20000000
#define FILE_FLAG_RANDOM_ACCESS         0x10000000
#define FILE_FLAG_SEQUENTIAL_SCAN       0x08000000
#define FILE_FLAG_DELETE_ON_CLOSE       0x04000000
#define FILE_FLAG_BACKUP_SEMANTICS      0x02000000
#define FILE_FLAG_POSIX_SEMANTICS       0x01000000
#define FILE_FLAG_SESSION_AWARE         0x00800000
#define FILE_FLAG_OPEN_REPARSE_POINT    0x00200000
#define FILE_FLAG_OPEN_NO_RECALL        0x00100000
#define FILE_FLAG_FIRST_PIPE_INSTANCE   0x00080000

#include <handleapi.h>
#include <errhandlingapi.h>
#include <memoryapi.h>
#include <profileapi.h>
#include <wingdi.h>
#include <winuser.h>
#include <synchapi.h>
#include <intrin.h>
#include <winbase.h>
#else
#include <windows.h>
#endif
#include <windowsx.h>

#include <xaudio2.h>

#include <GL/GL.h>
#include "gl/glext.h"
#include "gl/wglext.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

// #include "imgui.h"
// #include "imgui_impl_win32.h"
// #include "imgui_impl_opengl3.h"

#define _CMATH_
#define _CSTDLIB_
#include <tracy/Tracy.hpp>

#undef assert

#include <maths/maths.h>
#include <utils/misc.h>
#include <utils/logging.h>

// #define platform_big_alloc(memory_size) VirtualAlloc(0, memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
void* platform_big_alloc(size_t memory_size)
{
    void* out = VirtualAlloc(0, memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    assert(out, "VirtualAlloc failed allocating ", memory_size, " bytes, error code = ", (int) GetLastError(),"\n");
    return out;
}
// #define platform_big_alloc(memory_size) malloc(memory_size);

#include "memory.cpp"
#include "context.cpp"

// #include "win32_work_system.h"

struct file_t
{
    char filename[256];
    HANDLE handle;
};

define_printer(DWORD a, ("%li", a));

file_t open_file(char* filename, DWORD disposition)
{
    file_t file;
    memcpy(file.filename, filename, strlen(filename));
    file.handle = CreateFile(filename, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, 0, disposition, FILE_FLAG_SEQUENTIAL_SCAN, 0);
    assert(file.handle != INVALID_HANDLE_VALUE, GetLastError(), ", could not open file: ", filename);
    return file;
}

void close_file(file_t file)
{
    CloseHandle(file.handle);
}

size_t sizeof_file(file_t file)
{
    LARGE_INTEGER file_size;
    auto error = GetFileSizeEx(file.handle, &file_size);
    assert(error, GetLastError(), " getting file size of ", file.filename, "\n");

    return file_size.QuadPart;
}

void read_from_disk(void* data, file_t file, size_t offset, size_t size)
{
    OVERLAPPED overlapped = {
        .Offset = offset,
        .OffsetHigh = offset>>32,
    };
    int error = ReadFile(file.handle, data, size, 0, &overlapped);
}

void write_to_disk(file_t file, size_t offset, void* data, size_t size)
{
    OVERLAPPED overlapped = {
        .Offset = offset,
        .OffsetHigh = offset>>32,
    };
    int error = WriteFile(file.handle, data, size, 0, &overlapped);
}

#define gl_load_operation(rval, ext, args)  typedef rval (APIENTRY * ext##_Func)args; ext##_Func ext = 0;
#include "gl_functions_list.h"

void load_gl_functions()
{
    HMODULE gl_module_handle = GetModuleHandle("opengl32");

    // this is pointer to function which returns pointer to string with list of all wgl extensions
    PFNWGLGETEXTENSIONSSTRINGEXTPROC wglGetExtensionsStringEXT = 0;
    wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC) wglGetProcAddress("wglGetExtensionsStringEXT");

    assert(wglGetExtensionsStringEXT, "could not load wglGetExtensionsStringEXT");
    const char* extensions_list = wglGetExtensionsStringEXT();

    #define gl_check(extension) if(strstr(extensions_list, extension))
    #define gl_load_operation(ret, func, args) func = (CONCAT(func, _Func)) wglGetProcAddress(STR(func)); assert(func, STR(func) " could not be loaded");
    #include "gl_functions_list.h"
}

__inline uint get_cd(char* output)
{
    return GetCurrentDirectory(GetCurrentDirectory(0,0), output);
}

__inline void set_cd(char* directory_name)
{
    SetCurrentDirectory(directory_name);
}

char* load_file_0_terminated(memory_manager* manager, char* filename)
{
    HANDLE file = CreateFile(filename,
                             GENERIC_READ,
                             FILE_SHARE_READ, 0,
                             OPEN_EXISTING,
                             FILE_FLAG_SEQUENTIAL_SCAN, 0);

    if(file == INVALID_HANDLE_VALUE)
    {
        log_error("windows code ", GetLastError(), ", file ", filename, " could not be found\n");
        exit(EXIT_SUCCESS);
    }

    union
    {
        uint64 file_size;
        struct
        {
            DWORD file_size_low;
            DWORD file_size_high;
        };
    };
    file_size_low = GetFileSize(file, &file_size_high);
    if(file_size_low == INVALID_FILE_SIZE)
    {
        log_error(GetLastError(), " opening file\n");
        exit(EXIT_SUCCESS);
    }

    char* output = (char*) stalloc(file_size+1);

    DWORD bytes_read;
    int error = ReadFile(file,
                         output,
                         file_size,
                         &bytes_read,
                         0);
    if(!error)
    {
        log_error("error reading file\n");
        exit(EXIT_SUCCESS);
    }
    CloseHandle(file);

    output[file_size+1] = 0;
    return output;
}

#include "gl_graphics.h"
#include "xaudio2_audio.h"

#include "game.h"

WINDOWPLACEMENT g_wpPrev = { sizeof(g_wpPrev) };

struct window_t
{
    HWND hwnd;
    HGLRC hglrc;

    real_2 size;
    user_input input;

    LARGE_INTEGER timer_frequency;
    LARGE_INTEGER last_time;
    LARGE_INTEGER this_time;
};

void fullscreen(window_t wnd)
{
    DWORD dwStyle = GetWindowLong(wnd.hwnd, GWL_STYLE);
    if (dwStyle & WS_OVERLAPPEDWINDOW) {
        MONITORINFO mi = { sizeof(mi) };
        if (GetWindowPlacement(wnd.hwnd, &g_wpPrev) &&
            GetMonitorInfo(MonitorFromWindow(wnd.hwnd,
                                             MONITOR_DEFAULTTOPRIMARY), &mi)) {
            SetWindowLong(wnd.hwnd, GWL_STYLE,
                          dwStyle & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(wnd.hwnd, HWND_TOP,
                         mi.rcMonitor.left, mi.rcMonitor.top,
                         mi.rcMonitor.right - mi.rcMonitor.left,
                         mi.rcMonitor.bottom - mi.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    } else {
        SetWindowLong(wnd.hwnd, GWL_STYLE,
                      dwStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(wnd.hwnd, &g_wpPrev);
        SetWindowPos(wnd.hwnd, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

#ifdef IMGUI_VERSION
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// There is no distinct VK_xxx for keypad enter, instead it is VK_RETURN + KF_EXTENDED, we assign it an arbitrary value to make code more readable (VK_ codes go up to 255)
#define IM_VK_KEYPAD_ENTER      (VK_RETURN + 256)

// Map VK_xxx to ImGuiKey_xxx.
static ImGuiKey ImGui_ImplWin32_VirtualKeyToImGuiKey(WPARAM wParam)
{
    switch (wParam)
    {
        case VK_TAB: return ImGuiKey_Tab;
        case VK_LEFT: return ImGuiKey_LeftArrow;
        case VK_RIGHT: return ImGuiKey_RightArrow;
        case VK_UP: return ImGuiKey_UpArrow;
        case VK_DOWN: return ImGuiKey_DownArrow;
        case VK_PRIOR: return ImGuiKey_PageUp;
        case VK_NEXT: return ImGuiKey_PageDown;
        case VK_HOME: return ImGuiKey_Home;
        case VK_END: return ImGuiKey_End;
        case VK_INSERT: return ImGuiKey_Insert;
        case VK_DELETE: return ImGuiKey_Delete;
        case VK_BACK: return ImGuiKey_Backspace;
        case VK_SPACE: return ImGuiKey_Space;
        case VK_RETURN: return ImGuiKey_Enter;
        case VK_ESCAPE: return ImGuiKey_Escape;
        case VK_OEM_7: return ImGuiKey_Apostrophe;
        case VK_OEM_COMMA: return ImGuiKey_Comma;
        case VK_OEM_MINUS: return ImGuiKey_Minus;
        case VK_OEM_PERIOD: return ImGuiKey_Period;
        case VK_OEM_2: return ImGuiKey_Slash;
        case VK_OEM_1: return ImGuiKey_Semicolon;
        case VK_OEM_PLUS: return ImGuiKey_Equal;
        case VK_OEM_4: return ImGuiKey_LeftBracket;
        case VK_OEM_5: return ImGuiKey_Backslash;
        case VK_OEM_6: return ImGuiKey_RightBracket;
        case VK_OEM_3: return ImGuiKey_GraveAccent;
        case VK_CAPITAL: return ImGuiKey_CapsLock;
        case VK_SCROLL: return ImGuiKey_ScrollLock;
        case VK_NUMLOCK: return ImGuiKey_NumLock;
        case VK_SNAPSHOT: return ImGuiKey_PrintScreen;
        case VK_PAUSE: return ImGuiKey_Pause;
        case VK_NUMPAD0: return ImGuiKey_Keypad0;
        case VK_NUMPAD1: return ImGuiKey_Keypad1;
        case VK_NUMPAD2: return ImGuiKey_Keypad2;
        case VK_NUMPAD3: return ImGuiKey_Keypad3;
        case VK_NUMPAD4: return ImGuiKey_Keypad4;
        case VK_NUMPAD5: return ImGuiKey_Keypad5;
        case VK_NUMPAD6: return ImGuiKey_Keypad6;
        case VK_NUMPAD7: return ImGuiKey_Keypad7;
        case VK_NUMPAD8: return ImGuiKey_Keypad8;
        case VK_NUMPAD9: return ImGuiKey_Keypad9;
        case VK_DECIMAL: return ImGuiKey_KeypadDecimal;
        case VK_DIVIDE: return ImGuiKey_KeypadDivide;
        case VK_MULTIPLY: return ImGuiKey_KeypadMultiply;
        case VK_SUBTRACT: return ImGuiKey_KeypadSubtract;
        case VK_ADD: return ImGuiKey_KeypadAdd;
        case IM_VK_KEYPAD_ENTER: return ImGuiKey_KeypadEnter;
        case VK_LSHIFT: return ImGuiKey_LeftShift;
        case VK_LCONTROL: return ImGuiKey_LeftCtrl;
        case VK_LMENU: return ImGuiKey_LeftAlt;
        case VK_LWIN: return ImGuiKey_LeftSuper;
        case VK_RSHIFT: return ImGuiKey_RightShift;
        case VK_RCONTROL: return ImGuiKey_RightCtrl;
        case VK_RMENU: return ImGuiKey_RightAlt;
        case VK_RWIN: return ImGuiKey_RightSuper;
        case VK_APPS: return ImGuiKey_Menu;
        case '0': return ImGuiKey_0;
        case '1': return ImGuiKey_1;
        case '2': return ImGuiKey_2;
        case '3': return ImGuiKey_3;
        case '4': return ImGuiKey_4;
        case '5': return ImGuiKey_5;
        case '6': return ImGuiKey_6;
        case '7': return ImGuiKey_7;
        case '8': return ImGuiKey_8;
        case '9': return ImGuiKey_9;
        case 'A': return ImGuiKey_A;
        case 'B': return ImGuiKey_B;
        case 'C': return ImGuiKey_C;
        case 'D': return ImGuiKey_D;
        case 'E': return ImGuiKey_E;
        case 'F': return ImGuiKey_F;
        case 'G': return ImGuiKey_G;
        case 'H': return ImGuiKey_H;
        case 'I': return ImGuiKey_I;
        case 'J': return ImGuiKey_J;
        case 'K': return ImGuiKey_K;
        case 'L': return ImGuiKey_L;
        case 'M': return ImGuiKey_M;
        case 'N': return ImGuiKey_N;
        case 'O': return ImGuiKey_O;
        case 'P': return ImGuiKey_P;
        case 'Q': return ImGuiKey_Q;
        case 'R': return ImGuiKey_R;
        case 'S': return ImGuiKey_S;
        case 'T': return ImGuiKey_T;
        case 'U': return ImGuiKey_U;
        case 'V': return ImGuiKey_V;
        case 'W': return ImGuiKey_W;
        case 'X': return ImGuiKey_X;
        case 'Y': return ImGuiKey_Y;
        case 'Z': return ImGuiKey_Z;
        case VK_F1: return ImGuiKey_F1;
        case VK_F2: return ImGuiKey_F2;
        case VK_F3: return ImGuiKey_F3;
        case VK_F4: return ImGuiKey_F4;
        case VK_F5: return ImGuiKey_F5;
        case VK_F6: return ImGuiKey_F6;
        case VK_F7: return ImGuiKey_F7;
        case VK_F8: return ImGuiKey_F8;
        case VK_F9: return ImGuiKey_F9;
        case VK_F10: return ImGuiKey_F10;
        case VK_F11: return ImGuiKey_F11;
        case VK_F12: return ImGuiKey_F12;
        default: return ImGuiKey_None;
    }
}
#endif

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    #ifdef IMGUI_VERSION
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return true;
    #endif

    switch(msg)
    {
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        // case WM_SIZE:
        // {
        //     // //TODO: set correct context
        //     window_width = LOWORD(lParam);
        //     window_height = HIWORD(lParam);
        //     break;
        // }
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void win32_load_gl_functions(HWND hwnd, HINSTANCE hinstance)
{ //Load GL functions
    int error;

    HDC dummy_dc = GetDC(hwnd);
    assert(dummy_dc, "could not get dummy dc");

    PIXELFORMATDESCRIPTOR pfd =
        {
            .nSize = sizeof(PIXELFORMATDESCRIPTOR),
            .nVersion = 1,
            .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, //Flags
            .iPixelType = PFD_TYPE_RGBA,                                           //The kind of framebuffer. RGBA or palette.
            .cColorBits = 32,                                                      //Colordepth of the framebuffer.
            .cAlphaBits = 8,
            .cDepthBits = 32,                                                      //Number of bits for the depthbuffer
            .cStencilBits = 0,                                                     //Number of bits for the stencilbuffer
            .iLayerType = PFD_MAIN_PLANE,
        };

    int pf = ChoosePixelFormat(dummy_dc, &pfd);
    assert(pf, "could not choose pixel format");
    error = SetPixelFormat(dummy_dc, pf, &pfd);
    assert(error, "could not set pixel format");

    HGLRC dummy_glrc = wglCreateContext(dummy_dc);
    assert(dummy_glrc, "could not create dummy gl context");

    error = wglMakeCurrent(dummy_dc, dummy_glrc);
    assert(error, "could not activate dummy gl context");

    load_gl_functions();

    wglMakeCurrent(0, 0);
    wglDeleteContext(dummy_glrc);
    ReleaseDC(hwnd, dummy_dc);
}

window_t create_window(memory_manager* manager, char* window_title, char* class_name, int width, int height, int x, int y, HINSTANCE hinstance)
{
    int error;
    WNDCLASSEX wc;
    {//init the window class
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.lpfnWndProc = WndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = hinstance;
        wc.hIcon = LoadIcon(0, IDI_APPLICATION);
        wc.hCursor = LoadCursor(0, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
        wc.lpszMenuName = 0;
        wc.lpszClassName = class_name;
        wc.hIconSm = LoadIcon(0, IDI_APPLICATION);

        error = RegisterClassEx(&wc);
        assert(error, "window registration failed");
    }

    HWND hwnd = CreateWindowEx(
        WS_EX_APPWINDOW, //extended window style
        class_name, //the class name
        window_title, //The window title
        WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, //window style
        x, y, //window_position
        width, height, //window dimensions
        0, //handle to the parent window, this has no parents
        0, //menu handle
        hinstance, //duh
        0 //lparam
        );
    assert(hwnd, "window creation failed");

    // TITLEBARINFOEX title_info;
    // title_info.cbSize = sizeof(TITLEBARINFOEX);
    // SendMessage(hwnd, WM_GETTITLEBARINFOEX,0, (LPARAM) &title_info);
    // int title_height = title_info.rcTitleBar.bottom-title_info.rcTitleBar.top;
    // int title_width  = title_info.rcTitleBar.right -title_info.rcTitleBar.left;
    // log_output("title_height = ", title_height, "\n");
    // SetWindowPos(hwnd, HWND_TOP, x, y, width, height+2*title_height, 0);

    RECT rect;
    GetClientRect(hwnd, &rect);
    SetWindowPos(hwnd, HWND_TOP, x, y, 2*width-(rect.right-rect.left), 2*height-(rect.bottom-rect.top), 0);

    win32_load_gl_functions(hwnd, hinstance);

    HGLRC hglrc;
    {//init gl context
        HDC dc = GetDC(hwnd);
        assert(dc, "could not get dc");

        {
            PIXELFORMATDESCRIPTOR pfd =
                {
                    .nSize = sizeof(PIXELFORMATDESCRIPTOR),
                    .nVersion = 1,
                    .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
                    .iPixelType = PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
                    .cColorBits = 32,                        //Colordepth of the framebuffer.
                    .cAlphaBits = 8,
                    .cDepthBits = 32,                        //Number of bits for the depthbuffer
                    .cStencilBits = 0,                        //Number of bits for the stencilbuffer
                    .iLayerType = PFD_MAIN_PLANE,
                };

            int pf = ChoosePixelFormat(dc, &pfd);
            assert(pf, "could not choose pixel format");
            error = SetPixelFormat(dc, pf, &pfd);
            assert(error, "could not set pixel format");
        }

        int attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 4,
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
            0        //End
        };

        hglrc = wglCreateContextAttribsARB(dc, 0, attribs);
        assert(hglrc, "could not create gl context")

        error = wglMakeCurrent(dc, hglrc);
        assert(error, "could not make gl context current")

        log_output("OPENGL VERSION ", (char*)glGetString(GL_VERSION), "\n");
    }

    {
        glEnable(GL_FRAMEBUFFER_SRGB);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glDepthFunc(GL_GREATER);
        glEnable(GL_DEPTH_TEST);

        glDepthRange(-1, 1);

        //vsync
        wglSwapIntervalEXT(0);
    }

    glDebugMessageCallbackARB(gl_error_callback, 0);
    glEnable(GL_DEBUG_OUTPUT);

    gl_init_buffers();
    gl_init_shaders();

    { //create raw input device
        RAWINPUTDEVICE rid[2];
        size_t n_rid = 0;

        //mouse
        rid[n_rid++] = {.usUsagePage = 0x01,
                        .usUsage = 0x02,
                        .dwFlags = RIDEV_NOLEGACY,
                        .hwndTarget = hwnd};

        //keyboard
        rid[n_rid++] = {.usUsagePage = 0x01,
                        .usUsage = 0x06,
                        .dwFlags = RIDEV_NOLEGACY,
                        .hwndTarget = hwnd};

        assert(RegisterRawInputDevices(rid, n_rid, sizeof(rid[0])), "error reading rawinput device");
    }

    LARGE_INTEGER timer_frequency;
    LARGE_INTEGER this_time;
    QueryPerformanceFrequency(&timer_frequency);
    QueryPerformanceCounter(&this_time);

    return {.hwnd = hwnd,
            .hglrc = hglrc,
            .timer_frequency = timer_frequency,
            .last_time = this_time,
            .this_time = this_time};
}

window_t create_window(memory_manager* manager, char* window_title, char* class_name, HINSTANCE hinstance)
{
    return create_window(manager, window_title, class_name, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hinstance);
}

void show_window(window_t wnd)
{
    ShowWindow(wnd.hwnd, SW_NORMAL);
    // UpdateWindow(wnd.hwnd);
}

int update_window(window_t* wnd)
{
    HDC dc = GetDC(wnd->hwnd);
    SwapBuffers(dc);

    memset(wnd->input.pressed_buttons, 0, sizeof(wnd->input.pressed_buttons));
    memset(wnd->input.released_buttons, 0, sizeof(wnd->input.released_buttons));
    wnd->input.buttons[M_WHEEL_DOWN/8] &= ~(11<<(M_WHEEL_DOWN%8));
    wnd->input.mouse_wheel = 0;
    wnd->input.dmouse = zero_2;
    wnd->input.click_blocked = false;

    POINT cursor_point;
    RECT window_rect;
    if(GetActiveWindow() == wnd->hwnd)
    {
        GetWindowRect(wnd->hwnd, &window_rect);
        wnd->size = {(window_rect.right-window_rect.left),
                    (window_rect.bottom-window_rect.top)};
        ClipCursor(&window_rect);
    }

    GetCursorPos(&cursor_point);

    #ifdef IMGUI_VERSION
    ImGuiIO &io = ImGui::GetIO();
    #endif

    MSG msg;
    while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        switch(msg.message)
        {
            case WM_INPUT:
            {
                RAWINPUT raw;
                uint size = sizeof(raw);
                GetRawInputData((HRAWINPUT) msg.lParam, RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER));
                switch(raw.header.dwType)
                {
                    case RIM_TYPEMOUSE:
                    {
                        RAWMOUSE& ms = raw.data.mouse;

                        // if(ms.usButtonFlags & RI_MOUSE_WHEEL)
                        //     printf("0x%x, %d, %d, %d\n",
                        //            ms.usButtonFlags, ms.lLastX, ms.lLastY, ms.usButtonData);

                        if(ms.usFlags == MOUSE_MOVE_RELATIVE)
                        {
                            wnd->input.dmouse += {(4.0*ms.lLastX)/window_height,
                                    (-4.0*ms.lLastY)/window_height};
                        }
                        // if(ms.usFlags&0b11)
                        // {
                        //     wnd->mouse = {(2.0*ms.lLastX-(window_rect.left+window_rect.right))/window_height,
                        //                   (-2.0*ms.lLastY+(window_rect.bottom+window_rect.top))/window_height};
                        // }

                        //usButtonFlags can have more than one input at a time
                        #ifdef IMGUI_VERSION
                        #define update_numbered_button(N)               \
                            if(ms.usButtonFlags & RI_MOUSE_BUTTON_##N##_DOWN) \
                            { wnd->input.pressed_buttons[0] |= 1<<M##N; wnd->input.buttons[0] |= 1<<M##N;    io.AddMouseButtonEvent(N-1,  true); } \
                            else if(ms.usButtonFlags & RI_MOUSE_BUTTON_##N##_UP) \
                            { wnd->input.released_buttons[0] |= 1<<M##N; wnd->input.buttons[0] &= ~(1<<M##N); io.AddMouseButtonEvent(N-1, false); }
                        #else
                        #define update_numbered_button(N)               \
                            if(ms.usButtonFlags & RI_MOUSE_BUTTON_##N##_DOWN) \
                            { wnd->input.pressed_buttons[0] |= 1<<M##N; wnd->input.buttons[0] |= 1<<M##N; } \
                            else if(ms.usButtonFlags & RI_MOUSE_BUTTON_##N##_UP) \
                            { wnd->input.released_buttons[0] |= 1<<M##N; wnd->input.buttons[0] &= ~(1<<M##N); }
                        #endif

                        update_numbered_button(1);
                        update_numbered_button(2);
                        update_numbered_button(3);
                        update_numbered_button(4);
                        update_numbered_button(5);

                        if(ms.usButtonFlags & RI_MOUSE_WHEEL)
                        {
                            wnd->input.mouse_wheel = (short) ms.usButtonData/WHEEL_DELTA;
                            if((short) ms.usButtonData > 0)
                            {
                                wnd->input.buttons[1] |= 1<<(M_WHEEL_UP-8);
                                wnd->input.pressed_buttons[1] |= 1<<(M_WHEEL_UP-8);
                            }
                            else
                            {
                                wnd->input.buttons[1] |= 1<<(M_WHEEL_DOWN-8);
                                wnd->input.pressed_buttons[1] |= 1<<(M_WHEEL_DOWN-8);
                            }
                            #ifdef IMGUI_VERSION
                            io.AddMouseWheelEvent(0.0f, ms.usButtonData);
                            #endif
                        }

                        if(ms.usButtonFlags & RI_MOUSE_HWHEEL)
                        {
                            wnd->input.mouse_hwheel = (short) ms.usButtonData/WHEEL_DELTA;
                            if((short) ms.usButtonData > 0)
                            {
                                wnd->input.buttons[1] |= 1<<(M_WHEEL_RIGHT-8);
                                wnd->input.pressed_buttons[1] |= 1<<(M_WHEEL_RIGHT-8);
                            }
                            else
                            {
                                wnd->input.buttons[1] |= 1<<(M_WHEEL_LEFT-8);
                                wnd->input.pressed_buttons[1] |= 1<<(M_WHEEL_LEFT-8);
                            }
                            #ifdef IMGUI_VERSION
                            io.AddMouseWheelEvent(ms.usButtonData/WHEEL_DELTA, 0.0);
                            #endif
                        }

                        break;
                    }
                    case RIM_TYPEKEYBOARD:
                    {
                        RAWKEYBOARD& kb = raw.data.keyboard;
                        bool keyup = kb.Flags&RI_KEY_BREAK;
                        if(keyup) set_key_up(kb.VKey, wnd->input);
                        else    set_key_down(kb.VKey, wnd->input);

                        #ifdef IMGUI_VERSION
                        ImGuiKey imgui_key = ImGui_ImplWin32_VirtualKeyToImGuiKey(kb.VKey);
                        if(imgui_key != ImGuiKey_None)
                        {
                            io.AddKeyEvent(imgui_key, !keyup);
                        }
                        char c = kb.VKey;
                        if(c == VK_SPACE) c = ' ';
                        else if(c == VK_RETURN) c = '\n';
                        else if('A' <= c && c <= 'Z') {c += 'a'-'A';}
                        else if('0' <= c && c <= '9'){}
                        else c = 0;

                        if(c != 0)
                        {
                            if(is_pressed(kb.VKey, &wnd->input)) io.AddInputCharacter(kb.VKey);
                        }
                        #endif
                        break;
                    }
                }
                break;
            }
            case WM_QUIT:
            {
                return 0;
            }
            default:
                DispatchMessage(&msg);
        }
    }
    // RECT wnd_rect;
    // GetWindowRect(wnd.hwnd, &wnd_rect);

    // auto window_width = wnd_rect.right-wnd_rect.left;
    // auto window_height = wnd_rect.bottom-wnd_rect.top;
    // glViewport(0.5*(window_width-window_height), 0, window_height, window_height);

    // wnd->input.mouse += wnd->input.dmouse;

    wnd->input.mouse = {(2.0*cursor_point.x-(window_rect.left+window_rect.right))/window_height,
        (-2.0*cursor_point.y+(window_rect.bottom+window_rect.top))/window_height};

    // if(io.WantCaptureKeyboard) memset(wnd->input.buttons, 0, sizeof(wnd->input.buttons));
    // if(io.WantCaptureMouse)
    // {
    //     wnd->input.dmouse = {0,0};
    //     ShowCursor(1);
    // }
    // else
    // {
    //     ShowCursor(0);
    // }

    CURSORINFO ci;
    GetCursorInfo(&ci);
    static HCURSOR old_cursor = 0;
    if(debug_menu_active)
    {
        HCURSOR last_curosr = SetCursor(old_cursor);
    }
    else
    {
        HCURSOR last_cursor = SetCursor(0);
        if(last_cursor) old_cursor = last_cursor;
    }

    real gamma = 2.2;
    glClearColor(pow(0.2, gamma), pow(0.0, gamma), pow(0.3, gamma), 1.0);
    glClearDepth(-1.0);
    // glClearColor(0.25, 0.0, 0.35, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return 1;
}

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    current_context = create_context();
    memory_manager* manager = get_context()->manager;

    window_t wnd = create_window(manager, "3D Sand", "3dsand", 1280, 720, 10, 10, hinstance);
    // fullscreen(wnd);
    show_window(wnd);

    int next_id = 1;

    font_info font = init_font(manager, "C:/windows/fonts/arial.ttf", 16);

    init_materials_list();
    init_material_properties_textures();

    const int n_max_bodies = 1024*1024;
    const int n_max_brains = 128*1024;
    const int n_max_genomes = 128*1024;
    world w = {
        .seed = 128924,
        // .player = {.x = {211.1,200.1,32.1}, .x_dot = {0,0,0}},
        // .player = {.x = {289.1,175.1, 302.1}, .x_dot = {0,0,0}},
        .player = {.x = {room_size/2+45.1, room_size/2+25.1, 42.1}, .x_dot = {0,0,0}},

        .body_table = (index_table_entry*) stalloc_clear(sizeof(index_table_entry)*n_max_bodies),
        .n_max_bodies = n_max_bodies,
        .next_body_id = 1,
        //TODO: real max limits
        .bodies_cpu = (cpu_body_data*) stalloc_clear(4096*sizeof(cpu_body_data)),
        .bodies_gpu = (gpu_body_data*) stalloc_clear(4096*sizeof(gpu_body_data)),
        .n_bodies = 0,

        .body_fragments = (int*) stalloc(4096*sizeof(int)),
        .n_body_fragments = 0,

        .joints = (body_joint*) stalloc_clear(4096*sizeof(body_joint)),
        .n_joints = 0,

        .contacts = (contact_point*) stalloc_clear(128*1024*sizeof(contact_point)),
        .n_contacts = 0,

        .brain_table = (index_table_entry*) stalloc_clear(sizeof(index_table_entry)*n_max_brains),
        .n_max_brains = n_max_brains,
        .next_brain_id = 1,
        .brains = (brain*) stalloc_clear(1024*sizeof(brain)),
        .n_brains = 0,

        .genome_table = (index_table_entry*) stalloc_clear(sizeof(index_table_entry)*n_max_genomes),
        .n_max_genomes = n_max_genomes,
        .next_genome_id = 1,
        .genomes = (genome*) stalloc_clear(1024*sizeof(genome)),
        .n_genomes = 0,

        .gew = {
            .x = {-0.9, 0.9},

            .theta = pi/2,
            .phi = 0,
            .camera_dist = 16.0,

            .camera_axes = {
                1, 0, 0,
                0, 1, 0,
                0, 0, 1,
            },
            .camera_pos = {0,0,-10},

            .genodes = (genode*) stalloc_clear(1024*sizeof(genode)),
            .n_genodes = 0,

            .gizmo_x = {.r = 5, .color = {1,0,0,1},},
            .gizmo_y = {.r = 5, .color = {0,1,0,1},},
            .gizmo_z = {.r = 5, .color = {0,0,1,1},},

            .active_cell_material = 128,
        },

        .editor = {
            .object_table = (index_table_entry*) stalloc(sizeof(index_table_entry)*4096),
            .n_max_objects = 4096,
            .next_object_id = 1,
            .objects = (editor_object*) stalloc(sizeof(editor_object)*4096),
            .n_objects = 0,
            .joints = (object_joint*) stalloc(sizeof(object_joint)*4096),
            .n_joints = 0,
            .texture_space = {
                .max_size = {form_texture_size, form_texture_size, form_texture_size},
                .free_regions = (bounding_box*) stalloc(sizeof(bounding_box)*10000),
                .n_free_regions = 0,
            },
            .selected_object = 0,
            .current_selection = {},

            .material = 1,

            .camera_dist = 10,
            .theta = pi/2, .phi = 0,

            .rotate_sensitivity = 1,
            .pan_sensitivity = 2,
            .move_sensitivity = 0.5,
            .zoom_sensitivity = 0.05,

            .rd = {
                .fov = pi*120.0f/180.0f,
                .camera_pos = {},
                .camera_axes = {1,0,0, 0,1,0, 0,0,1},
            },
        },

        .body_space = {
            .max_size = {body_texture_size, body_texture_size, body_texture_size},
            .free_regions = (bounding_box*) stalloc(sizeof(bounding_box)*10000),
            .n_free_regions = 0,
        },

        .explosions = (explosion_data*) stalloc_clear(4096*sizeof(explosion_data)),
        .n_explosions = 0,

        .beams = (beam_data*) stalloc_clear(4096*sizeof(beam_data)),
        .n_beams = 0,

        .rays_in = (ray_in*) stalloc_clear(4096*sizeof(ray_in)),
        .rays_out = (ray_out*) stalloc_clear(4096*sizeof(ray_out)),
        .n_rays = 0,

        .c = (room*) stalloc(8*sizeof(room)),
        .chunk_lookup = {},

        .collision_grid = (collision_cell*) stalloc_clear(sizeof(collision_cell)*collision_cells_per_axis*collision_cells_per_axis*collision_cells_per_axis),
    };

    w.body_space.free_regions[w.body_space.n_free_regions++] = {{0,0,0}, {body_texture_size, body_texture_size, body_texture_size}};

    w.editor.texture_space.free_regions[w.editor.texture_space.n_free_regions++] = {{0,0,0}, {form_texture_size, form_texture_size, form_texture_size}};

    genome* g = create_genome(&w);
    {
        g->form_id_updates = (int*) stalloc(1024*sizeof(int));
        g->forms = (form_t*) stalloc(1024*sizeof(form_t));
        g->n_forms = 0;
        g->joints = (form_joint*) stalloc(1024*sizeof(form_joint));
        g->n_joints = 0;
        g->endpoints = (form_endpoint*) stalloc(1024*sizeof(form_endpoint));
        g->n_endpoints = 0;
        g->cell_types = (cell_type*) stalloc_clear(128*sizeof(cell_type));
        g->n_cell_types = 3;
    }

    for(int i = 0; i < g->n_cell_types; i++)
    {
        g->cell_types[i].mat = materials_list+128+i;
    }

    w.gew.active_genome = &w.genomes[0];

    compile_genome(w.gew.active_genome,  w.gew.genodes);

    int max_3d_texture_size;
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max_3d_texture_size);
    log_output("GL_MAX_3D_TEXTURE_SIZE: ", max_3d_texture_size, "\n");

    room* c = w.c;

    c->materials = (uint16*) stalloc(sizeof(uint16)*room_size*room_size*room_size);

    for(int z = 0; z < room_size; z++)
        for(int y = 0; y < room_size; y++)
            for(int x = 0; x < room_size; x++)
            {
                int water = get_material_index("AQUA");
                int sand = get_material_index("SAND");
                int wall_mat = get_material_index("WALL");
                int light = get_material_index("LAMP");
                int glass = get_material_index("GLSS");

                int material = 0;
                int height = 20;
                real rsq = sq(x-room_size/2)+sq(y-room_size/2);
                // height += 0.03*rsq*(exp(-0.0005*rsq));
                int ramp_height = 20;
                // height += clamp(ramp_height-abs(y-128), 0, ramp_height);
                if(sqrt(rsq) < 80 && z < height+ramp_height-5) material = water;
                if(sqrt(rsq) < 80 && z < 20) material = light;
                height += clamp((float) ramp_height-abs(sqrt(rsq)-80), 0.0, (float) ramp_height);
                height += max((y*(x-400)/room_size), 0);
                if(z < height) material = sand;
                // else if(z < room_size-1) material = (randui(&w.seed)%prob)==0;
                // if(x == room_size/2+0 && y == room_size/2+0) material = 2;
                // if(x == room_size/2+0 && y == room_size/2-1) material = 2;
                // if(x == room_size/2-1 && y == room_size/2-1) material = 2;
                // if(x == room_size/2-1 && y == room_size/2+0) material = 2;

                if(z <= 5) material = wall_mat;
                if(z > room_size-10) material = wall_mat;

                int door_width = 40;
                //outer walls
                if(abs(x-room_size/2) > door_width/2 && abs(y-room_size/2) > door_width/2 ||
                   x <            5 ||
                   x > room_size-5 ||
                   y <            5 ||
                   y > room_size-5)
                {
                    if(y <= 10) material = wall_mat;
                    if(x <= 10) material = wall_mat;
                    if(room_size-y <= 10) material = wall_mat;
                    if(room_size-x <= 10) material = wall_mat;
                    // if(room_size-z <= 10) material = wall_mat;
                }

                int glass_thickness = 3;
                if(x > 400 && y > 400 && z > 100 && z < 200 &&
                   (x < 400+glass_thickness || y < 400+glass_thickness || z < 100+glass_thickness)) material = glass;

                //inner walls
                int wall_thickness = 20;
                if((abs(x-room_size/2) < wall_thickness/2 || abs(y-room_size/2) < wall_thickness/2) &&
                   !(abs(x-room_size*1/4) < door_width/2 || abs(y-room_size*1/4) < door_width/2
                     || abs(x-room_size*3/4) < door_width/2 || abs(y-room_size*3/4) < door_width/2 || x > 400))
                {
                     material = wall_mat;
                }

                if(abs(z-room_size/2) < 5) material = wall_mat;
                if(abs(z-room_size/2) < 5 && z < room_size/2 && x > room_size/2) material = light;

                real spiral_height = 300;
                if(rsq > sq(80) && rsq < sq(120))
                {
                    if(abs((float)(fmod(z-(spiral_height/(2.0*pi))*(pi+atan2(y-room_size/2,x-room_size/2)), spiral_height/2))) < 3)
                        material = wall_mat;
                    else if(z >= height)
                        material = 0;
                }

                if(z == height+10 && x < 50 && y < 50) material = wall_mat;

                // if(y == room_size/2 && abs(x-room_size/2) <= 2*(i) && x%2 == 0) material = wall_mat;
                if(y == room_size*3/4 && x == room_size*3/4+20 && z < 50) material = wall_mat;

                // if(material == 0) material = 2;

                // if(material == 0) material = 2*((randui(&w.seed)%10000)==0); //"rain"
                c->materials[x+room_size*y+room_size*room_size*z] = material;
            }

    load_room_to_gpu(c);

    {
        editor_data* ed = &w.editor;
        editor_object* o = ed->objects+ed->n_objects++;
        int_3 size = {512,512,512};
        int total_size = axes_product(size);
        *o = {
            .id = ed->n_objects-1, //TODO
            .name = "World",
            .region = {{0,0,0},size},
            .materials = dynamic_alloc(total_size),
            .tint = {1,1,1,1},
            .highlight = {},
            .x = ed->camera_center,
            .orientation = {1,0,0,0},
            .gridlocked = false,
            .visible = true,
            .is_world = true,
        };
        o->modified_region = o->region;
        // memset(o->materials, 5, total_size);
        for(int z = 0; z < size.z; z++)
            for(int y = 0; y < size.y; y++)
                for(int x = 0; x < size.x; x++)
                {
                    o->materials[index_3D({x,y,z}, size)] = c->materials[index_3D({x,y,z}, room_size)];
                }
    }


    world_cell* cells = (world_cell*) calloc(room_size*room_size*room_size, sizeof(world_cell));
    rle_run* rle_cells = (rle_run*) calloc(room_size*room_size*room_size, sizeof(rle_run));
    byte* compressed_cells = (byte*) calloc(room_size*room_size*room_size, sizeof(world_cell));
    size_t compressed_size = 0;

    size_t encoded_size = encode_world_cells(rle_cells);
    compressed_size = compress_world_cells(rle_cells, encoded_size, compressed_cells);

    for(int i = 0; i < 3; i++)
    {
        int b = new_body_index(&w);
        int_3 unpadded_size = {12,12,12};
        // int_3 size = unpadded_size+(int_3){4,2,2};
        int_3 size = unpadded_size;
        w.bodies_cpu[b].region = {{}, size};
        w.bodies_cpu[b].materials = (body_cell*) dynamic_alloc(size.x*size.y*size.z*sizeof(body_cell));
        w.bodies_gpu[b].x_cm = 0.5*real_cast(unpadded_size);
        w.bodies_gpu[b].x = {room_size*3/4-5*b, room_size/2, 50};
        w.bodies_gpu[b].x_dot = {0,0,0};
        // w.bodies_gpu[b].omega = {0.0,-0.5,0.0};
        w.bodies_gpu[b].omega = {0.0,0.0,0.0};
        w.bodies_gpu[b].orientation = {1,0,0,0};

        w.bodies_gpu[b].m = 1.0;
        w.bodies_cpu[b].I = real_identity_3(10.0);

        w.bodies_cpu[b].genome_id = 0;

        w.bodies_gpu[b].substantial = true;

        real half_angle = 0.5*(pi/4);
        real_3 axis = {1,0,0};
        quaternion rotation = (quaternion){cos(half_angle), sin(half_angle)*axis.x, sin(half_angle)*axis.y, sin(half_angle)*axis.z};
        // w.b->orientation = w.b->orientation*rotation;

        for(int z = 0; z < unpadded_size.z; z++)
            for(int y = 0; y < unpadded_size.y; y++)
                for(int x = 0; x < unpadded_size.x; x++)
                {
                    bool x_middle = x >= unpadded_size.x/3 && x < unpadded_size.x*2/3;
                    bool y_middle = y >= unpadded_size.x/3 && y < unpadded_size.x*2/3;
                    bool z_middle = z >= unpadded_size.x/3 && z < unpadded_size.x*2/3;

                    // int material = 2;
                    // if((x+y+z)%2==0 && (z <= 0 || z >= w.bodies_gpu[b].unpadded_size.z-1)) material = 1;
                    // if((x+y+z)%2==0 && (x <= 0 || x >= w.bodies_gpu[b].unpadded_size.x-1)) material = 1;
                    // if((x+y+z)%2==0 && (y <= 0 || y >= w.bodies_gpu[b].unpadded_size.y-1)) material = 1;
                    // if(x_middle && y_middle) material = 0;
                    // if(y_middle && z_middle) material = 0;
                    // if(z_middle && x_middle) material = 0;

                    int material = 0;
                    if(x_middle && y_middle) material = 3;
                    if(y_middle && z_middle) material = 3;
                    if(z_middle && x_middle) material = 3;
                    if(material == 3 && (x+y+z)%2==0 && (z <= 0 || z >= unpadded_size.z-1)) material = 1;
                    if(material == 3 && (x+y+z)%2==0 && (x <= 0 || x >= unpadded_size.x-1)) material = 1;
                    if(material == 3 && (x+y+z)%2==0 && (y <= 0 || y >= unpadded_size.y-1)) material = 1;

                    // if(abs(y - 12) > 2 || abs(x - 12) > 2) material = 0;

                    w.bodies_cpu[b].materials[index_3D({x,y,z}, size)] = {.material = material};
                }
    }

    int limb_length = 4;
    int shoulder_length = 6;
    int body_length = 8;
    int body_width = 3;
    int body_depth = 1;
    int limb_thickness = 1;
    real head_size = 5;

    int_3 offsets[10];
    int body_material = 128;

    int limb_start_id = g->n_forms;
    int body_id;
    int head_id;
    for(int i = 0; i < 8; i++) //limbs
    {
        form_t* form = &g->forms[g->n_forms++];

        form->x = {};
        form->orientation = {1,0,0,0};

        form->cell_material_id = 0;

        form->int_orientation = 0;
        form->theta = 2;
        if(i == 4 || i == 6)
        {
            form->theta = 1;
            form->phi = (6+2*i)%8;
        }

        int_3 size = {limb_length,1,1};
        int total_size = axes_product(size);
        form->materials = (uint8*) dynamic_alloc(total_size);
        memset(form->materials, 0, total_size);

        for(int x = 0; x < size.x; x++)
            for(int y = 0; y < size.y; y++)
                for(int z = 0; z < size.z; z++)
                {
                    int m = 128;
                    form->materials[index_3D({x,y,z}, size)] = m;
                }

        form->region = {{}, size};
    }

    head_id = g->n_forms;
    {
        form_t* form = &g->forms[g->n_forms++];

        form->is_root = true;

        form->x = {};
        form->orientation = {1,0,0,0};

        form->cell_material_id = 0;

        form->int_orientation = 0;
        form->theta = 0;

        int_3 size = {head_size,head_size,head_size};
        int total_size = axes_product(size);
        form->materials = (uint8*) dynamic_alloc(total_size);
        memset(form->materials, 0, total_size);

        for(int x = 0; x < head_size; x++)
            for(int y = 0; y < head_size; y++)
                for(int z = 0; z < head_size; z++)
                {
                    int m = 128;
                    form->materials[index_3D({x,y,z}, size)] = m;
                }

        form->region = {{}, size};
    }

    body_id = g->n_forms;
    {
        form_t* form = &g->forms[g->n_forms++];

        form->x = {};
        form->orientation = {1,0,0,0};

        form->cell_material_id = 0;

        form->int_orientation = 0;
        form->theta = -2;

        int_3 size = {body_length,body_width,body_depth};
        int total_size = axes_product(size);
        form->materials = (uint8*) dynamic_alloc(total_size);
        memset(form->materials, 0, total_size);

        for(int x = 0; x < body_length; x++)
            for(int y = 0; y < body_width; y++)
                for(int z = 0; z < body_depth; z++)
                {
                    int material = 0;
                    if(x < shoulder_length || (z == 0 && y == body_width/2)) material = body_material;
                    form->materials[index_3D({x,y,z}, size)] = material;
                }
        form->region = {{}, size};
    }

    {
        int b = new_body_index(&w);
        int_3 size = {20,10,5};
        w.bodies_cpu[b].region = {{}, size};
        w.bodies_cpu[b].materials = (body_cell*) dynamic_alloc(size.x*size.y*size.z*sizeof(body_cell));
        w.bodies_gpu[b].x_cm = 0.5*real_cast(size);
        w.bodies_gpu[b].x = {room_size*3/4-0.5*b, room_size/2, 50};
        w.bodies_gpu[b].x_dot = {0,0,0};
        w.bodies_gpu[b].omega = {0.0,0.0,0.0};
        w.bodies_gpu[b].orientation = {1,0,0,0};

        w.bodies_gpu[b].m = 0.0;
        w.bodies_cpu[b].I = {};

        w.bodies_cpu[b].genome_id = 0;

        w.bodies_gpu[b].substantial = true;

        real half_angle = 0.5*(pi/4);
        real_3 axis = {1,0,0};
        quaternion rotation = (quaternion){cos(half_angle), sin(half_angle)*axis.x, sin(half_angle)*axis.y, sin(half_angle)*axis.z};
        // w.b->orientation = w.b->orientation*rotation;

        for(int z = 0; z < size.z; z++)
            for(int y = 0; y < size.y; y++)
                for(int x = 0; x < size.x; x++)
                {
                    int material = 3;
                    if(x == size.x-1 || y == size.y-1 || z == size.z-1) material = 0;
                    w.bodies_cpu[b].materials[x+y*size.x+z*size.x*size.y] = {.material = material};
                    real m = 0.001;
                    w.bodies_gpu[b].m += m;

                    real_3 r = {x+0.5,y+0.5,z+0.5};
                    r += -w.bodies_gpu[b].x_cm;
                    w.bodies_cpu[b].I += real_identity_3(m*(0.4*sq(0.5)+normsq(r)));
                    for(int i = 0; i < 3; i++)
                        for(int j = 0; j < 3; j++)
                            w.bodies_cpu[b].I[i][j] += -m*r[i]*r[j];
                }

        // for(int i = 0; i < 3; i++)
        // {
        //     for(int j = 0; j < 3; j++)
        //         log_output(w.bodies_cpu[b].I[i][j], " ");
        //     log_output("\n");
        // }
        // log_output("\n");
        // log_output(w.bodies_gpu[b].m, "\n");
        // log_output("\n");
    }

    // w.bodies_cpu[0].is_root = true;
    // w.bodies_cpu[0].joints[w.bodies_cpu[0].n_children++] = {
    //     .type = joint_hinge,
    //     .child_body_id = 1,
    //     .pos = {{4,4,0}, {4,4,0}},
    //     .axis = {0,0},
    //     .max_speed = 0.5,
    //     .max_torque = 0.2,
    // };

    g->joints[g->n_joints++] = {
        .type = joint_ball,
        .form_id = {head_id, body_id},
        .pos = {(int_3){(head_size-1)/2,(head_size-1)/2,0}, (int_3){body_length-1,(body_width-1)/2,0}},
        .axis = {2,0},
    };

    for(int i = 0; i < 4; i++)
    {
        g->joints[g->n_joints++] = {
            .type = joint_ball,
            .form_id = {limb_start_id+2*i, limb_start_id+2*i+1},
            .pos = {(int_3){0+limb_length-1,0,0},(int_3){0,0,0}},
            .axis = {1,1},
        };
    }

    g->joints[g->n_joints++] = {
        .type = joint_ball,
        .form_id = {body_id, limb_start_id+0},
        .pos = {(int_3){0,0,0}, (int_3){0,0,0}},
        .axis = {1,1},
    };

    g->joints[g->n_joints++] = {
        .type = joint_ball,
        .form_id = {body_id, limb_start_id+2},
        .pos = {(int_3){0,body_width-1,0}, (int_3){0,0,0}},
        .axis = {1,1},
    };

    g->joints[g->n_joints++] = {
        .type = joint_ball,
        .form_id = {body_id, limb_start_id+4},
        .pos = {(int_3){shoulder_length-1,0,0}, (int_3){0,0,0}},
        .axis = {1,1},
    };

    g->joints[g->n_joints++] = {
        .type = joint_ball,
        .form_id = {body_id, limb_start_id+6},
        .pos = {(int_3){shoulder_length-1,body_width-1,0}, (int_3){0,0,0}},
        .axis = {1,1},
    };

    brain* br = create_brain(&w);
    {
        br->root_id = head_id;
        br->body_ids = (int*) dynamic_alloc(256*sizeof(int));
        br->n_max_bodies = 256;
        br->endpoints = (endpoint*) stalloc(1024*sizeof(endpoint));
        br->n_endpoints = 0;
        br->genome_id = g->id;
    }

    creature_from_genome(manager, &w, g, br);
    for(int bi = 0; bi < br->n_bodies; bi++)
    {
        int b = get_body_index(&w, br->body_ids[bi]);
        cpu_body_data* body_cpu = w.bodies_cpu+b;
        gpu_body_data* body_gpu = w.bodies_gpu+b;

        uint8_4 materials_data = {128,0,0,0};

        // glBindTexture(GL_TEXTURE_3D, form_materials_texture);
        // glTexSubImage3D(GL_TEXTURE_3D, 0,
        //                 pos.x, pos.y, pos.z,
        //                 1, 1, 1,
        //                 GL_RGBA_INTEGER, GL_UNSIGNED_BYTE,
        //                 &materials_data);

        body_gpu->x_cm = 0.5*real_cast(body_cpu->region.l)+0.5*real_cast(body_cpu->region.u);

        body_gpu->m = 0;
        body_cpu->I = {};
        for(int z = body_cpu->region.l.z; z < body_cpu->region.u.z; z++)
            for(int y = body_cpu->region.l.y; y < body_cpu->region.u.y; y++)
                for(int x = body_cpu->region.l.x; x < body_cpu->region.u.x; x++)
                {
                    real m = 0.001;
                    body_gpu->m += m;

                    real_3 r = {x+0.5,y+0.5,z+0.5};
                    r += -body_gpu->x_cm;
                    body_cpu->I += real_identity_3(m*(0.4*sq(0.5)+normsq(r)));
                    for(int i = 0; i < 3; i++)
                        for(int j = 0; j < 3; j++)
                            body_cpu->I[i][j] += -m*r[i]*r[j];

                    body_cpu->materials[index_3D((int_3){x,y,z}-body_cpu->region.l, dim(body_cpu->region))] = {BASE_CELL_MAT};
                }
        update_inertia(body_cpu, body_gpu);

        body_gpu->substantial = true;
    }

    g->endpoints[g->n_endpoints++] = {
        .type = endpoint_foot,
        .form_id = limb_start_id+1,
        .pos = (int_3){limb_length-1,0,0},
    };

    g->endpoints[g->n_endpoints++] = {
        .type = endpoint_foot,
        .form_id = limb_start_id+3,
        .pos = (int_3){limb_length-1,0,0},
    };

    // //hand feet
    // br->endpoints[br->n_endpoints++] = {
    //     .type = endpoint_foot,
    //     .body_id = 9,
    //     .pos = {limb_length-1,0,0},
    //     .foot_phase = 0.0,
    //     .root_anchor = {shoulder_length-1,0,0},
    //     .root_dist = 2*limb_length,
    // };
    // br->endpoints[br->n_endpoints++] = {
    //     .type = endpoint_foot,
    //     .body_id = 11,
    //     .pos = {limb_length-1,0,0},
    //     .foot_phase = 0.5,
    //     .root_anchor = {shoulder_length-1,body_width-1,0},
    //     .root_dist = 2*limb_length,
    // };

    for(int b = 0; b < w.n_bodies; b++)
    {
        w.bodies_gpu[b].x = {room_size/2+15.1*(1+b), room_size/2+95.1, 42.1};

        w.bodies_gpu[b].old_x = w.bodies_gpu[b].x;
        w.bodies_gpu[b].old_orientation = w.bodies_gpu[b].orientation;
        update_inertia(&w.bodies_cpu[b], &w.bodies_gpu[b]);
    }

    // CreateDirectory(w.tim.savedir, 0);
    // w.entity_savefile = open_file("entities.dat");

    render_data rd = {};
    init_render_data(&rd);

    render_data ui = {
        .background_color = {0.01,0.01,0.01,1},
        .foreground_color = {1,1,1,1},
        .highlight_color = {0.1,0.1,0.1,1},
    };
    init_render_data(&ui);

    init_render_data(&w.editor.rd);

    load_sprites();

    audio_data ad;
    { //create sound device
        HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
        assert(hr == S_OK);

        IXAudio2* xaudio2 = 0;
        hr = XAudio2Create(&xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
        assert(hr == S_OK);

        IXAudio2MasteringVoice* master_voice = 0;
        hr = xaudio2->CreateMasteringVoice(&master_voice, 1, 48000, 0, 0, 0, AudioCategory_GameEffects);
        assert(hr == S_OK);


		WAVEFORMATEX wft = {
			WAVE_FORMAT_PCM, //wFormatTag
			1,               //nChannels
			48000,           //nSamplesPerSec
			96000,           //nAvgBytesPerSec = nSamplesPerSec*nBlockAlign = nSamplesPerSec*nChannels*wBitsPerSample/8
			2,               //nBlockAlign = nChannels*wBitsPerSample/8
			16,              //wBitsPerSample
			0,               //cbSize
		};

        IXAudio2SourceVoice* source_voice = 0;
        hr = xaudio2->CreateSourceVoice(&source_voice, &wft, XAUDIO2_VOICE_NOPITCH, 1.0, 0 /*callback*/, 0, 0);
        assert(hr == S_OK);

        hr = xaudio2->StartEngine();
        assert(hr == S_OK);

        size_t buffer_size = 4*wft.nAvgBytesPerSec;
        ad = (audio_data){
            .buffer = (int16*) stalloc(buffer_size),
            .sample_rate = wft.nSamplesPerSec,
            .source_voice = source_voice,
            .samples_played = 0,

            .noises = (noise*) stalloc(1000*sizeof(noise)),
            .n_noises = 0,
        };
        ad.buffer_length = buffer_size/sizeof(ad.buffer[0]);

        XAUDIO2_BUFFER audio_buffer = {
            .Flags = 0,
            .AudioBytes = buffer_size,
            .pAudioData = (byte*) ad.buffer,
            .PlayBegin = 0,
            .PlayLength = ad.buffer_length,
            .LoopBegin = 0,
            .LoopLength = 0,
            .LoopCount = XAUDIO2_LOOP_INFINITE,
            .pContext = 0
        };

        source_voice->SubmitSourceBuffer(&audio_buffer,0);
        source_voice->Start();

        // for(int i = 0; i < ad.buffer_length; i++)
        // {
        //     ad.buffer[i] = 6000*sin(2000*4*2*pi*i/ad.buffer_length);
        // }
    }

    #ifdef IMGUI_VERSION
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    const char* glsl_version = "#version 460";
    ImGui_ImplWin32_Init(wnd.hwnd);
    ImGui_ImplOpenGL3_Init(glsl_version);

    #endif

    ui.camera = {
        1.0*window_height/window_width, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };

    // work_stack = (work_task*) stalloc(1024*sizeof(work_task));
    // n_work_entries = 0;
    // n_remaining_tasks = 0;
    // work_semephore = CreateSemaphoreA(0, 0, n_max_workers, NULL);
    // for(int i = 0; i < n_max_workers+1; i++)
    // {
    //     DWORD thread_id;
    //     if(i == 0)
    //     {
    //         main_context  = create_thread_context(0);
    //         continue;
    //     }

    //     HANDLE thread_handle = CreateThread(0, 0, thread_proc, (void*) (uint64) i, 0, &thread_id);
    //     CloseHandle(thread_handle);
    // }

    waterfall = load_audio_file(manager, "data/sounds/water_flow.wav");
    jump_sound = load_audio_file(manager, "data/sounds/Meat_impacts_0.wav");
    // play_sound(&ad, &waterfall, 1.0);

    int grid_width = 100;
    int grid_height = 100;
    real grid_size = 10.0;
    int n_ground_circles = (grid_width+1)*(grid_height+1);
    circle_render_info* ground_circles = (circle_render_info*) stalloc(sizeof(circle_render_info)*n_ground_circles);

    for(int x = 0; x <= grid_width; x++)
    {
        for(int y = 0; y <= grid_height; y++)
        {
            ground_circles[x+y*(grid_width+1)] = {
                // .x = {(x-0.5*grid_width-0.5)*grid_size, (y-0.5*grid_height-0.5)*grid_size, 0.0},
                .x = {(x-0.5)*grid_size, (y-0.5)*grid_size, 0.0},
                .r = 0.5,
                .color = {1, 1, 1, 0.2},
            };
        }
    }

    bool do_draw_lightprobes = false;
    bool show_fps = true;
    real smoothed_frame_time = 1.0;

    bool step_mode = false;

    real Deltat = 0;
    //TODO: make render loop evenly sample inputs when vsynced
    while(update_window(&wnd))
    {
        // while(Deltat <= dt)
        // {
        //     wnd.last_time = wnd.this_time;
        //     QueryPerformanceCounter(&wnd.this_time);
        //     Deltat += ((real) (wnd.this_time.QuadPart - wnd.last_time.QuadPart))/(wnd.timer_frequency.QuadPart);
        // }
        // const int N_MAX_FRAMES = 2;
        // for(int i = 0; Deltat > 0 && i < N_MAX_FRAMES; Deltat -= dt, i++)

        #ifdef IMGUI_VERSION
        ImGui_ImplWin32_NewFrame();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();
        #endif

        wnd.last_time = wnd.this_time;
        QueryPerformanceCounter(&wnd.this_time);
        {
            while(Deltat > dt) Deltat -= dt;

            start_frame(&rd);
            start_frame(&ui);
            start_frame(&w.editor.rd);

            update_sounds(&ad);

            update_game(manager, &w, &rd, &ui, &ad, &wnd.input);

            update_camera_matrix(&rd);
            update_camera_matrix(&w.editor.rd);

            // if(is_pressed('R', wnd.input)) log_output("position: (", w.player.x.x, ", ", w.player.x.y, ", ", w.player.x.z, ")\n");

            if(is_down('K', &wnd.input))
            {
                LARGE_INTEGER time0;
                LARGE_INTEGER time1;
                QueryPerformanceCounter(&time0);

                // download_world_cells(cells);
                // QueryPerformanceCounter(&time1);
                // real download_time = ((real) (time1.QuadPart-time0.QuadPart))/(wnd.timer_frequency.QuadPart);
                // size_t compressed_size = compress_world_cells(cells, compressed_cells);
                // QueryPerformanceCounter(&time0);

                size_t encoded_size = encode_world_cells(rle_cells);
                QueryPerformanceCounter(&time1);
                real download_time = ((real) (time1.QuadPart-time0.QuadPart))/(wnd.timer_frequency.QuadPart);
                compressed_size = compress_world_cells(rle_cells, encoded_size, compressed_cells);
                QueryPerformanceCounter(&time0);
                real compression_time = ((real) (time0.QuadPart-time1.QuadPart))/(wnd.timer_frequency.QuadPart);
                log_output("downloading rlencoded world cells took ", download_time, "s, compression took ", compression_time, "s\n");
                size_t raw_size = room_size*room_size*room_size;
                log_output("downloaded and compressed world cells to ", 100.0f*compressed_size/encoded_size,"%, from ", encoded_size,  " bytes to ", compressed_size, " bytes\n");

                // total_run_length = encoded_size/2
                // for(int i = 0; i < ; i++)
                // {
                //     rle_cells[i].
                // }
            }

            if(is_pressed('L', &wnd.input))
            {
                LARGE_INTEGER time0;
                LARGE_INTEGER time1;
                QueryPerformanceCounter(&time0);

                size_t size = decompress_world_cells(rle_cells, compressed_cells, compressed_size);
                QueryPerformanceCounter(&time1);
                real decompression_time = ((real) (time1.QuadPart-time0.QuadPart))/(wnd.timer_frequency.QuadPart);
                decode_world_cells(rle_cells, size);
                QueryPerformanceCounter(&time0);
                real upload_time = ((real) (time0.QuadPart-time1.QuadPart))/(wnd.timer_frequency.QuadPart);
                log_output("uploading rlencoded world cells took ", upload_time, "s, decompression took ", decompression_time, "s\n");
                // for(int i = 0; i < 16; i++) simulate_world_cells(&w, 1);
            }

            cast_rays(&w);

            if(w.edit_mode)
            {
                edit_world(&w);
            }
            else
            {
                step_mode = step_mode != is_pressed(VK_OEM_COMMA, &wnd.input);
                if(step_mode ? is_pressed('Z', &wnd.input) : !is_down('Z', &wnd.input))
                {
                    simulate_particles();
                    update_beams(&w);
                    simulate_world_cells(&w, 1);
                }
            }

            if(is_pressed('P', &wnd.input)) do_draw_lightprobes = !do_draw_lightprobes;
            if(is_pressed(VK_F1, &wnd.input)) show_fps = !show_fps;

            /*NOTE FOR NEXT TIME:
              render at lower resolution, use that for lighting/reflection info,
              get voxel index of each pixel in low resolution prepass
              check 4 surrounding voxels during high resolution render to figure out which one to render
            */
        }

        if(!is_down('N', &wnd.input))
        {
            move_lightprobes();
            cast_lightprobes();
            update_lightprobes();
        }

        //Render world
        glEnable(GL_DEPTH_TEST);

        // render_depth_prepass(materials_textures[current_materials_texture], rd.camera_axes, rd.camera_pos, {512,512,512},{0,0,0});

        if(!is_down('B', &wnd.input))
        {
            render_prepass(rd.camera_axes, rd.camera_pos, (int_3){room_size, room_size, room_size}, (int_3){0,0,0}, w.bodies_gpu, w.n_bodies);

            glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_texture, 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normal_texture, 0);
            // glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_texture, 0);

            GLenum buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
            glDrawBuffers(len(buffers), buffers);

            glViewport(0, 0, resolution_x, resolution_y);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            render_world(rd.camera_axes, rd.camera_pos);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0, 0);
            glDrawBuffers(1, buffers);

            // render_room(w.c, rd.camera_axes, rd.camera_pos, w.bodies_gpu, w.n_bodies);

            // for(int b = 0; b < w.n_bodies; b++)
            // {
            //     render_body(w.bodies_cpu+b, w.bodies_gpu+b, rd.camera_axes, rd.camera_pos);
            // }
        }
        else
        {
            glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_texture, 0);
            // glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_texture, 0);

            glViewport(0, 0, resolution_x, resolution_y);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        // glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, denoised_color_texture, 0);
        // glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
        // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // glDisable(GL_DEPTH_TEST);
        // for(int i = 0; i < 1; i++)
        // {
        //     denoise(denoised_color_texture, color_texture);
        //     denoise(color_texture, denoised_color_texture);
        // }
        // glEnable(GL_DEPTH_TEST);

        if(w.edit_mode)
        {
            glDisable(GL_DEPTH_TEST);
            render_editor_voxels(&w.editor);
            glEnable(GL_DEPTH_TEST);
        }

        glDisable(GL_DEPTH_TEST);
        draw_circles(rd.circles, rd.n_circles, rd.camera);
        draw_circles(ui.circles, ui.n_circles, ui.camera);
        draw_rectangles(rd.rectangles, rd.n_rectangles, rd.camera);
        draw_rectangles(ui.rectangles, ui.n_rectangles, ui.camera);
        draw_sprites(rd.sprites, rd.n_sprites, rd.camera);
        draw_sprites(ui.sprites, ui.n_sprites, ui.camera);

        uint line_points_offset = 0;
        for(int l = 0; l < rd.n_lines; l++)
        {
            draw_round_line(manager, rd.line_points+line_points_offset, rd.n_line_points[l], rd.camera);
            line_points_offset += rd.n_line_points[l];
        }

        line_points_offset = 0;
        for(int l = 0; l < ui.n_lines; l++)
        {
            draw_round_line(manager, ui.line_points+line_points_offset, ui.n_line_points[l], ui.camera);
            line_points_offset += ui.n_line_points[l];
        }
        glEnable(GL_DEPTH_TEST);

        glEnable(GL_CULL_FACE);
        draw_rings(rd.rings, rd.n_rings, rd.camera, rd.camera_axes, rd.camera_pos);
        draw_spheres(rd.spheres, rd.n_spheres, rd.camera, rd.camera_axes, rd.camera_pos);
        draw_line_3ds(rd.line_3ds, rd.n_line_3ds, rd.camera, rd.camera_axes, rd.camera_pos);

        draw_rings(w.editor.rd.rings, w.editor.rd.n_rings, w.editor.rd.camera, w.editor.rd.camera_axes, w.editor.rd.camera_pos);
        draw_spheres(w.editor.rd.spheres, w.editor.rd.n_spheres, w.editor.rd.camera, w.editor.rd.camera_axes, w.editor.rd.camera_pos);
        draw_line_3ds(w.editor.rd.line_3ds, w.editor.rd.n_line_3ds, w.editor.rd.camera, w.editor.rd.camera_axes, w.editor.rd.camera_pos);

        glDisable(GL_CULL_FACE);

        if(do_draw_lightprobes)
        {
            glClear(GL_DEPTH_BUFFER_BIT);
            draw_lightprobes(rd.camera);
        }

        // glClear(GL_DEPTH_BUFFER_BIT);
        // draw_particles(rd.camera);

        // draw_beams(rd.camera, rd.camera_axes, &w);

        // draw_circles(ground_circles, n_ground_circles, rd.camera);

        char frame_time_text[1024];
        real frame_time = ((real) (wnd.this_time.QuadPart - wnd.last_time.QuadPart))/(wnd.timer_frequency.QuadPart);
        smoothed_frame_time = lerp(smoothed_frame_time, frame_time, 0.03);
        sprintf(frame_time_text, "%2.1f ms\n%2.1f fps", smoothed_frame_time*1000.0f, 1.0f/smoothed_frame_time);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, window_width, window_height);
        glDisable(GL_DEPTH_TEST);
        // draw_to_screen(color_texture);
        draw_to_screen_ssao(color_texture, depth_texture, normal_texture);

        // if(show_fps) draw_debug_text(frame_time_text, -1080/5+10, 720/5-10);
        if(show_fps) draw_text(frame_time_text, -(1.0*window_width/window_height)+0.15, 0.95, {1,1,1,1}, {}, font);
        draw_text(rd.debug_log, -0.9, -0.8, {1,1,1,1}, {-1, 1}, font);
        draw_text(ui.debug_log, -0.9, -0.8, {1,1,1,1}, {-1, 1}, font);

        for(int i = 0; i < ui.n_texts; i++)
            draw_text(ui.texts[i].text, ui.texts[i].x.x, ui.texts[i].x.y, ui.texts[i].color, ui.texts[i].alignment, font);

        // ImGui::ShowDemoWindow();

        #ifdef IMGUI_VERSION
        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        FrameMark;
        #endif
    }
    return 0;
}
