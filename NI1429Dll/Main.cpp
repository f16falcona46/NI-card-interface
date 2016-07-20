#include "stdafx.h"
#include <memory>
#include <future>
#include <iostream>
#include <string>
#include "debug_new.h"
#include "NI1429Streamer.h"
#include "DataBuffer.h"


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
	AllocConsole();
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
	try {
		const int total_lines = 4096;
		const int width = 512;
		std::shared_ptr<DataBuffer> buffer_ptr(new DataBuffer(total_lines, width, "test.png"));
		NI1429Streamer streamer;
		buffer_ptr->BindForAcquisition();
		buffer_ptr->ResetCounter();
		streamer.Setup("img0", width, total_lines, buffer_ptr);
		if (streamer.isSetUp) {
			std::future<void> handle = std::async(std::launch::async, [&streamer]() { streamer.AcquisitionLoop(); });
			streamer.StartAcquiring();
			//Sleep(1000);
			handle.get();
			for (int i = 0; i < width*total_lines; ++i) {
				if ((i % 10000) == 0 ){
					std::cout << "Sample " << i << ": " << buffer_ptr->buf[i] << std::endl;
				}
			}
		}
	}
	catch (const std::exception& e) {
		MessageBoxA(NULL, e.what(), "Exception", MB_OK);
	}
	std::string s;
	std::getline(std::cin, s);
	return 0;
}
