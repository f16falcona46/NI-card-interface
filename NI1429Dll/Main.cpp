#include "stdafx.h"
#include "NI1429Streamer.h"
#include "DataBuffer.h"
#include <memory>
#include <future>
#include <iostream>
#include <string>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
	try {
		std::shared_ptr<DataBuffer> buffer_ptr(new DataBuffer(512));
		NI1429Streamer streamer;
		buffer_ptr->BindForAcquisition();
		buffer_ptr->ResetCounter();
		streamer.Setup("img0", 512, 512, 512, buffer_ptr);
		if (streamer.isSetUp) {
			std::future<void> handle = std::async(std::launch::async, [&streamer]() { streamer.AcquisitionLoop(); });
			streamer.StartAcquiring();
			for (volatile int i = 0; i < 1000; ++i);
			streamer.StopAcquiring();
		}
	}
	catch (...) {
		MessageBoxA(NULL, "Caught!", "Exception", MB_OK);
	}
	return 0;
}
