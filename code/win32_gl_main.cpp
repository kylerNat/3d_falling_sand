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

#undef assert

#include <maths/maths.h>
#include <utils/misc.h>
#include <utils/logging.h>

// #define platform_big_alloc(memory_size) VirtualAlloc(0, memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
void* platform_big_alloc(size_t memory_size)
{
    void* out = VirtualAlloc(0, memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    assert(out, "VirtualAlloc failed allocating ", memory_size, " bytes, error code = ", (int) GetLastError());
    return out;
}
// #define platform_big_alloc(memory_size) malloc(memory_size);
#include "memory.h"

#include "win32_work_system.h"

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

    DWORD file_size = 0;
    auto error = GetFileSize(file, &file_size);
    if(!error)
    {
        log_error(GetLastError(), " opening file\n");
        exit(EXIT_SUCCESS);
    }

    char* output = (char*) permalloc(manager, file_size+1);

    DWORD bytes_read;
    error = ReadFile(file,
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

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        case WM_SIZE:
        {
            // //TODO: set correct context
            window_width = LOWORD(lParam);
            window_height = HIWORD(lParam);
            glViewport(0, 0, window_width, window_height);
            // glViewport(0.5*(window_width-window_height), 0, window_height, window_height);
            // glViewport(0, 0.5*(window_height-window_width), window_width, window_width);
            break;
        }
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

    gl_init_programs(manager);

    gl_init_general_buffers(manager);

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

    wnd->input.buttons[M_WHEEL_DOWN/8] &= ~(11<<(M_WHEEL_DOWN%8));
    wnd->input.mouse_wheel = 0;
    wnd->input.dmouse = zero_2;

    POINT cursor_point;
    RECT window_rect;
    if(GetActiveWindow() == wnd->hwnd)
    {
        GetWindowRect(wnd->hwnd, &window_rect);
        wnd->size = {(window_rect.right-window_rect.left),
                    (window_rect.bottom-window_rect.top)};
        ClipCursor(&window_rect);
        ShowCursor(0);
    }

    GetCursorPos(&cursor_point);

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
                        #define update_numbered_button(N)               \
                            if(ms.usButtonFlags & RI_MOUSE_BUTTON_##N##_DOWN) \
                                wnd->input.buttons[0] |= 1<<M##N;       \
                            else if(ms.usButtonFlags & RI_MOUSE_BUTTON_##N##_UP) \
                                wnd->input.buttons[0] &= ~(1<<M##N);

                        update_numbered_button(1);
                        update_numbered_button(2);
                        update_numbered_button(3);
                        update_numbered_button(4);
                        update_numbered_button(5);

                        if(ms.usButtonFlags & RI_MOUSE_WHEEL)
                        {
                            wnd->input.mouse_wheel = (short) ms.usButtonData;
                            if((short) ms.usButtonData > 0)
                            {
                                wnd->input.buttons[1] |= 1<<(M_WHEEL_UP-8);
                            }
                            else
                            {
                                wnd->input.buttons[1] |= 1<<(M_WHEEL_DOWN-8);
                            }
                        }

                        break;
                    }
                    case RIM_TYPEKEYBOARD:
                    {
                        RAWKEYBOARD& kb = raw.data.keyboard;
                        if(kb.VKey == 'G')
                        {
                            wnd->input.mouse = {(2.0*cursor_point.x-(window_rect.left+window_rect.right))/window_height,
                                                (-2.0*cursor_point.y+(window_rect.bottom+window_rect.top))/window_height};
                        }
                        if(kb.Flags == RI_KEY_BREAK) set_key_up(kb.VKey, wnd->input);
                        if(kb.Flags == RI_KEY_MAKE) set_key_down(kb.VKey, wnd->input);
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

    wnd->input.mouse += wnd->input.dmouse;

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
    memory_block* block = allocate_new_block(block_size);
    memory_manager* manager = (memory_manager*) (block->memory + block->used);
    block->used += sizeof(memory_manager);
    manager->first = block;
    manager->current = manager->first;

    window_t wnd = create_window(manager, "3D Sand", "3dsand", 1280, 720, 10, 10, hinstance);
    // fullscreen(wnd);
    show_window(wnd);

    int next_id = 1;

    world w = {
        .seed = 128924,
        // .player = {.x = {211.1,200.1,32.1}, .x_dot = {0,0,0}},
        // .player = {.x = {289.1,175.1, 302.1}, .x_dot = {0,0,0}},
        .player = {.x = {chunk_size/2+45.1, chunk_size/2+25.1, 42.1}, .x_dot = {0,0,0}},
        .bodies_cpu = (cpu_body_data*) permalloc_clear(manager, 1024*sizeof(cpu_body_data)),
        .bodies_gpu = (gpu_body_data*) permalloc_clear(manager, 1024*sizeof(gpu_body_data)),
        .n_bodies = 0,
        .joints = (joint*) permalloc_clear(manager, 1024*sizeof(joint)),
        .n_joints = 0,
        .components = (body_component*) permalloc_clear(manager, 1024*sizeof(body_component)),
        .n_components = 0,
        .brains = (brain*) permalloc_clear(manager, 1024*sizeof(brain)),
        .n_brains = 0,
        .c = (chunk*) permalloc(manager, 8*sizeof(chunk)),
        .chunk_lookup = {},
    };

    int max_3d_texture_size;
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max_3d_texture_size);
    log_output("GL_MAX_3D_TEXTURE_SIZE: ", max_3d_texture_size, "\n");

    chunk* c = w.c;

    c->materials = (uint16*) permalloc(manager, sizeof(uint16)*chunk_size*chunk_size*chunk_size);

    for(int z = 0; z < chunk_size; z++)
        for(int y = 0; y < chunk_size; y++)
            for(int x = 0; x < chunk_size; x++)
            {
                int material = 0;
                int height = 20;
                real rsq = sq(x-chunk_size/2)+sq(y-chunk_size/2);
                // height += 0.03*rsq*(exp(-0.0005*rsq));
                int ramp_height = 20;
                // height += clamp(ramp_height-abs(y-128), 0, ramp_height);
                if(sqrt(rsq) < 80 && z < height+ramp_height-5) material = 2;
                height += clamp((float) ramp_height-abs(sqrt(rsq)-80), 0.0, (float) ramp_height);
                if(z < height) material = 1;
                // else if(z < chunk_size-1) material = (randui(&w.seed)%prob)==0;
                // if(x == chunk_size/2+0 && y == chunk_size/2+0) material = 2;
                // if(x == chunk_size/2+0 && y == chunk_size/2-1) material = 2;
                // if(x == chunk_size/2-1 && y == chunk_size/2-1) material = 2;
                // if(x == chunk_size/2-1 && y == chunk_size/2+0) material = 2;


                if(z <= 5) material = 3;
                if(z > chunk_size-10) material = 3;

                int door_width = 40;
                if(abs(x-chunk_size/2) > door_width/2 && abs(y-chunk_size/2) > door_width/2 ||
                   x <            5 ||
                   x > chunk_size-5 ||
                   y <            5 ||
                   y > chunk_size-5)
                {
                    if(y <= 10) material = 3;
                    if(x <= 10) material = 3;
                    if(chunk_size-y <= 10) material = 3;
                    if(chunk_size-x <= 10) material = 3;
                    // if(chunk_size-z <= 10) material = 3;
                }

                int wall_thickness = 20;
                if((abs(x-chunk_size/2) < wall_thickness/2 || abs(y-chunk_size/2) < wall_thickness/2) &&
                   !(abs(x-chunk_size*1/4) < door_width/2 || abs(y-chunk_size*1/4) < door_width/2
                     || abs(x-chunk_size*3/4) < door_width/2 || abs(y-chunk_size*3/4) < door_width/2))
                {
                     material = 3;
                }

                if(abs(z-chunk_size/2) < 5 && abs(x-chunk_size/2)+abs(y-chunk_size/2) > 60) material = 3;

                // if(y == chunk_size/2 && abs(x-chunk_size/2) <= 2*(i) && x%2 == 0) material = 3;
                if(y == chunk_size*3/4 && x == chunk_size*3/4+20 && z < 50) material = 3;

                // if(material == 0) material = 2*((randui(&w.seed)%10000)==0); //"rain"
                c->materials[x+chunk_size*y+chunk_size*chunk_size*z] = material;
            }

    load_chunk_to_gpu(c);

    for(int b = 0; b < 4; b++)
    {
        w.bodies_gpu[b].size = {12,12,12};
        w.bodies_cpu[b].materials = (uint16*) permalloc(manager, w.bodies_gpu[b].size.x*w.bodies_gpu[b].size.y*w.bodies_gpu[b].size.z*sizeof(uint16));
        w.bodies_gpu[b].x_cm = 0.5*real_cast(w.bodies_gpu[b].size);
        w.bodies_gpu[b].x = {chunk_size*3/4-5*b, chunk_size/2, 50};
        w.bodies_gpu[b].x_dot = {0,0,0};
        // w.bodies_gpu[b].omega = {0.0,-0.5,0.0};
        w.bodies_gpu[b].omega = {0.0,0.0,0.0};
        w.bodies_gpu[b].orientation = {1,0,0,0};

        w.bodies_gpu[b].m = 1.0;
        w.bodies_cpu[b].I = real_identity_3(10.0);

        real half_angle = 0.5*(pi/4);
        real_3 axis = {1,0,0};
        quaternion rotation = (quaternion){cos(half_angle), sin(half_angle)*axis.x, sin(half_angle)*axis.y, sin(half_angle)*axis.z};
        // w.b->orientation = w.b->orientation*rotation;

        for(int z = 0; z < w.bodies_gpu[b].size.z; z++)
            for(int y = 0; y < w.bodies_gpu[b].size.y; y++)
                for(int x = 0; x < w.bodies_gpu[b].size.x; x++)
                {
                    bool x_middle = x >= w.bodies_gpu[b].size.x/3 && x < w.bodies_gpu[b].size.x*2/3;
                    bool y_middle = y >= w.bodies_gpu[b].size.x/3 && y < w.bodies_gpu[b].size.x*2/3;
                    bool z_middle = z >= w.bodies_gpu[b].size.x/3 && z < w.bodies_gpu[b].size.x*2/3;

                    // int material = 2;
                    // if((x+y+z)%2==0 && (z <= 0 || z >= w.bodies_gpu[b].size.z-1)) material = 1;
                    // if((x+y+z)%2==0 && (x <= 0 || x >= w.bodies_gpu[b].size.x-1)) material = 1;
                    // if((x+y+z)%2==0 && (y <= 0 || y >= w.bodies_gpu[b].size.y-1)) material = 1;
                    // if(x_middle && y_middle) material = 0;
                    // if(y_middle && z_middle) material = 0;
                    // if(z_middle && x_middle) material = 0;

                    int material = 0;
                    if(x_middle && y_middle) material = 3;
                    if(y_middle && z_middle) material = 3;
                    if(z_middle && x_middle) material = 3;
                    if(material == 3 && (x+y+z)%2==0 && (z <= 0 || z >= w.bodies_gpu[b].size.z-1)) material = 1;
                    if(material == 3 && (x+y+z)%2==0 && (x <= 0 || x >= w.bodies_gpu[b].size.x-1)) material = 1;
                    if(material == 3 && (x+y+z)%2==0 && (y <= 0 || y >= w.bodies_gpu[b].size.y-1)) material = 1;

                    // if(abs(y - 12) > 2 || abs(x - 12) > 2) material = 0;

                    w.bodies_cpu[b].materials[x+y*w.bodies_gpu[b].size.x+z*w.bodies_gpu[b].size.x*w.bodies_gpu[b].size.y] = material;
                }
        w.n_bodies++;
    }

    int limb_length = 6;
    int shoulder_length = 6;
    int body_length = 8;
    int body_width = 5;
    int body_depth = 1;
    int limb_thickness = 1;
    real head_size = 5;

    int limb_start_id = w.n_bodies;
    int body_id;
    int head_id;
    for(int i = 0; i < 8; i++) //limbs
    {
        int b = w.n_bodies;
        w.bodies_gpu[b].size = {limb_length,limb_thickness,limb_thickness};
        w.bodies_cpu[b].materials = (uint16*) permalloc(manager, w.bodies_gpu[b].size.x*w.bodies_gpu[b].size.y*w.bodies_gpu[b].size.z*sizeof(uint16));
        w.bodies_gpu[b].x_cm = 0.5*real_cast(w.bodies_gpu[b].size);
        w.bodies_gpu[b].x = {chunk_size*3/4-0.5*b, chunk_size/2, 50};
        w.bodies_gpu[b].x_dot = {0,0,0};
        w.bodies_gpu[b].omega = {0.0,0.0,0.0};
        w.bodies_gpu[b].orientation = {1,0,0,0};

        w.bodies_gpu[b].m = 0.0;
        w.bodies_cpu[b].I = {};

        w.bodies_cpu[b].root = limb_start_id+9;

        real half_angle = 0.5*(pi/4);
        real_3 axis = {1,0,0};
        quaternion rotation = (quaternion){cos(half_angle), sin(half_angle)*axis.x, sin(half_angle)*axis.y, sin(half_angle)*axis.z};
        // w.b->orientation = w.b->orientation*rotation;

        for(int z = 0; z < w.bodies_gpu[b].size.z; z++)
            for(int y = 0; y < w.bodies_gpu[b].size.y; y++)
                for(int x = 0; x < w.bodies_gpu[b].size.x; x++)
                {
                    int material = 3;
                    w.bodies_cpu[b].materials[x+y*w.bodies_gpu[b].size.x+z*w.bodies_gpu[b].size.x*w.bodies_gpu[b].size.y] = material;

                    real m = 0.001;
                    if(material == 0) m = 0;
                    w.bodies_gpu[b].m += m;

                    real_3 r = {x+0.5,y+0.5,z+0.5};
                    r += -w.bodies_gpu[b].x_cm;
                    w.bodies_cpu[b].I += real_identity_3(m*(0.4*sq(0.5)+normsq(r)));
                    for(int i = 0; i < 3; i++)
                        for(int j = 0; j < 3; j++)
                            w.bodies_cpu[b].I[i][j] += -m*r[i]*r[j];
                }
        w.n_bodies++;
    }

    {
        int b = w.n_bodies;
        body_id = b;
        w.bodies_gpu[b].size = {body_length,body_width,body_depth};
        w.bodies_cpu[b].materials = (uint16*) permalloc(manager, w.bodies_gpu[b].size.x*w.bodies_gpu[b].size.y*w.bodies_gpu[b].size.z*sizeof(uint16));
        w.bodies_gpu[b].x_cm = 0.5*real_cast(w.bodies_gpu[b].size);
        w.bodies_gpu[b].x = {chunk_size*3/4-0.5*b, chunk_size/2, 50};
        w.bodies_gpu[b].x_dot = {0,0,0};
        w.bodies_gpu[b].omega = {0.0,0.0,0.0};
        w.bodies_gpu[b].orientation = {1,0,0,0};

        w.bodies_gpu[b].m = 0.0;
        w.bodies_cpu[b].I = {};

        real half_angle = 0.5*(pi/4);
        real_3 axis = {1,0,0};
        quaternion rotation = (quaternion){cos(half_angle), sin(half_angle)*axis.x, sin(half_angle)*axis.y, sin(half_angle)*axis.z};
        // w.b->orientation = w.b->orientation*rotation;

        for(int z = 0; z < w.bodies_gpu[b].size.z; z++)
            for(int y = 0; y < w.bodies_gpu[b].size.y; y++)
                for(int x = 0; x < w.bodies_gpu[b].size.x; x++)
                {
                    int material = 0;
                    if(x < shoulder_length || (z == 0 && y == body_width/2)) material = 3;
                    w.bodies_cpu[b].materials[x+y*w.bodies_gpu[b].size.x+z*w.bodies_gpu[b].size.x*w.bodies_gpu[b].size.y] = material;

                    real m = 0.001;
                    if(material == 0) m = 0;
                    w.bodies_gpu[b].m += m;

                    real_3 r = {x+0.5,y+0.5,z+0.5};
                    r += -w.bodies_gpu[b].x_cm;
                    w.bodies_cpu[b].I += real_identity_3(m*(0.4*sq(0.5)+normsq(r)));
                    for(int i = 0; i < 3; i++)
                        for(int j = 0; j < 3; j++)
                            w.bodies_cpu[b].I[i][j] += -m*r[i]*r[j];
                }
        w.n_bodies++;
    }

    {
        int b = w.n_bodies;
        head_id = b;
        w.bodies_gpu[b].size = {6,6,6};
        w.bodies_cpu[b].materials = (uint16*) permalloc(manager, w.bodies_gpu[b].size.x*w.bodies_gpu[b].size.y*w.bodies_gpu[b].size.z*sizeof(uint16));
        w.bodies_gpu[b].x_cm = {head_size/2,head_size/2,head_size/2};
        w.bodies_gpu[b].x = {chunk_size*3/4-0.5*b, chunk_size/2, 50};
        w.bodies_gpu[b].x_dot = {0,0,0};
        w.bodies_gpu[b].omega = {0.0,0.0,0.0};
        w.bodies_gpu[b].orientation = {1,0,0,0};

        w.bodies_gpu[b].m = 0.0;
        w.bodies_cpu[b].I = {};

        real half_angle = 0.5*(pi/4);
        real_3 axis = {1,0,0};
        quaternion rotation = (quaternion){cos(half_angle), sin(half_angle)*axis.x, sin(half_angle)*axis.y, sin(half_angle)*axis.z};
        // w.b->orientation = w.b->orientation*rotation;

        for(int z = 0; z < w.bodies_gpu[b].size.z; z++)
            for(int y = 0; y < w.bodies_gpu[b].size.y; y++)
                for(int x = 0; x < w.bodies_gpu[b].size.x; x++)
                {
                    if(x >= head_size || y >= head_size || z >= head_size) continue;
                    int material = 3;
                    w.bodies_cpu[b].materials[x+y*w.bodies_gpu[b].size.x+z*w.bodies_gpu[b].size.x*w.bodies_gpu[b].size.y] = material;
                    real m = 0.001;
                    if(material == 0) m = 0;
                    w.bodies_gpu[b].m += m;

                    real_3 r = {x+0.5,y+0.5,z+0.5};
                    r += -w.bodies_gpu[b].x_cm;
                    w.bodies_cpu[b].I += real_identity_3(m*(0.4*sq(0.5)+normsq(r)));
                    for(int i = 0; i < 3; i++)
                        for(int j = 0; j < 3; j++)
                            w.bodies_cpu[b].I[i][j] += -m*r[i]*r[j];
                }
        w.n_bodies++;
    }

    {
        int b = w.n_bodies;
        w.bodies_gpu[b].size = {20,10,5};
        w.bodies_cpu[b].materials = (uint16*) permalloc(manager, w.bodies_gpu[b].size.x*w.bodies_gpu[b].size.y*w.bodies_gpu[b].size.z*sizeof(uint16));
        w.bodies_gpu[b].x_cm = 0.5*real_cast(w.bodies_gpu[b].size);
        w.bodies_gpu[b].x = {chunk_size*3/4-0.5*b, chunk_size/2, 50};
        w.bodies_gpu[b].x_dot = {0,0,0};
        w.bodies_gpu[b].omega = {0.0,0.0,0.0};
        w.bodies_gpu[b].orientation = {1,0,0,0};

        w.bodies_gpu[b].m = 0.0;
        w.bodies_cpu[b].I = {};

        real half_angle = 0.5*(pi/4);
        real_3 axis = {1,0,0};
        quaternion rotation = (quaternion){cos(half_angle), sin(half_angle)*axis.x, sin(half_angle)*axis.y, sin(half_angle)*axis.z};
        // w.b->orientation = w.b->orientation*rotation;

        for(int z = 0; z < w.bodies_gpu[b].size.z; z++)
            for(int y = 0; y < w.bodies_gpu[b].size.y; y++)
                for(int x = 0; x < w.bodies_gpu[b].size.x; x++)
                {
                    int material = 3;
                    w.bodies_cpu[b].materials[x+y*w.bodies_gpu[b].size.x+z*w.bodies_gpu[b].size.x*w.bodies_gpu[b].size.y] = material;
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
        w.n_bodies++;
    }

    w.bodies_cpu[0].is_root = true;
    w.bodies_cpu[0].joints[w.bodies_cpu[0].n_children++] = {
        .type = joint_hinge,
        .child_body_id = 1,
        .pos = {{4,4,0}, {4,4,0}},
        .axis = {0,0},
        .max_speed = 0.5,
        .max_torque = 0.2,
    };

    w.bodies_cpu[head_id].is_root = true;
    w.bodies_cpu[head_id].joints[w.bodies_cpu[head_id].n_children++] = {
        .type = joint_ball,
        .child_body_id = body_id,
        .pos = {{(head_size-1)/2,(head_size-1)/2,0}, {body_length-1,body_width/2,0}},
        .axis = {2,0},
        .max_speed = 0.5,
        .max_torque = 0.2,
    };

    for(int i = 0; i < 4; i++)
    {
        w.bodies_cpu[limb_start_id+2*i].joints[w.bodies_cpu[limb_start_id+2*i].n_children++] = {
            .type = joint_ball,
            .child_body_id = limb_start_id+2*i+1,
            .pos = {{limb_length-1,0,0}, {0,0,0}},
            .axis = {1,1},
            .max_speed = 0.5,
            .max_torque = 0.2,
        };
    }

    w.bodies_cpu[limb_start_id+5].joints[w.bodies_cpu[limb_start_id+5].n_children++] = {
        .type = joint_ball,
        .child_body_id = limb_start_id+0,
        .pos = {{limb_length-1,0,0}, {0,0,0}},
        .axis = {1,1},
        .max_speed = 0.5,
        .max_torque = 0.2,
    };

    w.bodies_cpu[limb_start_id+7].joints[w.bodies_cpu[limb_start_id+7].n_children++] = {
        .type = joint_ball,
        .child_body_id = limb_start_id+2,
        .pos = {{limb_length-1,0,0}, {0,0,0}},
        .axis = {1,1},
        .max_speed = 0.5,
        .max_torque = 0.2,
    };

    w.bodies_cpu[body_id].joints[w.bodies_cpu[body_id].n_children++] = {
        .type = joint_ball,
        .child_body_id = limb_start_id+4,
        .pos = {{0,0,0}, {0,0,0}},
        .axis = {1,1},
        .max_speed = 0.5,
        .max_torque = 0.2,
    };

    w.bodies_cpu[body_id].joints[w.bodies_cpu[body_id].n_children++] = {
        .type = joint_ball,
        .child_body_id = limb_start_id+6,
        .pos = {{0,body_width-1,0}, {0,0,0}},
        .axis = {1,1},
        .max_speed = 0.5,
        .max_torque = 0.2,
    };

    // w.bodies_cpu[body_id].joints[w.bodies_cpu[body_id].n_children++] = {
    //     .type = joint_ball,
    //     .child_body_id = limb_start_id+0,
    //     .pos = {{0,0,0}, {0,0,0}},
    //     .axis = {1,1},
    //     .max_speed = 0.5,
    //     .max_torque = 0.2,
    // };

    // w.bodies_cpu[body_id].joints[w.bodies_cpu[body_id].n_children++] = {
    //     .type = joint_ball,
    //     .child_body_id = limb_start_id+2,
    //     .pos = {{0,body_width-1,0}, {0,0,0}},
    //     .axis = {1,1},
    //     .max_speed = 0.5,
    //     .max_torque = 0.2,
    // };

    // w.bodies_cpu[body_id].joints[w.bodies_cpu[body_id].n_children++] = {
    //     .type = joint_ball,
    //     .child_body_id = limb_start_id+4,
    //     .pos = {{shoulder_length-1,0,0}, {0,0,0}},
    //     .axis = {1,1},
    //     .max_speed = 0.5,
    //     .max_torque = 0.2,
    // };

    // w.bodies_cpu[body_id].joints[w.bodies_cpu[body_id].n_children++] = {
    //     .type = joint_ball,
    //     .child_body_id = limb_start_id+6,
    //     .pos = {{shoulder_length-1,body_width-1,0}, {0,0,0}},
    //     .axis = {1,1},
    //     .max_speed = 0.5,
    //     .max_torque = 0.2,
    // };

    brain* br = &w.brains[w.n_brains++];
    *br = {
        .endpoints = (endpoint*) permalloc(manager, sizeof(endpoint)*1024),
        .n_endpoints = 0,
    };
    br->endpoints[br->n_endpoints++] = {
        .type = endpoint_foot,
        .body_id = 5,
        .pos = {limb_length-1,0,0},
        .state = foot_lift,
        .foot_phase_offset = 0.0,
        .target_type = tv_velocity,
        .target_value = {0,0,0},
    };

    br->endpoints[br->n_endpoints++] = {
        .type = endpoint_foot,
        .body_id = 7,
        .pos = {limb_length-1,0,0},
        .state = foot_lift,
        .foot_phase_offset = 0.5,
        .target_type = tv_velocity,
        .target_value = {0,0,0},
    };

    for(int j = 0; j < w.n_joints; j++)
    {
        w.components[w.n_components++] = {comp_joint, (void*) &w.joints[j]};
    }

    for(int b = 0; b < w.n_bodies; b++)
    {
        w.bodies_cpu[b].invI = inverse(w.bodies_cpu[b].I);
        update_inertia(&w.bodies_cpu[b], &w.bodies_gpu[b]);

        load_body_to_gpu(&w.bodies_cpu[b], &w.bodies_gpu[b]);
    }

    // CreateDirectory(w.tim.savedir, 0);
    // w.entity_savefile = open_file("entities.dat");

    render_data rd = {
        .circles = (circle_render_info*) permalloc(manager, 1000000*sizeof(circle_render_info)),
        .n_circles = 0,

        .line_points = (line_render_info*) permalloc(manager, 10000*sizeof(line_render_info)),
        .n_total_line_points = 0,
        .n_line_points = (uint*) permalloc(manager, 1000*sizeof(uint)),
        .n_lines = 0,

        .sheets = (sheet_render_info*) permalloc(manager, 10000*sizeof(sheet_render_info)),
        .n_sheets = 0,

        .spherical_functions = (spherical_function_render_info*) permalloc(manager, 10000*sizeof(spherical_function_render_info)),
        .n_spherical_functions = 0,

        .blobs = (blob_render_info*) permalloc(manager, 10000*sizeof(blob_render_info)),
        .n_blobs = 0,
    };

    render_data ui = {
        .circles = (circle_render_info*) permalloc(manager, 10000*sizeof(circle_render_info)),
        .n_circles = 0,

        .line_points = (line_render_info*) permalloc(manager, 10000*sizeof(line_render_info)),
        .n_total_line_points = 0,
        .n_line_points = (uint*) permalloc(manager, 1000*sizeof(uint)),
        .n_lines = 0,

        .spherical_functions = (spherical_function_render_info*) permalloc(manager, 1000*sizeof(spherical_function_render_info)),
        .n_spherical_functions = 0,
    };

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
            .buffer = (int16*) permalloc(manager, buffer_size),
            .sample_rate = wft.nSamplesPerSec,
            .source_voice = source_voice,
            .samples_played = 0,

            .noises = (noise*) permalloc(manager, 1000*sizeof(noise)),
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

    ui.camera = {
        1.0*window_height/window_width, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };

    work_stack = (work_task*) permalloc(manager, 1024*sizeof(work_task));
    n_work_entries = 0;
    n_remaining_tasks = 0;
    work_semephore = CreateSemaphoreA(0, 0, n_max_workers, NULL);
    for(int i = 0; i < n_max_workers+1; i++)
    {
        DWORD thread_id;
        if(i == 0)
        {
            main_context  = create_thread_context(0);
            continue;
        }

        HANDLE thread_handle = CreateThread(0, 0, thread_proc, (void*) (uint64) i, 0, &thread_id);
        CloseHandle(thread_handle);
    }

    waterfall = load_audio_file(manager, "../sounds/water_flow.wav");
    jump_sound = load_audio_file(manager, "../sounds/Meat_impacts_0.wav");
    // play_sound(&ad, &waterfall, 1.0);

    int grid_width = 100;
    int grid_height = 100;
    real grid_size = 10.0;
    int n_ground_circles = (grid_width+1)*(grid_height+1);
    circle_render_info* ground_circles = (circle_render_info*) permalloc(manager, sizeof(circle_render_info)*n_ground_circles);

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
        {
            while(Deltat > dt) Deltat -= dt;
            rd.n_sheets = 0;
            rd.n_spherical_functions = 0;
            rd.n_blobs = 0;
            rd.n_circles = 0;
            rd.n_circles = 0;
            rd.n_total_line_points = 0;
            rd.n_lines = 0;
            rd.n_line_points[0] = 0;

            ui.n_sheets = 0;
            ui.n_spherical_functions = 0;
            ui.n_circles = 0;
            ui.n_total_line_points = 0;
            ui.n_lines = 0;
            ui.n_line_points[0] = 0;

            update_sounds(&ad);

            update_and_render(manager, &w, &rd, &ui, &ad, wnd.input);

            if(is_pressed('R', wnd.input)) log_output("position: (", w.player.x.x, ", ", w.player.x.y, ", ", w.player.x.z, ")\n");
            // simulate_chunk(w.c, 1);
            // for(int i = 0 ; i < 8; i++)
            //     simulate_chunk(w.c, 0);
            if(!is_down('Z', wnd.input)) simulate_chunk(w.c, 1);
            //
            //TODO: seperate update and render to improve performance and allow for a partial step for visual updating
            memcpy(wnd.input.prev_buttons, wnd.input.buttons, sizeof(wnd.input.buttons));
        }

        glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_texture, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
        glViewport(0, 0, resolution_x, resolution_y);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //Render world

        glEnable(GL_DEPTH_TEST);

        // render_depth_prepass(materials_textures[current_materials_texture], rd.camera_axes, rd.camera_pos, {512,512,512},{0,0,0});

        if(!is_down('N', wnd.input))
        {
            update_lightprobes();
        }

        if(!is_down('B', wnd.input))
        {
            render_chunk(w.c, rd.camera_axes, rd.camera_pos, w.bodies_gpu, w.n_bodies);

            // for(int b = 0; b < w.n_bodies; b++)
            // {
            //     render_body(w.bodies_cpu+b, w.bodies_gpu+b, rd.camera_axes, rd.camera_pos);
            // }
        }

        glDisable(GL_DEPTH_TEST);
        draw_circles(rd.circles, rd.n_circles, rd.camera);
        glEnable(GL_DEPTH_TEST);

        draw_circles(ui.circles, ui.n_circles, ui.camera);

        draw_circles(ground_circles, n_ground_circles, rd.camera);

        // uint line_points_offset = 0;
        // for(int l = 0; l < rd.n_lines; l++)
        // {
        //     draw_round_line(manager, rd.line_points+line_points_offset, rd.n_line_points[l], rd.camera);
        //     line_points_offset += rd.n_line_points[l];
        // }

        // line_points_offset = 0;
        // for(int l = 0; l < ui.n_lines; l++)
        // {
        //     draw_round_line(manager, ui.line_points+line_points_offset, ui.n_line_points[l], ui.camera);
        //     line_points_offset += ui.n_line_points[l];
        // }

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, window_width, window_height);
        glDisable(GL_DEPTH_TEST);
        draw_to_screen(color_texture);
    }
    return 0;
}
