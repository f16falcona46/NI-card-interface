// NILineScanReader.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "NILineScanReader.h"
#include <cstdint>
#include "niimaq.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void setup(int num_lines);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_NILINESCANREADER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_NILINESCANREADER));

    MSG msg;

	setup(100);

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

void setup(int num_lines) {
	const char* interface_name = "img0"; //which camera
	const int image_width = 512; //image size
	const int image_height = 1;

	unsigned int bytes_per_pixel;
	unsigned int* skip_count_buffers = new unsigned int[num_lines](); //zero initialize (we aren't skipping anything)

	INTERFACE_ID interface_ID = 0;
	SESSION_ID session_ID = 0;
	imgInterfaceOpen(interface_name, &interface_ID); //open interface and session
	imgSessionOpen(interface_ID, &session_ID);

	imgSetAttribute2(session_ID, IMG_ATTR_ROI_WIDTH, image_width); //set image width and height to get
	imgSetAttribute2(session_ID, IMG_ATTR_ROI_HEIGHT, image_height);
	imgSetAttribute2(session_ID, IMG_ATTR_ROWPIXELS, image_width);

	BUFLIST_ID buflist_ID = 0;
	imgCreateBufList(num_lines, &buflist_ID); //make a buffer list
	imgGetAttribute(session_ID, IMG_ATTR_BYTESPERPIXEL, &bytes_per_pixel); //make the buffer and pointers to it
	unsigned int buf_size = image_width * image_height * bytes_per_pixel;

	uint16_t** line_buffers_ptrs = new uint16_t*[num_lines];
	uint16_t* line_buffers = new uint16_t[buf_size * num_lines];

	for (int i = 0; i < num_lines; ++i) {
		line_buffers_ptrs[i] = line_buffers + i*buf_size;
		imgSetBufferElement2(buflist_ID, i, IMG_BUFF_ADDRESS, line_buffers_ptrs[i]); //register the ptr to the buffer
		imgSetBufferElement2(buflist_ID, i, IMG_BUFF_SIZE, buf_size);
		unsigned int buf_cmd = (i == num_lines - 1) ? IMG_CMD_STOP : IMG_CMD_NEXT;
		imgSetBufferElement2(buflist_ID, i, IMG_BUFF_COMMAND, buf_cmd);
		imgSetBufferElement2(buflist_ID, i, IMG_BUFF_SKIPCOUNT, skip_count_buffers[i]);
	}

	imgMemLock(buflist_ID);
	imgSessionConfigure(session_ID, buflist_ID);

	//TODO: triggering
	//TODO: imgSequenceSetup
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_NILINESCANREADER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_NILINESCANREADER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
