#include "stdafx.h"
#include <windows.h>
#include <memory>
#include <future>
#include <iostream>
#include <string>
#include <locale>
#include <codecvt>
#include <sstream>
#include "debug_new.h"
#include "NI1429Streamer.h"
#include "DataBuffer.h"
#include "resource.h"

INT_PTR CALLBACK ConfigureDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG:
	{
		HICON hIcon = static_cast<HICON>(LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MYICON), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE));
		SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	}
	return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			break;
		case IDOK:
			try {
				std::wstring interface_name;
				int textlen = GetWindowTextLength(GetDlgItem(hwnd, IFC_INTERFACENAME));
				interface_name.resize(textlen);
				GetWindowText(GetDlgItem(hwnd, IFC_INTERFACENAME), const_cast<wchar_t*>(interface_name.data()), textlen + 1);
				std::string ifname = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(interface_name);

				std::wstring w_buf;
				textlen = GetWindowTextLength(GetDlgItem(hwnd, IDC_WIDTH));
				w_buf.resize(textlen);
				GetWindowText(GetDlgItem(hwnd, IDC_WIDTH), const_cast<wchar_t*>(w_buf.data()), textlen + 1);
				std::string buf = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(w_buf);
				size_t width = std::stoi(buf);

				textlen = GetWindowTextLength(GetDlgItem(hwnd, IDC_HEIGHT));
				w_buf.resize(textlen);
				GetWindowText(GetDlgItem(hwnd, IDC_HEIGHT), const_cast<wchar_t*>(w_buf.data()), textlen + 1);
				buf = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(w_buf);
				size_t height = std::stoi(buf);

				std::shared_ptr<DataBuffer> buffer_ptr(new DataBuffer(height, width, "out.png"));
				NI1429Streamer streamer;
				buffer_ptr->BindForAcquisition();
				buffer_ptr->ResetCounter();

				streamer.Setup(ifname.c_str(), width, height, buffer_ptr);
				if (streamer.isSetUp) {
					std::future<void> handle = std::async(std::launch::async, [&streamer]() { streamer.AcquisitionLoop(); });
					streamer.StartAcquiring();
					handle.get();
				}
			}
			catch (const std::exception& e) {
				MessageBoxA(hwnd, e.what(), "Exception", MB_OK);
				EndDialog(hwnd, 3);
			}
			EndDialog(hwnd, IDOK);
			break;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
	INT_PTR ret = DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_CONFIGURE), NULL, ConfigureDlgProc);
	if (ret == IDOK) {
		MessageBoxA(NULL, "Operation completed successfully.", "Notice", MB_OK | MB_ICONINFORMATION);
		return 0;
	}
	else if (ret == IDCANCEL) {
		MessageBoxA(NULL, "Operation canceled.", "Notice", MB_OK | MB_ICONINFORMATION);
		return -3;
	}
	else {
		MessageBoxA(NULL, "Operation failed to complete.", "Notice", MB_OK | MB_ICONERROR);
		return -1;
	}
	return 0;
}
