#include <Windows.h>
#include <windows.h>
#include <TlHelp32.h>
#include <cstring> // 添加 cstring 头文件
#include <tchar.h>

void AutoPowerOn()
{
    HKEY hKey;
    //std::string strRegPath = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
 
 
    //1、找到系统的启动项  
    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) ///打开启动项       
    {
        //2、得到本程序自身的全路径
        TCHAR strExeFullDir[MAX_PATH];
        GetModuleFileName(NULL, strExeFullDir, MAX_PATH);
 
 
        //3、判断注册表项是否已经存在
        TCHAR strDir[MAX_PATH] = {};
        DWORD nLength = MAX_PATH;
        long result = RegGetValue(hKey, nullptr, _T("WindowsBetter"), RRF_RT_REG_SZ, 0, strDir, &nLength);
 
 
        //4、已经存在
        if (result != ERROR_SUCCESS || _tcscmp(strExeFullDir, strDir) != 0)
        {
            //5、添加一个子Key,并设置值，"WindowsBetter"是应用程序名字（不加后缀.exe） 
            RegSetValueEx(hKey, _T("WindowsBetter"), 0, REG_SZ, (LPBYTE)strExeFullDir, (lstrlen(strExeFullDir) + 1)*sizeof(TCHAR));
 
            //6、关闭注册表
            RegCloseKey(hKey);
        }
    }
}

const wchar_t* kMutexName = L"Global\\MonitorMutex";
const wchar_t* kWatermarkProcessName = L"WindowsBetter.exe";

HANDLE FindOrStartWatermark() {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return NULL;
    
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W); // 正确初始化结构体大小
    bool found = false;
    HANDLE hWatermark = NULL;
    
    if (Process32FirstW(snapshot, &pe)) {
        do {
            if (wcscmp(pe.szExeFile, kWatermarkProcessName) == 0) {
                hWatermark = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
                if (hWatermark) {
                    found = true;
                    break;
                }
            }
        } while (Process32NextW(snapshot, &pe));
    }
    CloseHandle(snapshot);

    if (!found) {
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        
        wchar_t cmd[MAX_PATH];
        GetModuleFileNameW(NULL, cmd, MAX_PATH);
        wchar_t* lastSlash = wcsrchr(cmd, L'\\');
        if (lastSlash) {
            *(lastSlash + 1) = L'\0';
            wcscat_s(cmd, MAX_PATH, kWatermarkProcessName);
        }
        
        if (CreateProcessW(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hThread);
            hWatermark = pi.hProcess;
        }
    }
    return hWatermark;
}

int main() {
    FreeConsole(); // 隐藏控制台窗口

    AutoPowerOn();
    
    // 防止多实例
    HANDLE hMutex = CreateMutexW(NULL, TRUE, kMutexName);
    if (GetLastError() == ERROR_ALREADY_EXISTS) return 0;
    
    // 守护循环
    while (true) {
        HANDLE hWatermark = FindOrStartWatermark();
        if (hWatermark) {
            WaitForSingleObject(hWatermark, INFINITE);
            CloseHandle(hWatermark);
        }
        Sleep(100);
    }
    
    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
    return 0;
}