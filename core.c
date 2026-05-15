
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
