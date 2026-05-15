#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <shellapi.h>
#include <shlobj.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")

#define APP_NAME "NumLockFixer"
#define REG_RUN_PATH "Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define INSTALL_PATH "C:\\Program Files\\NumLockFixer\\NumLockFixer.exe"

HHOOK g_hook = NULL;
HINSTANCE g_hInst = NULL;

// Check if current process has administrator privileges
int IsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    AllocateAndInitializeSid(&NtAuthority, 2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &adminGroup);
    CheckTokenMembership(NULL, adminGroup, &isAdmin);
    FreeSid(adminGroup);
    return isAdmin;
}

// Relaunch the program with administrator privileges
void RelaunchAsAdmin(char* arg) {
    char path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);
    ShellExecute(NULL, "runas", path, arg, NULL, SW_HIDE);
}

// Restart the system immediately
void RestartSystem() {
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    LUID luid;
    
    OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &luid);
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, 0);
    
    ExitWindowsEx(EWX_REBOOT, 0x00000000);
}

// Low-level keyboard hook procedure that blocks numpad keys when NumLock is off
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT* k = (KBDLLHOOKSTRUCT*)lParam;
        BOOL numlock = (GetKeyState(VK_NUMLOCK) & 1);
        
        if (!numlock) {
            if (k->vkCode >= VK_NUMPAD0 && k->vkCode <= VK_NUMPAD9)
                return 1;
            if (k->vkCode == VK_DECIMAL)
                return 1;
            if (k->vkCode == VK_INSERT || k->vkCode == VK_DELETE ||
                k->vkCode == VK_HOME || k->vkCode == VK_END ||
                k->vkCode == VK_PRIOR || k->vkCode == VK_NEXT ||
                k->vkCode == VK_UP || k->vkCode == VK_DOWN ||
                k->vkCode == VK_LEFT || k->vkCode == VK_RIGHT ||
                k->vkCode == VK_CLEAR)
                return 1;
        }
    }
    return CallNextHookEx(g_hook, nCode, wParam, lParam);
}

// Install keyboard hook and enter message loop
void RunHook() {
    g_hook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, g_hInst, 0);
    if (!g_hook) return;
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

// Remove startup registry entry
void RemoveRegistryStartup() {
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, REG_RUN_PATH, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegDeleteValue(hKey, APP_NAME);
        RegCloseKey(hKey);
    }
}

// Delete installed program files
void DeleteInstalledFiles() {
    system("rmdir /s /q \"C:\\Program Files\\NumLockFixer\"");
}

// Enable registry mode: permanently disable numpad keys via scancode mapping
void EnableRegistryMode() {
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Control\\Keyboard Layout",
        0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        
        BYTE map[] = {
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x02, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x45, 0x00,
            0x00, 0x00, 0x00, 0x00
        };
        RegSetValueEx(hKey, "Scancode Map", 0, REG_BINARY, map, sizeof(map));
        RegCloseKey(hKey);
    }
}

// Disable registry mode: remove scancode mapping to restore normal numpad behavior
void DisableRegistryMode() {
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Control\\Keyboard Layout",
        0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        
        RegDeleteValue(hKey, "Scancode Map");
        RegCloseKey(hKey);
    }
}

// Hook mode: run once without installation
void DoHookMode() {
    FreeConsole();
    RunHook();
}

// Auto-start hook mode: background execution for startup
void DoHookModeAuto() {
    FreeConsole();
    RunHook();
}

// Install to C:\Program Files and add auto-start
void DoInstallAndRun() {
    if (!IsAdmin()) {
        FreeConsole();
        RelaunchAsAdmin("--install");
        return;
    }
    
    char srcPath[MAX_PATH];
    GetModuleFileName(NULL, srcPath, MAX_PATH);
    
    CreateDirectory("C:\\Program Files\\NumLockFixer", NULL);
    
    CopyFile(srcPath, INSTALL_PATH, FALSE);
    
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, REG_RUN_PATH, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        char cmdLine[MAX_PATH * 2];
        snprintf(cmdLine, sizeof(cmdLine), "\"%s\" --auto", INSTALL_PATH);
        RegSetValueEx(hKey, APP_NAME, 0, REG_SZ, (BYTE*)cmdLine, strlen(cmdLine) + 1);
        RegCloseKey(hKey);
    }
    
    FreeConsole();
    RunHook();
}

// Uninstall: remove auto-start and delete files, then restart
void DoUninstallAndRestart() {
    if (!IsAdmin()) {
        FreeConsole();
        RelaunchAsAdmin("--uninstall");
        return;
    }
    
    RemoveRegistryStartup();
    DeleteInstalledFiles();
    RestartSystem();
}

// Enable registry mode and restart
void DoEnableRegistryAndRestart() {
    if (!IsAdmin()) {
        FreeConsole();
        RelaunchAsAdmin("--enable-registry");
        return;
    }
    EnableRegistryMode();
    RestartSystem();
}

// Disable registry mode and restart
void DoDisableRegistryAndRestart() {
    if (!IsAdmin()) {
        FreeConsole();
        RelaunchAsAdmin("--disable-registry");
        return;
    }
    DisableRegistryMode();
    RestartSystem();
}

int main(int argc, char* argv[]) {
    g_hInst = GetModuleHandle(NULL);
    
    // Command line argument handling
    if (argc > 1) {
        if (strcmp(argv[1], "--hook") == 0) {
            DoHookMode();
            return 0;
        }
        if (strcmp(argv[1], "--auto") == 0) {
            DoHookModeAuto();
            return 0;
        }
        if (strcmp(argv[1], "--install") == 0) {
            DoInstallAndRun();
            return 0;
        }
        if (strcmp(argv[1], "--uninstall") == 0) {
            DoUninstallAndRestart();
            return 0;
        }
        if (strcmp(argv[1], "--enable-registry") == 0) {
            DoEnableRegistryAndRestart();
            return 0;
        }
        if (strcmp(argv[1], "--disable-registry") == 0) {
            DoDisableRegistryAndRestart();
            return 0;
        }
    }
    
    AllocConsole();
    
    printf("=== NumLock Fixer ===\n");
    printf("1. Hook mode, disable numpad keys when NumLock is off (run once)\n");
    printf("2. Hook mode, disable numpad keys when NumLock is off (run once + auto-start)\n");
    printf("3. Uninstall hook mode + disable auto-start (auto restart)\n");
    printf("4. Registry mode (permanently disable NumLock when off, auto restart)\n");
    printf("5. Restore registry mode (restore NumLock key function, auto restart)\n");
    printf("Enter choice (1-5): ");
    
    char input[10];
    fgets(input, sizeof(input), stdin);
    int choice = atoi(input);
    
    switch (choice) {
        case 1:
            DoHookMode();
            break;
        case 2:
            DoInstallAndRun();
            break;
        case 3:
            DoUninstallAndRestart();
            break;
        case 4:
            DoEnableRegistryAndRestart();
            break;
        case 5:
            DoDisableRegistryAndRestart();
            break;
        default:
            printf("Invalid option\nPress Enter to exit...");
            getchar();
            FreeConsole();
            break;
    }
    
    return 0;
}
