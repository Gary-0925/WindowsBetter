#include <Windows.h>
#include <string>
#include <TlHelp32.h>
#include <cstring> // 添加 cstring 头文件

// 全局变量
const wchar_t* kMutexName = L"Global\\WatermarkMutex";
const wchar_t* kMonitorProcessName = L"Wmonitor.exe";

HANDLE FindOrStartMonitor() {
    // 检查监控进程是否存在
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return NULL;
    
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W); // 正确初始化结构体大小
    bool found = false;
    HANDLE hMonitor = NULL;
    
    if (Process32FirstW(snapshot, &pe)) {
        do {
            if (wcscmp(pe.szExeFile, kMonitorProcessName) == 0) {
                hMonitor = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
                if (hMonitor) {
                    found = true;
                    break;
                }
            }
        } while (Process32NextW(snapshot, &pe));
    }
    CloseHandle(snapshot);

    // 启动监控程序
    if (!found) {
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        
        wchar_t cmd[MAX_PATH];
        GetModuleFileNameW(NULL, cmd, MAX_PATH);
        wchar_t* lastSlash = wcsrchr(cmd, L'\\');
        if (lastSlash) {
            *(lastSlash + 1) = L'\0';
            wcscat_s(cmd, MAX_PATH, kMonitorProcessName);
        }
        
        if (CreateProcessW(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hThread);
            hMonitor = pi.hProcess;
        }
    }
    return hMonitor;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            // 创建大号字体
            HFONT hFont = CreateFontW(
                48, 0, 0, 0, FW_BOLD, // 高度48，加粗
                FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                // 清晰渲染
                ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                L"Arial"
            );
            HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
            
            // 设置透明背景
            RECT rect;
            GetClientRect(hWnd, &rect);
            SetBkMode(hdc, TRANSPARENT);

            const int margin = 20;
            RECT textRect = {
                rect.right - 1500,
                rect.bottom - 120,
                rect.right - margin,
                rect.bottom - margin
            };
            
            // 绘制水印文本
            const wchar_t* text = L"Windows优化专家已优化完成！请联系洛谷用户1202669付费激活后继续使用。";
            //SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(200, 200, 200));
            DrawTextW(hdc, text, -1, &textRect, DT_VCENTER | DT_SINGLELINE);
            DrawTextW(hdc, text, -1, &rect, DT_LEFT | DT_TOP | DT_VCENTER);
            DrawTextW(hdc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            
            SelectObject(hdc, hOldFont);
            DeleteObject(hFont);

            EndPaint(hWnd, &ps);
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}

DWORD WINAPI MonitorThread(LPVOID lpParam) {
    while (true) {
        Sleep(100);
        HANDLE hMonitor = FindOrStartMonitor();
        if (hMonitor) {
            WaitForSingleObject(hMonitor, INFINITE);
            CloseHandle(hMonitor);
        }
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    FreeConsole(); // 隐藏控制台窗口
    
    // 防止多实例
    HANDLE hMutex = CreateMutexW(NULL, TRUE, kMutexName);
    if (GetLastError() == ERROR_ALREADY_EXISTS) return 0;
    
    // 启动监控程序
    HANDLE hMonitor = FindOrStartMonitor();
    
    // 创建透明窗口
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"WatermarkWindow";
    if (!RegisterClassExW(&wc)) return 0;
    
    HWND hWnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
        wc.lpszClassName,
        L"Watermark",
        WS_POPUP,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, hInstance, NULL
    );
    
    if (!hWnd) return 0;
    
    // 设置窗口透明度
    SetLayeredWindowAttributes(hWnd, 0, 160, LWA_COLORKEY | LWA_ALPHA);
    ShowWindow(hWnd, SW_SHOW);
    
    // 监控线程
    HANDLE hThread = CreateThread(NULL, 0, MonitorThread, NULL, 0, NULL);
    
    // 主消息循环
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    
    if (hThread) CloseHandle(hThread);
    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
    return 0;
}