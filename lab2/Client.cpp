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
#define DEFAULT_PORT "27015"
#define HOSTNAME "192.168.43.240"

int LastXPos, LastYPos;
bool toPaint = false;
WSADATA wsaData;
SOCKET ConnectSocket = INVALID_SOCKET;
struct addrinfo *result = NULL,
        *ptr = NULL,
        hints;
const char *sendbuf = "this is a test";
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

int SendMsg(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    MessageToSend msg = {uMsg, wParam, lParam};
    iResult = send(ConnectSocket, (char *) (&msg), sizeof(msg), 0 );
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        fflush(stdout);
        WSACleanup();
        return 1;
    }
    do {
        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if ( iResult > 0 )
            printf("Bytes received: %d\n", iResult);
    } while( iResult > 0 );
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

            ZeroMemory( &hints, sizeof(hints) );
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;

            // Resolve the server address and port
            iResult = getaddrinfo(HOSTNAME, DEFAULT_PORT, &hints, &result);
            if ( iResult != 0 ) {
                printf("getaddrinfo failed with error: %d\n", iResult);
                WSACleanup();
                return 1;
            }
            // Attempt to connect to an address until one succeeds
            for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {

                // Create a SOCKET for connecting to server
                ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
                                       ptr->ai_protocol);
                if (ConnectSocket == INVALID_SOCKET) {
                    printf("socket failed with error: %ld\n", WSAGetLastError());
                    WSACleanup();
                    return 1;
                }

                // Connect to server.
                iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
                if (iResult == SOCKET_ERROR) {
                    closesocket(ConnectSocket);
                    ConnectSocket = INVALID_SOCKET;
                    continue;
                }
                break;
            }
            freeaddrinfo(result);
            if (ConnectSocket == INVALID_SOCKET) {
                printf("Unable to connect to server!\n");
                fflush(stdout);
                WSACleanup();
                return 1;
            }
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