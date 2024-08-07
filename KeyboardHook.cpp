#include <iostream>
#include <windows.h>
#include <winuser.h>
#include <unordered_set>

using namespace std;

// 用于接收WM_INPUT的隐藏窗口的句柄
HWND myWindow;
// 当前窗口的句柄
HWND prevWindow;
// 注册原始输入设备
VOID RegKeyboardRawInput(HWND myWindow);
// 自定义消息函数
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
// 维护按下按键的表 防止按住时持续输出
unordered_set<unsigned short> recordedKeys;

VOID RegKeyboardRawInput(HWND myWindow){
    RAWINPUTDEVICE rawInputDevice[1];
    // 设备类
    rawInputDevice[0].usUsagePage = 0x01;
    // 获取键盘的原始输入源
    rawInputDevice[0].usUsage = 0x06;
    // 即使窗口失去焦点位置，仍然接收输入消息
    rawInputDevice[0].dwFlags = RIDEV_INPUTSINK;
    // 用于接收WM_INPUT的窗口句柄
    rawInputDevice[0].hwndTarget = myWindow;
    // 注册
    if (RegisterRawInputDevices(rawInputDevice, 1, sizeof(rawInputDevice[0])) == FALSE) cout << "register failed." << endl;
}

LRESULT CALLBACK WindowProc(_In_  HWND hwnd,_In_  UINT uMsg,_In_  WPARAM wParam,_In_  LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_COMMAND:
            break;
        case WM_PAINT:
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        case WM_INPUT:{
			BYTE keyState[256];
			unsigned char Key[1] = {'\0'};
			UINT dwSize = '\0';
            // 获取缓冲区大小
            GetRawInputData((HRAWINPUT)lParam, (UINT)RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
            // 分配缓冲区
            LPBYTE lpbBuffer = new BYTE[dwSize];
            // 取数据到缓冲区
            GetRawInputData((HRAWINPUT)lParam, (UINT)RID_INPUT, (LPVOID)lpbBuffer, (PUINT)&dwSize, (UINT)sizeof(RAWINPUTHEADER));
            RAWINPUT* raw = (RAWINPUT *)lpbBuffer;
            //获取所有键盘输入
            if (raw->header.dwType == RIM_TYPEKEYBOARD){
                //按下
                if (raw->data.keyboard.Message == WM_KEYDOWN){
                    //按键去重
                    if (recordedKeys.find(raw->data.keyboard.VKey) == recordedKeys.end()) {
                        // vk转ascii
                        ToAscii(raw->data.keyboard.VKey, raw->data.keyboard.MakeCode, keyState, (LPWORD)Key, 0);
                        //按键去重
                        recordedKeys.insert(raw->data.keyboard.VKey);
                        //输出
                        cout << "KEY_DOWN: " << Key << endl;
                    }
                }
				//释放
				if (raw->data.keyboard.Message == WM_KEYUP){
                    ToAscii(raw->data.keyboard.VKey, raw->data.keyboard.MakeCode, keyState, (LPWORD)Key, 0);
                    recordedKeys.erase(raw->data.keyboard.VKey);
                    cout <<  "KEY_UP: " << Key << endl;
                }
            }
            delete[] lpbBuffer;
            return 0;
        }
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int main() {
    //隐藏窗口
    HINSTANCE hInst;
    hInst = GetModuleHandle(NULL);	//获取模块句柄
    WNDCLASSEX wcx;
    wcx.cbSize = sizeof(wcx);
    wcx.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_NOCLOSE;//窗口风格
    wcx.lpfnWndProc = WindowProc;     //自定义消息处理函数
    wcx.cbClsExtra = 0;                // no extra class memory
    wcx.cbWndExtra = 0;                // no extra window memory
    wcx.hInstance = hInst;         //当前实例句柄
    wcx.hIcon = LoadIcon(NULL,IDI_APPLICATION); //图标风格
    wcx.hCursor = LoadCursor(NULL,IDC_ARROW);  //鼠标风格
    wcx.hbrBackground = (HBRUSH)WHITE_BRUSH; //背景色
    wcx.lpszMenuName = NULL;  //菜单名
    wcx.lpszClassName = "ITSMYOSDCLASS"; //窗口类的名称
    wcx.hIconSm = NULL;

    if (!RegisterClassEx(&wcx))
    {
        cout << "RegisterClassEx failed" << endl;
        return 1;
    }

    //显示位置
    int OSDleft = GetSystemMetrics(SM_CXSCREEN) / 2 - 300;
    int OSDTop = GetSystemMetrics(SM_CYSCREEN) / 2;

    myWindow = CreateWindowEx(
            WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,//扩展风格
            wcx.lpszClassName,//上面注册的类名lpszClassName，保持一致
            NULL,
            WS_VISIBLE | WS_POPUP,//窗口风格
            OSDleft,//窗口相对于父级的坐标
            OSDTop,//窗口相对于父级的坐标
            300,//宽
            300,//高
            (HWND)NULL,//没有父窗口，(HWND)NULL//GetDesktopWindow(),
            (HMENU)NULL,//没有菜单，为ULL
            hInst,//当前应用程序的实例句柄
            (LPVOID)NULL);	//没有附加数据，NULL

    if (!myWindow)
    {
        cout << "CreateWindowEx failed" << endl;
        return 1;
    }
    RegKeyboardRawInput(myWindow);
    //消息循环 不处理会卡死
    MSG msg;
    while (GetMessage(&msg, (HWND)NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
