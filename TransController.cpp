#include <Windows.h>
#include <string>
#include <sstream>
#include <commctrl.h>  // 必须包含此头文件用于按钮控件和托盘图标

// 全局变量
HWND g_hEditBox; // 文本框句柄
HWND g_hExitButton; // 退出按钮句柄
HWND g_hBackgroundButton; // 后台运行按钮句柄
int currentTransparency = 100;  // 当前透明度，默认100%

// 设置窗口透明度
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
        if (!RegisterHotKey(hwnd, i, MOD_ALT | MOD_SHIFT, 0x30 + i)) { // 使用 Alt + Shift + 数字键
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

// 设置按钮字体为微软雅黑
void SetButtonFont(HWND hWnd) {
    HFONT hFont = CreateFont(
        18, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        FIXED_PITCH | FF_DONTCARE, L"微软雅黑"
    );
    SendMessage(hWnd, WM_SETFONT, (WPARAM)hFont, TRUE);
}

// 后台运行功能
void RunInBackground(HWND hwnd) {
    // 将窗口最小化并隐藏
    ShowWindow(hwnd, SW_HIDE);
}

// 处理鼠标滚轮调整透明度
void HandleMouseWheel(HWND hwnd, int delta) {
    if (GetAsyncKeyState(VK_MENU) & 0x8000 && GetAsyncKeyState(VK_SHIFT) & 0x8000) {  // Alt + Shift 按下时
        // 如果滚轮向上，增加透明度；向下则减少透明度
        if (delta > 0 && currentTransparency < 100) {
            currentTransparency += 2;  // 增加透明度步长改为2
        }
        else if (delta < 0 && currentTransparency > 0) {
            currentTransparency -= 2;  // 减少透明度步长改为2
        }

        // 限制透明度范围在 0 到 100 之间
        currentTransparency = max(0, min(100, currentTransparency));

        // 调整当前窗口的透明度
        SetWindowTransparency(hwnd, currentTransparency);

        // 在文本框中显示日志
        std::wstringstream ss;
        ss << L"当前透明度: " << currentTransparency << L"%\n";
        std::wstring log = ss.str();
        SendMessage(g_hEditBox, EM_SETSEL, -1, -1);
        SendMessage(g_hEditBox, EM_REPLACESEL, 0, (LPARAM)log.c_str());
    }
}

// 窗口过程函数
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // 创建文本框
        g_hEditBox = CreateWindowEx(
            WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            10, 10, 360, 200, hwnd, NULL, GetModuleHandle(NULL), NULL
        );

        // 创建后台运行按钮
        g_hBackgroundButton = CreateWindow(L"BUTTON", L"后台运行",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            10, 220, 120, 30, hwnd, (HMENU)1, GetModuleHandle(NULL), NULL);

        // 创建退出按钮
        g_hExitButton = CreateWindow(L"BUTTON", L"退出",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            140, 220, 120, 30, hwnd, (HMENU)2, GetModuleHandle(NULL), NULL);

        // 设置字体为微软雅黑
        HFONT hFont = CreateFont(
            20, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            FIXED_PITCH | FF_DONTCARE, L"微软雅黑"
        );
        SendMessage(g_hEditBox, WM_SETFONT, (WPARAM)hFont, TRUE);

        // 设置按钮字体为微软雅黑
        SetButtonFont(g_hBackgroundButton);
        SetButtonFont(g_hExitButton);

        // 注册快捷键
        if (!RegisterHotKeys(hwnd)) {
            MessageBox(hwnd, L"无法注册快捷键！", L"错误", MB_ICONERROR);
            PostQuitMessage(0);
        }

        // 设置窗口图标
        HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(101)); // 需要准备一个资源ID为101的图标
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

        // 设置窗口标题
        SetWindowText(hwnd, L"透明度控制者v1.2 作者：ICER233");

        break;
    }
    case WM_HOTKEY: {
        HWND foregroundWindow = GetForegroundWindow();
        if (foregroundWindow) {
            int transparency = 100 - (static_cast<int>(wParam) * 10);
            SetWindowTransparency(foregroundWindow, transparency);

            // 在文本框中显示日志
            std::wstringstream ss;
            ss << L"已将[" << foregroundWindow << L"]的透明度设为 " << transparency << L"%\n";
            std::wstring log = ss.str();
            SendMessage(g_hEditBox, EM_SETSEL, -1, -1);
            SendMessage(g_hEditBox, EM_REPLACESEL, 0, (LPARAM)log.c_str());
        }
        break;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == 1) { // 后台运行按钮
            RunInBackground(hwnd);
        }
        else if (LOWORD(wParam) == 2) { // 退出按钮
            PostQuitMessage(0);
        }
        break;
    }
    case WM_MOUSEWHEEL: {
        HandleMouseWheel(hwnd, GET_WHEEL_DELTA_WPARAM(wParam));
        break;
    }
    case WM_DESTROY: {
        // 注销快捷键
        for (int i = 0; i <= 9; i++) {
            UnregisterHotKey(hwnd, i);
        }
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// 检查程序是否已在运行
bool IsAnotherInstanceRunning() {
    HANDLE hMutex = CreateMutex(NULL, TRUE, L"TransparencyAppMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // 如果互斥体已经存在，说明程序已经在运行
        CloseHandle(hMutex);
        return true;
    }
    return false;
}

// 主函数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 检测程序是否已在运行
    if (IsAnotherInstanceRunning()) {
        // 如果程序已经在运行，找到它的窗口并显示
        HWND hwndExisting = FindWindow(L"TransparencyApp", L"透明度控制者v1.2 作者：ICER233");
        if (hwndExisting) {
            // 激活并显示窗口
            ShowWindow(hwndExisting, SW_RESTORE);
            SetForegroundWindow(hwndExisting);
        }
        return 0;  // 当前实例退出
    }

    // 初始化Common Controls
    INITCOMMONCONTROLSEX icc = { sizeof(INITCOMMONCONTROLSEX), ICC_WIN95_CLASSES };
    InitCommonControlsEx(&icc);  // 调用 InitCommonControlsEx 函数

    // 注册窗口类
    const wchar_t* CLASS_NAME = L"TransparencyApp";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClass(&wc);

    // 创建窗口
    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, L"透明度控制者v1.2 作者：ICER233",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        NULL, NULL, hInstance, NULL
    );
    if (!hwnd) {
        MessageBox(NULL, L"无法创建窗口！", L"错误", MB_ICONERROR);
        return 1;
    }

    // 显示窗口
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
