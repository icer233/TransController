#include <Windows.h>
#include <string>
#include <sstream>
#include <commctrl.h>

// 全局变量
HWND g_hEditBox;         // 文本框句柄
HWND g_hExitButton;      // 退出按钮句柄
HWND g_hBackgroundButton;// 后台运行按钮句柄
HWND g_hStepEdit;        // 步长输入框句柄
int currentTransparency = 100;  // 当前不透明度，默认100%
HHOOK g_hMouseHook;      // 鼠标钩子
const wchar_t windowTitle[25] = L"透明度控制者v1.2.5 作者：ICER233";

// 设置窗口不透明度
void SetWindowTransparency(HWND hwnd, int transparency) {
    if (hwnd) {
        LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);
        SetLayeredWindowAttributes(hwnd, 0, (255 * transparency) / 100, LWA_ALPHA);
    }
}

// 注册全局快捷键
bool RegisterHotKeys(HWND hwnd) {
    for (int i = 0; i <= 9; i++) {
        if (!RegisterHotKey(hwnd, i, MOD_ALT | MOD_SHIFT, 0x30 + i)) {
            return false;
        }
        std::wstringstream ss;
        ss << L"快捷键 Alt+Shift+" << i << L" 注册成功！\n";
        std::wstring log = ss.str();
        SendMessage(g_hEditBox, EM_SETSEL, -1, -1);
        SendMessage(g_hEditBox, EM_REPLACESEL, 0, (LPARAM)log.c_str());
    }
    return true;
}

// 设置控件字体为微软雅黑
void SetFont(HWND hWnd, int height) {
    HFONT hFont = CreateFont(
        height, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        FIXED_PITCH | FF_DONTCARE, L"微软雅黑"
    );
    SendMessage(hWnd, WM_SETFONT, (WPARAM)hFont, TRUE);
}

// 后台运行功能
void RunInBackground(HWND hwnd) {
    ShowWindow(hwnd, SW_HIDE);
}

void LogWindowTransparency(HWND hwnd, int transparency) {
    WCHAR title[256];
    GetWindowTextW(hwnd, title, 256);
    std::wstringstream ss;
    ss << L"已将[0x" << std::hex << (uintptr_t)hwnd << std::dec << L"][" << title << L"]的不透明度设为 " << transparency << L"%\n";
    std::wstring log = ss.str();
    SendMessage(g_hEditBox, EM_SETSEL, -1, -1);
    SendMessage(g_hEditBox, EM_REPLACESEL, 0, 0);
    SendMessage(g_hEditBox, EM_REPLACESEL, 0, (LPARAM)log.c_str());
}

// 处理鼠标滚轮调整前景窗口不透明度
void HandleMouseWheel(HWND hwnd, int delta) {
    HWND foregroundWindow = GetForegroundWindow();
    if (foregroundWindow) {
        if (GetAsyncKeyState(VK_MENU) & 0x8000 && GetAsyncKeyState(VK_SHIFT) & 0x8000) {
            // 获取步长设置
            wchar_t stepStr[10] = { 0 };
            GetWindowTextW(g_hStepEdit, stepStr, 10);
            int step = _wtoi(stepStr);
            if (step <= 0 || step > 100) step = 2;

            if (delta > 0 && currentTransparency < 100) {
                currentTransparency += step;
            }
            else if (delta < 0 && currentTransparency > 0) {
                currentTransparency -= step;
            }
            currentTransparency = max(0, min(100, currentTransparency));
            SetWindowTransparency(foregroundWindow, currentTransparency);
            LogWindowTransparency(foregroundWindow, currentTransparency);
        }
    }
}

// 鼠标滚轮钩子回调函数
LRESULT CALLBACK MouseWheelProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        MSLLHOOKSTRUCT* pMouse = (MSLLHOOKSTRUCT*)lParam;
        if (wParam == WM_MOUSEWHEEL) {
            int delta = GET_WHEEL_DELTA_WPARAM(pMouse->mouseData);
            HWND foregroundWindow = GetForegroundWindow();
            if (foregroundWindow) {
                HandleMouseWheel(foregroundWindow, delta);
            }
        }
    }
    return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}

// 安装 & 移除 鼠标滚轮钩子
void InstallMouseHook() {
    g_hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseWheelProc, NULL, 0);
}
void RemoveMouseHook() {
    UnhookWindowsHookEx(g_hMouseHook);
}

// 检查程序是否已在运行
bool IsAnotherInstanceRunning() {
    HANDLE hMutex = CreateMutex(NULL, TRUE, L"TransparencyAppMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(hMutex);
        return true;
    }
    return false;
}

// 窗口过程函数
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // 创建步长控件
        CreateWindow(L"STATIC", L"滚轮步长：",
            WS_CHILD | WS_VISIBLE,
            125, 200, 70, 25, hwnd, NULL, GetModuleHandle(NULL), NULL
        );

        // 创建文本框
        g_hEditBox = CreateWindowEx(
            WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            10, 10, 360, 180, hwnd, NULL, GetModuleHandle(NULL), NULL
        );

        // 创建步长设置文本框
        g_hStepEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"2",
            WS_CHILD | WS_VISIBLE | ES_NUMBER, 195, 200, 50, 25,
            hwnd, (HMENU)3, GetModuleHandle(NULL), NULL);

        // 创建功能按钮
        g_hBackgroundButton = CreateWindow(L"BUTTON", L"后台运行",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            65, 235, 120, 30, hwnd, (HMENU)1, GetModuleHandle(NULL), NULL);

        // 创建退出按钮
        g_hExitButton = CreateWindow(L"BUTTON", L"退出",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            200, 235, 120, 30, hwnd, (HMENU)2, GetModuleHandle(NULL), NULL);

        // 统一设置字体
        SetFont(g_hEditBox, 18);
        SetFont(g_hStepEdit, 18);
        SetFont(g_hBackgroundButton, 18);
        SetFont(g_hExitButton, 18);
        SetFont(GetDlgItem(hwnd, 3), 18); // 步长输入框
        SetFont(GetDlgItem(hwnd, 0), 20); // 静态文本

        // 注册快捷键
        if (!RegisterHotKeys(hwnd)) {
            MessageBox(hwnd, L"无法注册快捷键！", L"错误", MB_ICONERROR);
            PostQuitMessage(0);
        }

        // 设置窗口图标和标题
        HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(101));
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SetWindowText(hwnd, windowTitle);

        // 禁止窗口拉伸
        SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_SIZEBOX);

        InstallMouseHook();
        break;
    }
    case WM_HOTKEY: {
        HWND foregroundWindow = GetForegroundWindow();
        if (foregroundWindow) {
            int transparency = 100 - (static_cast<int>(wParam) * 10);
            SetWindowTransparency(foregroundWindow, transparency);
            LogWindowTransparency(foregroundWindow, transparency);
        }
        break;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == 1) { // 后台运行
            RunInBackground(hwnd);
        }
        else if (LOWORD(wParam) == 2) { // 退出
            PostQuitMessage(0);
        }
        break;
    }
    case WM_DESTROY: { // 清理资源
        for (int i = 0; i <= 9; i++) UnregisterHotKey(hwnd, i);
        RemoveMouseHook();
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// 主函数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    if (IsAnotherInstanceRunning()) {
        MessageBox(NULL, L"程序已在运行！", L"提示", MB_OK);
        return 0;
    }

    INITCOMMONCONTROLSEX icc = { sizeof(INITCOMMONCONTROLSEX), ICC_WIN95_CLASSES };
    InitCommonControlsEx(&icc);

    const wchar_t* CLASS_NAME = L"TransparencyApp";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClass(&wc);

    // 创建窗口
    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, windowTitle,
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 400, 325,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        MessageBox(NULL, L"无法创建窗口！", L"错误", MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
