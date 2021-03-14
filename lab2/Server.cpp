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
#include <thread>

using namespace Gdiplus;

#define DEFAULT_BUFLEN 8196
#define DEFAULT_PORT "27015"
#define HOSTNAME "localhost"

int LastXPos, LastYPos;
bool toPaint = false;
WSADATA wsaData;
int iResult;

SOCKET ListenSocket = INVALID_SOCKET;
SOCKET ClientSocket = INVALID_SOCKET;

struct addrinfo *result = NULL;
struct addrinfo hints;

std::thread recvthr;
bool StopRecieve = false;
int iSendResult;
char recvbuf[DEFAULT_BUFLEN];
int recvbuflen = DEFAULT_BUFLEN;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
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
            TEXT("Server"),  // window caption
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

int GetMessageTCP(HWND hwnd) {
    freopen("server.log", "w", stdout);
    while (!StopRecieve) {
        // Receive until the peer shuts down the connection
        do {
            iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
            if (iResult > 0) {
                printf("Bytes received: %d\n", iResult);

                // Echo the buffer back to the sender
                iSendResult = send(ClientSocket, recvbuf, iResult, 0);
                if (iSendResult == SOCKET_ERROR) {
                    printf("send failed with error: %d\n", WSAGetLastError());
                    closesocket(ClientSocket);
                    WSACleanup();
                    return 1;
                }
                printf("Bytes sent: %d\n", iSendResult);
                for (auto *now = (MessageToSend *) recvbuf; now != (MessageToSend *) (recvbuf + iResult); now++) {
                    switch (now->uMsg) {
                        case WM_LBUTTONDOWN:
                            printf("LBUTTONDOWN\n");
                            break;
                        case WM_MOUSEMOVE:
                            printf("MOUSEMOVE\n");
                            break;
                        case WM_LBUTTONUP:
                            printf("LBUTTONUP\n");
                            break;
                    }
                    SendMessage(hwnd, now->uMsg, now->wParam, now->lParam);
                    fflush(stdout);
                }
            }
        } while (iResult > 0);
    }
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
            if (iResult != 0) {
                printf("WSAStartup failed with error: %d\n", iResult);
                return 1;
            }

            ZeroMemory(&hints, sizeof(hints));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;
            hints.ai_flags = AI_PASSIVE;

            // Resolve the server address and port
            iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
            if ( iResult != 0 ) {
                printf("getaddrinfo failed with error: %d\n", iResult);
                WSACleanup();
                return 1;
            }

            // Create a SOCKET for connecting to server
            ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
            if (ListenSocket == INVALID_SOCKET) {
                printf("socket failed with error: %ld\n", WSAGetLastError());
                freeaddrinfo(result);
                WSACleanup();
                return 1;
            }

            // Setup the TCP listening socket
            iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
            if (iResult == SOCKET_ERROR) {
                printf("bind failed with error: %d\n", WSAGetLastError());
                freeaddrinfo(result);
                closesocket(ListenSocket);
                WSACleanup();
                return 1;
            }
            iResult = listen(ListenSocket, SOMAXCONN);
            if (iResult == SOCKET_ERROR) {
                printf("listen failed with error: %d\n", WSAGetLastError());
                closesocket(ListenSocket);
                WSACleanup();
                return 1;
            }
            // Accept a client socket
            ClientSocket = accept(ListenSocket, NULL, NULL);
            if (ClientSocket == INVALID_SOCKET) {
                printf("accept failed with error: %d\n", WSAGetLastError());
                closesocket(ListenSocket);
                WSACleanup();
                return 1;
            }
            DWORD nonBlocking = 1;
            ioctlsocket( ListenSocket, FIONBIO, &nonBlocking);
            freeaddrinfo(result);

            recvthr = std::thread(GetMessageTCP, hwnd);
            return 0;
        }
        case WM_DESTROY:
            // cleanup
            StopRecieve = true;
            recvthr.detach();
            closesocket(ClientSocket);
            closesocket(ListenSocket);
            WSACleanup();
            PostQuitMessage(0);
            return 0;
        case WM_LBUTTONDOWN: {
            LastXPos = LOWORD(lParam) + DELTAX;
            LastYPos = HIWORD(lParam) + DELTAY;
            toPaint = true;
            return 0;
        }
        case WM_LBUTTONUP: {
            toPaint = false;
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