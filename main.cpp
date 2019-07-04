

#include <iostream>
#include <Windows.h>
#include <thread>
#include <chrono>

#define _WIN32_WINNT 0x0400
#pragma comment(lib, "user32.lib")

using namespace std;

bool isActive = true, isCurrentWindow = false, isAFK = false;
bool isSendingKey = false;

auto activeTime = std::chrono::system_clock::now();

LPCSTR Target_window_Name = "Grand Theft Auto V"; //<- Has to match window name
HWND hWindowHandle = nullptr;

HHOOK hKeyboardHook;

__declspec(dllexport) LRESULT CALLBACK KeyboardEvent(int nCode, WPARAM wParam, LPARAM lParam)
{
    DWORD SHIFT_key = 0;
    DWORD CTRL_key = 0;
    DWORD ALT_key = 0;

    if ((nCode == HC_ACTION) && ((wParam == WM_SYSKEYDOWN) || (wParam == WM_KEYDOWN)))
    {
        KBDLLHOOKSTRUCT hooked_key = *((KBDLLHOOKSTRUCT *)lParam);
        DWORD dwMsg = 1;
        dwMsg += hooked_key.scanCode << 16;
        dwMsg += hooked_key.flags << 24;
        char lpszKeyName[1024] = {0};

        int i = GetKeyNameText(dwMsg, (lpszKeyName + 1), 0xFF) + 1;

        int key = hooked_key.vkCode;

        SHIFT_key = GetAsyncKeyState(VK_SHIFT);
        CTRL_key = GetAsyncKeyState(VK_CONTROL);
        ALT_key = GetAsyncKeyState(VK_MENU);
        if(!isSendingKey) activeTime = std::chrono::system_clock::now();

        // F6
        if (key == 117)
        {
            isActive = !isActive;
            auto str = isActive ? "ACTIVE" : "NOT ACTIVE";
            cout << str << endl;
            SetConsoleTitle((str));
            
            SHIFT_key = 0;
            CTRL_key = 0;
            ALT_key = 0;
        }
    }
    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

void MessageLoop()
{
    MSG message;
    while (GetMessage(&message, NULL, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
}

DWORD WINAPI my_HotKey(LPVOID lpParm)
{
    HINSTANCE hInstance = GetModuleHandle(NULL);
    if (!hInstance)
        hInstance = LoadLibrary((LPCSTR)lpParm);
    if (!hInstance)
        return 1;

    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)KeyboardEvent, hInstance, NULL);
    MessageLoop();
    UnhookWindowsHookEx(hKeyboardHook);
    return 0;
}

void SendKey()
{
    if (hWindowHandle != nullptr)
    {
        isSendingKey = true;
        INPUT input;
        input.type = INPUT_KEYBOARD;
        input.ki.wScan = 0x0c; // hardware scan code for key
        input.ki.time = 0;
        input.ki.dwExtraInfo = 0;
        input.ki.dwFlags = KEYEVENTF_SCANCODE;

        INPUT input0;
        input0.type = INPUT_KEYBOARD;
        input0.ki.wScan = 0xE0; // hardware scan code for key
        input0.ki.time = 0;
        input0.ki.dwExtraInfo = 0;
        input0.ki.dwFlags = KEYEVENTF_SCANCODE; // 0 for key press

        //INPUT inputs[2] = {input0, input};
        SendInput(1, &input, sizeof(INPUT));

        cout << "Sent key..." << endl;
        isSendingKey = false;
    }
}

int main(int argc, char** argv)
{
    while (hWindowHandle == nullptr)
    {
        hWindowHandle = FindWindow(NULL, Target_window_Name);
        cout << "Trying to locate " << Target_window_Name << "..." << endl;
        Sleep(1000);
    }

    HANDLE hThread;
    DWORD dwThread;

    hThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)my_HotKey, (LPVOID)argv[0], NULL, &dwThread);

    /* uncomment to hide console window */
    //ShowWindow(FindWindowA("ConsoleWindowClass", NULL), false);

    cout << "Successfully located " << Target_window_Name << endl;
    cout << "Press F6 to toggle ANTI-AFK" << endl;

    while (true)
    {
        // Wait 5 minutes
        
        auto curWindow = GetForegroundWindow();
        bool isCurrentWindowNew = curWindow == hWindowHandle;
        if(!isCurrentWindowNew && isCurrentWindow)
        {
            cout << "Game window is inactive." << endl;
        }
        else if(isCurrentWindowNew && !isCurrentWindow)
        {
            cout << "Game window is active." << endl;
        }
        isCurrentWindow = isCurrentWindowNew;
        // Call our method
        int elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>
                             (std::chrono::system_clock::now()-activeTime).count();
        if(elapsed_seconds >= 30)
        {
            isAFK = true;
        }
        else
        {
            isAFK = false;
        }

        auto str = isActive ? (!isCurrentWindow ? "GAME IS INACTIVE" : (isAFK ? "ACTIVE" : "USER IS NOT AFK")) : "NOT ACTIVE";
        SetConsoleTitle((str));
        if(isActive && isAFK)
        {
            if(isCurrentWindow)
            {
                SendKey();
                Sleep(10000);
            }
        }
        
    }
    if (hThread)
        return WaitForSingleObject(hThread, INFINITE);
    // Execution continues ...
    std::cin.get();
}