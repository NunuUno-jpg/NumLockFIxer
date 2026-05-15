
# NumLock Fixer

**13 lines of code to solve a 40-year-old Windows keyboard problem**

[![License: AGPL v3](https://img.shields.io/badge/License-AGPL%20v3-blue.svg)](https://www.gnu.org/licenses/agpl-3.0)

## The Problem

In 1981, IBM designed the PC keyboard to save space by making the right-side numpad double as arrow keys when NumLock was off.

40 years later, keyboards have dedicated arrow keys, but this legacy design remains.

**When NumLock is off, pressing numpad keys moves the cursor instead of typing numbers** — a problem that has plagued billions of Windows users for 40 years.

## The Solution

### Core Code (13 lines)

```c

#define _CRT_SECURE_NO_WARNINGS
#include<windows.h>

LRESULT CALLBACK P(int c,WPARAM w,LPARAM l) {
    if(c==HC_ACTION&&(w==WM_KEYDOWN||w==WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT*k=(KBDLLHOOKSTRUCT*)l;
        if(!(GetKeyState(VK_NUMLOCK)&1))
            if((k->vkCode>=VK_NUMPAD0&&k->vkCode<=VK_NUMPAD9)||
                k->vkCode==VK_DECIMAL||k->vkCode==VK_INSERT||
                k->vkCode==VK_DELETE||k->vkCode==VK_HOME||
                k->vkCode==VK_END||k->vkCode==VK_PRIOR||
                k->vkCode==VK_NEXT||k->vkCode==VK_UP||
                k->vkCode==VK_DOWN||k->vkCode==VK_LEFT||
                k->vkCode==VK_RIGHT||k->vkCode==VK_CLEAR)
                return 1;
    }
    return CallNextHookEx(0,c,w,l);
}

int main() {
   FreeConsole(); SetWindowsHookEx(WH_KEYBOARD_LL,P,0,0);
    MSG m;
    while(GetMessage(&m,0,0,0)) {
        TranslateMessage(&m);
        DispatchMessage(&m);
    }
    return 0;
}

```

Files

File Description Lines
core.c Core solution (minimal implementation) 13
NumLockFixer.c Full-featured tool (menu / install / uninstall / registry mode) ~250

Full Version Features

=== NumLock Fixer ===
1. Hook mode, disable numpad keys when NumLock is off (run once)
2. Hook mode, disable numpad keys when NumLock is off (run once + auto-start)
3. Uninstall hook mode + disable auto-start (auto restart)
4. Registry mode (permanently disable NumLock when off, auto restart)
5. Restore registry mode (restore NumLock key function, auto restart)

Option Function Admin Required
1 Hook mode (run once) ❌
2 Hook mode + install to C:\ + auto-start ✅
3 Uninstall + disable auto-start + restart ✅
4 Registry mode (disable NumLock permanently) + restart ✅
5 Restore registry mode + restart ✅

Compilation

Full Version


gcc -o NumLockFixer.exe NumLockFixer.c -luser32 -ladvapi32 -lshell32


Core Version

gcc -o core.exe core.c -luser32


Usage

1. Download NumLockFixer.exe
2. Run as administrator (required for options 2/3/4/5)
3. Select desired option
4. Restart (options 3/4/5) or start working immediately (options 1/2)

 
