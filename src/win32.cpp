#include "win32.h"
#include <windows.h>
#include <cstdlib>
#include <cstdint>
#include <cstdio>

struct Frame {
    int width;
    int height;
    uint32_t *pixels;
};

struct Window {
    // Render
    Frame frame;
    BITMAPINFO frame_bitmap_info;
    HBITMAP frame_bitmap;
    HDC frame_device_context;
    // Input events
};

LRESULT CALLBACK 
WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static Window *window = NULL;

    switch(msg)
    {
        case WM_CREATE: 
        {
            CREATESTRUCT *create_struct = (CREATESTRUCT*)lParam;
            window = (Window*)create_struct->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)window);
        } break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        case WM_PAINT: 
        {
            //LONG_PTR ptr = GetWindowLongPtr(hwnd, GWLP_USERDATA);
            //window = reinterpret_cast<Window*>(ptr);

            static PAINTSTRUCT paint;
            static HDC device_context;

            device_context = BeginPaint(hwnd, &paint);
            
            BitBlt(device_context, 
                paint.rcPaint.left, paint.rcPaint.top, 
                paint.rcPaint.right - paint.rcPaint.left, paint.rcPaint.bottom - paint.rcPaint.top, 
                window->frame_device_context,
                paint.rcPaint.left, paint.rcPaint.top,
                SRCCOPY);

            EndPaint(hwnd, &paint);
        } break;
        case WM_SIZE: 
        {
            //LONG_PTR ptr = GetWindowLongPtr(hwnd, GWLP_USERDATA);
            //window = reinterpret_cast<Window*>(ptr);

            window->frame_bitmap_info.bmiHeader.biWidth = LOWORD(lParam);
            window->frame_bitmap_info.bmiHeader.biHeight = HIWORD(lParam);

            if (window->frame_bitmap) 
            {
                DeleteObject(window->frame_bitmap);
            }

            window->frame_bitmap = CreateDIBSection(NULL, 
                &window->frame_bitmap_info, 
                DIB_RGB_COLORS, 
                reinterpret_cast<void**>(&window->frame.pixels), 
                0, 0);

            SelectObject(window->frame_device_context, window->frame_bitmap);

            window->frame.width = LOWORD(lParam);
            window->frame.height = HIWORD(lParam);
        } break;
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) 
            {
                PostQuitMessage(0);
            } else 
            {
                // record in window struct
            }
            break;
        case WM_KEYUP:
            // record in window struct
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

const char window_class[] = "emulatorClass";

int CALLBACK 
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 
{
    int argc = 1;
    char *arg = lpCmdLine;

    while (arg[0] != NULL) 
    {
        while (arg[0] != 0 && arg[0] == ' ') 
        {
            arg++;
        }

        if (arg[0] != 0) 
        {
            argc++;

            while (arg[0] != 0 && arg[0] != ' ') 
            {
                arg++;
            }
        }
    }

    char **argv = reinterpret_cast<char**>(malloc(argc * sizeof(char*)));
    int index = 1;
    arg = lpCmdLine;
    
    while (arg[0] != NULL) 
    {
        while (arg[0] != 0 && arg[0] == ' ') 
        {
            arg++;
        }

        if (arg[0] != 0) 
        {
            argv[index] = arg;
            index++;

            while (arg[0] != 0 && arg[0] != ' ') 
            {
                arg++;
            }

            if (arg[0] != 0) 
            {
                arg[0] = 0;
                arg++;
            }
        }
    }

    char filename[_MAX_PATH];
    
    GetModuleFileNameA(NULL, filename, _MAX_PATH);
    argv[0] = filename;

    void *application = NULL;
    int width = CW_USEDEFAULT;
    int height = CW_USEDEFAULT;
    char *window_title = "Window";

#if 1
    AllocConsole();
    freopen("CON", "w", stdout);
    freopen("CON", "r", stdin);
    char title[128];
    sprintf_s(title, "Attached to: %i", GetCurrentProcessId());
    SetConsoleTitleA(title);
#endif

    if (!init_application(argc, argv, &application, &width, &height, &window_title) || !application) 
    {
        MessageBoxExA(NULL, "Failed to start application", "Error", MB_ICONERROR | MB_OK, 0);
        return -1;
    }

    WNDCLASSEXA wcex = {};
    wcex.cbSize = sizeof(wcex);
    wcex.style = 0 ;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
    wcex.lpszClassName = window_class;

    if (!RegisterClassExA(&wcex))
    {
        MessageBoxExA(NULL, "Failed to register window class", "Error", MB_ICONERROR | MB_OK, 0);
        return -1;
    }

    RECT rect = {};
    rect.bottom = height;
    rect.right = width;

    if (!AdjustWindowRectEx(&rect, WS_TILED | WS_CAPTION, false, 0))
    {
        MessageBoxExA(NULL, "Failed to adjust window rectangle", "Error", MB_ICONERROR | MB_OK, 0);
        return -1;
    }

    Window window = {};
    window.frame_bitmap_info.bmiHeader.biSize = sizeof(window.frame_bitmap_info.bmiHeader);
    window.frame_bitmap_info.bmiHeader.biPlanes = 1;
    window.frame_bitmap_info.bmiHeader.biBitCount = 32;
    window.frame_bitmap_info.bmiHeader.biCompression = BI_RGB;
    window.frame_device_context = CreateCompatibleDC(0);

    HWND hwnd = CreateWindowExA(
        0,
        window_class,
        window_title,
        WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 
        rect.right - rect.left, rect.bottom - rect.top,
        NULL,
        NULL,
        hInstance,
        &window
        );

    if (!hwnd) 
    {
        MessageBoxExA(NULL, "Failed to create window", "Error", MB_ICONERROR | MB_OK, 0);
        return -1;
    }

    MSG msg = {};
    bool running = true;

    while (running) 
    {
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) 
        {
            if (msg.message == WM_QUIT) 
            {
                running = false;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (!running) 
        {
            break;
        }

        if (!update_application(application))
        {
            break;
        }

        // Sound?

        handle_input(application);

        if (render_application(application, window.frame.pixels, window.frame.width, window.frame.height)) 
        {
            InvalidateRect(hwnd, NULL, false);
            UpdateWindow(hwnd);
        }
    }

    return 0;
}

uint8_t * 
read_file(char *filename, uint64_t *file_size) 
{
    HANDLE handle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (!handle) 
    {
        return NULL;
    }

    *file_size = GetFileSize(handle, NULL);

    if (file_size == 0) 
    {
        CloseHandle(handle);
        return NULL;
    }

    uint8_t *data = reinterpret_cast<uint8_t*>(malloc(*file_size));
    DWORD bytes_read;

    if (!ReadFile(handle, data, *file_size, &bytes_read, NULL) || bytes_read != *file_size) 
    {
        CloseHandle(handle);
        return NULL;
    }

    CloseHandle(handle);
    return data;
}

void
message_box(char *title, char *msg)
{
    MessageBoxExA(NULL, msg, title, MB_OK, 0);
}