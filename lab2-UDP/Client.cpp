#ifndef UNICODE
#define UNICODE
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <windowsx.h>
#include <objidl.h>
#include <gdiplus.h>
#include <cstdlib>
#include <cstdio>
using namespace Gdiplus;


#define DEFAULT_BUFLEN 8196
#define DEFAULT_PORT 27015
#define DEFAULT_HOST "192.168.43.240"

int LastXPos, LastYPos;
bool toPaint = false;
WSADATA wsaData;
SOCKET ConnectSocket = INVALID_SOCKET;
sockaddr_in addr;
char recvbuf[DEFAULT_BUFLEN];
int iResult;
int recvbuflen = DEFAULT_BUFLEN;


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    freopen("client.log", "w", stdout);
    HWND                hWnd;
    MSG                 msg;
    WNDCLASS            wndClass;
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR           gdiplusToken;

    // Initialize GDI+.
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    wndClass.style          = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc    = WindowProc;
    wndClass.cbClsExtra     = 0;
    wndClass.cbWndExtra     = 0;
    wndClass.hInstance      = hInstance;
    wndClass.hIcon          = LoadIcon(NULL, IDI_APPLICATION);
    wndClass.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground  = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndClass.lpszMenuName   = NULL;
    wndClass.lpszClassName  = TEXT("WindowClass");

    RegisterClass(&wndClass);

    hWnd = CreateWindow(
            TEXT("WindowClass"),   // window class name
            TEXT("Client"),  // window caption
            WS_OVERLAPPEDWINDOW,      // window style
            CW_USEDEFAULT,            // initial x position
            CW_USEDEFAULT,            // initial y position
            CW_USEDEFAULT,            // initial x size
            CW_USEDEFAULT,            // initial y size
            NULL,                     // parent window handle
            NULL,                     // window menu handle
            hInstance,                // program instance handle
            NULL);                    // creation parameters

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    while(GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    GdiplusShutdown(gdiplusToken);
    return msg.wParam;
}

#define DELTAX 7
#define DELTAY 30

struct MessageToSend {
    UINT uMsg;
    WPARAM wParam;
    LPARAM lParam;
};

#define server_address "127.0.0.1"
int SendMsg(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    MessageToSend msg = {uMsg, wParam, lParam};
    iResult = sendto(ConnectSocket, (char *) (&msg), sizeof(msg), 0, (sockaddr *) &addr, sizeof(addr));
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        fflush(stdout);
        WSACleanup();
        return 1;
    }
    printf("Sent %d bytes\n", iResult);
    fflush(stdout);
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // Initialize Winsock
            iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
            if (iResult != 0) {
                printf("WSAStartup failed with error: %d\n", iResult);
                return 1;
            }
            ConnectSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (ConnectSocket <= 0) {
                printf("Failed to create socket");
                return 1;
            }
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = inet_addr(DEFAULT_HOST);
            addr.sin_port = htons(DEFAULT_PORT);

            DWORD nonBlocking = 1;
            ioctlsocket( ConnectSocket, FIONBIO, &nonBlocking);
            return 0;
        }
        case WM_DESTROY:
            closesocket(ConnectSocket);
            WSACleanup();
            PostQuitMessage(0);
            return 0;
        case WM_LBUTTONDOWN: {
            LastXPos = LOWORD(lParam) + DELTAX;
            LastYPos = HIWORD(lParam) + DELTAY;
            toPaint = true;
            SendMsg(uMsg, wParam, lParam);
            return 0;
        }
        case WM_LBUTTONUP: {
            toPaint = false;
            SendMsg(uMsg, wParam, lParam);
            return 0;
        }
        case WM_MOUSEMOVE: {
            if (toPaint) {
                int xPos = LOWORD(lParam) + DELTAX;
                int yPos = HIWORD(lParam) + DELTAY;
                RECT rc;
                GetClientRect(hwnd, &rc);
                xPos += rc.left; yPos += rc.top;
                HDC hdc = GetWindowDC(hwnd);
                Graphics graphics(hdc);
                Pen pen(Color(255, 0, 0, 0));
                graphics.DrawLine(&pen, xPos, yPos, LastXPos, LastYPos);
                LastXPos = xPos;
                LastYPos = yPos;
                SendMsg(uMsg, wParam, lParam);
            }
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}