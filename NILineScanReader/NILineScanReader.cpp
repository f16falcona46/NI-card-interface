// NILineScanReader.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "NILineScanReader.h"
#include <cstdint>
#include <future>
#include <stdexcept>
#include "niimaq.h"

#define MAX_LOADSTRING 100
#define errChk(fCall) if (error = (fCall), error < 0) {throw std::runtime_error("");}

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
bool setup_buffers(int num_lines, int num_frames);
void DisplayIMAQError(Int32 error);
bool copy_ring_buffer(const SESSION_ID& session_ID, uint8_t* large_buffer, int bytes_per_image, int buffers_per_image, bool* volatile stop);

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

	setup_buffers(512, 1000000);

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

bool setup_buffers(int num_lines, int num_frames) {
	const char* interface_name = "img0"; //which camera
	const int image_width = 512; //image size
	const int image_height = 1;

	int error = 0;
	unsigned int bytes_per_pixel;
	unsigned int* skip_count_buffers = new unsigned int[num_lines](); //zero initialize (we aren't skipping anything)

	INTERFACE_ID interface_ID = 0;
	SESSION_ID session_ID = 0;
	try {
		errChk(imgInterfaceOpen(interface_name, &interface_ID)); //open interface and session
		errChk(imgSessionOpen(interface_ID, &session_ID));

		errChk(imgSetAttribute2(session_ID, IMG_ATTR_ROI_WIDTH, image_width)); //set image width and height to get
		errChk(imgSetAttribute2(session_ID, IMG_ATTR_ROI_HEIGHT, image_height));
		errChk(imgSetAttribute2(session_ID, IMG_ATTR_ROWPIXELS, image_width));

		BUFLIST_ID buflist_ID = 0;
		errChk(imgCreateBufList(num_lines, &buflist_ID)); //make a buffer list
		errChk(imgGetAttribute(session_ID, IMG_ATTR_BYTESPERPIXEL, &bytes_per_pixel)); //make the buffer and pointers to it
		unsigned int ring_buf_size = image_width * image_height * bytes_per_pixel;

		uint8_t** line_buffers_ptrs = new uint8_t*[num_lines];
		uint8_t* line_buffers = new uint8_t[ring_buf_size * num_lines];

		uint8_t* large_buffer = new uint8_t[num_lines * ring_buf_size * num_frames];

		for (int i = 0; i < num_lines; ++i) {
			line_buffers_ptrs[i] = line_buffers + i*ring_buf_size;
			errChk(imgSetBufferElement2(buflist_ID, i, IMG_BUFF_ADDRESS, line_buffers_ptrs[i])); //register the ptr to the buffer
			errChk(imgSetBufferElement2(buflist_ID, i, IMG_BUFF_SIZE, ring_buf_size));
			unsigned int buf_cmd = (i == num_lines - 1) ? IMG_CMD_STOP : IMG_CMD_NEXT;
			errChk(imgSetBufferElement2(buflist_ID, i, IMG_BUFF_COMMAND, buf_cmd));
		}

		errChk(imgMemLock(buflist_ID));
		errChk(imgSessionConfigure(session_ID, buflist_ID));

		//TODO: triggering

		errChk(imgSessionAcquire(session_ID, TRUE, NULL)); //start acquisition, async, no callback

		bool stop = false;
		std::future<bool> copy_handle = std::async(std::launch::async, copy_ring_buffer, session_ID, large_buffer, ring_buf_size, num_lines, &stop);
	}
	catch (const std::runtime_error& ex) {
		DisplayIMAQError(error);
		return false;
	}
	return true;
}

bool copy_ring_buffer(const SESSION_ID& session_ID, uint8_t* large_buffer, int bytes_per_image, int buffers_per_image, bool* volatile stop) {
	int error = 0;
	uInt32 buf_num = 0;
	uInt32 current_buf = 0;
	void* buf_addr = nullptr;
	try {
		while (!*stop) {
			errChk(imgSessionExamineBuffer2(session_ID, buf_num, &current_buf, &buf_addr));
			memcpy(large_buffer + (current_buf%buffers_per_image)*bytes_per_image, buf_addr, bytes_per_image * sizeof(*large_buffer));
			errChk(imgSessionReleaseBuffer(session_ID));
			buf_num = current_buf + 1;
		}
	}
	catch (const std::runtime_error& ex) {
		DisplayIMAQError(error);
		return false;
	}
	return true;
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

// in case of error this function will display a dialog box
// with the error message
void DisplayIMAQError(Int32 error)
{
	static Int8 ErrorMessage[256];

	memset(ErrorMessage, 0x00, sizeof(ErrorMessage));

	// converts error code to a message
	imgShowError(error, ErrorMessage);

	MessageBoxA(NULL, ErrorMessage, "Imaq Sample", MB_OK | MB_ICONERROR);
}