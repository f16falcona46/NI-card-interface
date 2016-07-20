#include "StdAfx.h"
#include "NI1429Streamer.h"
#include "StackWalker.h"
#include "DataBuffer.h"

#include <stdexcept>
#include <memory>
#include <iostream>
#include "niimaq.h"

#undef DEBUG

#ifdef DEBUG
#define WE_ARE_HERE() do { char buf[50]; sprintf(buf, "We are at line %d.", __LINE__); MessageBoxA(NULL, buf, "Notice", MB_OK); } while (false)
#else
#define WE_ARE_HERE() 
#endif


#define errChk(fCall) if (error = (fCall), error < 0) DisplayIMAQError(error, __LINE__); else (void)0

NI1429Streamer::NI1429Streamer(void) : big_buffer(), interface_name() {
	ring_buffer = nullptr;
	acquire = false;
	samples_per_line = 0;
	total_lines = 0;
	ring_buf_size = 0;
	session_ID = 0;
	interface_ID = 0;
	buflist_ID = 0;
	isSetUp = false;
}

NI1429Streamer::~NI1429Streamer(void)
{
	ReleaseResources();
}

void NI1429Streamer::Setup(const std::string& interface_name, size_t samples_per_line, size_t total_lines, DataBuffer::Pointer big_buffer)
{
	WE_ARE_HERE();
	if (!isSetUp) {
		int error = 0;

		this->samples_per_line = samples_per_line;
		this->total_lines = total_lines;
		this->big_buffer = big_buffer;
		this->interface_name = interface_name;

		interface_ID = 0;
		session_ID = 0;
		errChk(imgInterfaceOpen(interface_name.c_str(), &interface_ID)); //open interface and session
		errChk(imgSessionOpen(interface_ID, &session_ID));

		errChk(imgSetAttribute2(session_ID, IMG_ATTR_ROI_WIDTH, samples_per_line)); //set image width and height to get
		errChk(imgSetAttribute2(session_ID, IMG_ATTR_ACQWINDOW_HEIGHT, total_lines));
		errChk(imgSetAttribute2(session_ID, IMG_ATTR_ROI_HEIGHT, total_lines));
		errChk(imgSetAttribute2(session_ID, IMG_ATTR_ROWPIXELS, samples_per_line));
		errChk(imgGetAttribute(session_ID, IMG_ATTR_BYTESPERPIXEL, &bytes_per_sample)); //make the buffer and pointers to it
	}
	acquire = false;
	isSetUp = true;
	WE_ARE_HERE();
}

void NI1429Streamer::AcquisitionLoop(void) {
	if (!isSetUp) DisplayErrorAndThrow("Did not set up NI1429Streamer!");
	WE_ARE_HERE();
	int error = 0;
	void* buf_addr = nullptr;
	int16_t* framebuffer = nullptr;
	while (!acquire);
	WE_ARE_HERE();
	errChk(imgGrabSetup(session_ID, TRUE));
	errChk(imgGrab(session_ID, reinterpret_cast<void**>(&framebuffer), TRUE));
	big_buffer->CopyIn(reinterpret_cast<short*>(framebuffer), samples_per_line * total_lines);
	
	big_buffer->UnbindForAcquisition();
	isSetUp = true;
	acquire = false;
	WE_ARE_HERE();
}

void NI1429Streamer::StartAcquiring(void) {
	if (!isSetUp) DisplayErrorAndThrow("Did not set up NI1429Streamer!");
	acquire = true;
}

void NI1429Streamer::StopAcquiring(void) {
	if (!isSetUp) DisplayErrorAndThrow("Did not set up NI1429Streamer!");
	acquire = false;
	isSetUp = false;
}

void NI1429Streamer::DisplayIMAQError(Int32 error, int line)
{
	Int8 ErrorMessage[256];

	memset(ErrorMessage, 0x00, sizeof(ErrorMessage));

	// converts error code to a message
	imgShowError(error, ErrorMessage);

	char buf[300];
	sprintf(buf, "%s at line %d", ErrorMessage, line);
	StackWalker sw;
	sw.ShowCallstack();
	MessageBoxA(NULL, buf, "IMAQ Error", MB_OK | MB_ICONERROR);
	throw std::runtime_error(ErrorMessage);
}

void NI1429Streamer::DisplayErrorAndThrow(const char* error)
{
	StackWalker sw;
	sw.ShowCallstack();
	MessageBoxA(NULL, error, "NI1429Streamer Error", MB_OK | MB_ICONERROR);
	throw std::logic_error(error);
}

void NI1429Streamer::ReleaseResources(void) {
	if (session_ID) imgSessionAbort(session_ID, NULL);
	if (buflist_ID) imgMemUnlock(buflist_ID);
	if (buflist_ID) imgDisposeBufList(buflist_ID, FALSE);
	buflist_ID = 0;
	if (session_ID) imgClose(session_ID, TRUE);
	session_ID = 0;
	if (interface_ID) imgClose(interface_ID, TRUE);
	interface_ID = 0;
	if (ring_buffer) delete[] ring_buffer;
	ring_buffer = nullptr;
	isSetUp = false;
	acquire = false;
}