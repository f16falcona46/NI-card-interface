#include "StdAfx.h"
#include "NI1429Streamer.h"
#include "StackWalker.h"
#include "DataBuffer.h"
#include "boost/thread.hpp"

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

#undef DEBUG_DPRINTF

#ifdef DEBUG_DPRINTF
#include "dprintf.h"
#define WE_ARE_HERE_DPRINTF() do { dprintf("We are in %s line %d.\r\n", __FILE__, __LINE__); } while (false)
#else
#define WE_ARE_HERE_DPRINTF() 
#endif

#define TRIGGERING


#define errChk(fCall) if (error = (fCall), error < 0) DisplayIMAQError(error, __LINE__); else (void)0

NI1429Streamer::NI1429Streamer(void) : big_buffer(), interface_name() {
	WE_ARE_HERE_DPRINTF();
	ring_buffer = nullptr;
	mini_buf = nullptr;
	acquire = false;
	samples_per_line = 0;
	total_lines = 0;
	frame_size = 0;
	session_ID = 0;
	interface_ID = 0;
	buflist_ID = 0;
	isSetUp = false;
	buffer_needs_reallocation = true;
}

NI1429Streamer::~NI1429Streamer(void)
{
	WE_ARE_HERE_DPRINTF();
	ReleaseNIDAQResources();
	if (ring_buffer) delete[] ring_buffer;
	if (mini_buf) delete[] mini_buf;
}

void NI1429Streamer::CreateBuflist(void)
{

}

void NI1429Streamer::Setup(const std::string& interface_name, size_t samples_per_line, size_t total_lines, DataBuffer::Pointer big_buffer, IUserInterface* UI_pointer)
{
	WE_ARE_HERE_DPRINTF();
	WE_ARE_HERE();

	int error = 0;

	this->samples_per_line = samples_per_line;
	this->total_lines = total_lines;
	this->big_buffer = big_buffer;
	this->interface_name = interface_name;
	this->UI_pointer = UI_pointer;
	if (this->frame_size != samples_per_line * max_height) {
		this->frame_size = samples_per_line * max_height;
		buffer_needs_reallocation = true;
	}

	interface_ID = 0;
	session_ID = 0;
	errChk(imgInterfaceOpen(interface_name.c_str(), &interface_ID)); //open interface and session
	errChk(imgSessionOpen(interface_ID, &session_ID));

	errChk(imgSetAttribute2(session_ID, IMG_ATTR_ROI_WIDTH, samples_per_line));
	errChk(imgSetAttribute2(session_ID, IMG_ATTR_ROWPIXELS, samples_per_line));
	errChk(imgGetAttribute(session_ID, IMG_ATTR_BYTESPERPIXEL, &bytes_per_sample));
	errChk(imgSetAttribute2(session_ID, IMG_ATTR_ACQWINDOW_HEIGHT, max_height));
	errChk(imgSetAttribute2(session_ID, IMG_ATTR_ROI_HEIGHT, max_height));

#ifdef TRIGGERING
	errChk(imgSessionTriggerConfigure2(session_ID, IMG_SIGNAL_EXTERNAL, 0, IMG_TRIG_POLAR_ACTIVEH, 5000, IMG_TRIG_ACTION_BUFFER));
#endif

	if (buffer_needs_reallocation) {
		if (ring_buffer) delete[] ring_buffer;
		if (mini_buf) delete[] mini_buf;
		mini_buf = new int16_t[frame_size];
		ring_buffer = new int16_t[frame_size * num_frames];
	}

	errChk(imgCreateBufList(num_frames, &buflist_ID));
	for (size_t i = 0; i < num_frames; ++i) {
		errChk(imgSetBufferElement2(buflist_ID, i, IMG_BUFF_ADDRESS, ring_buffer + i * frame_size));
		errChk(imgSetBufferElement2(buflist_ID, i, IMG_BUFF_SIZE, frame_size * sizeof(*ring_buffer)));
		unsigned int buf_cmd = (i == num_frames - 1) ? IMG_CMD_LOOP : IMG_CMD_NEXT;
		errChk(imgSetBufferElement2(buflist_ID, i, IMG_BUFF_COMMAND, buf_cmd));
	}
	errChk(imgMemLock(buflist_ID));

	acquire = false;
	isSetUp = true;
	WE_ARE_HERE();
}

void NI1429Streamer::AcquisitionLoop(void) {
	WE_ARE_HERE_DPRINTF();
	if (!isSetUp) DisplayErrorAndThrow("Did not set up NI1429Streamer!");
	WE_ARE_HERE();
	int error = 0;
	while (!acquire);
	WE_ARE_HERE();


	//errChk(imgGrabSetup(session_ID, TRUE));
	errChk(imgSessionConfigure(session_ID, buflist_ID));
	errChk(imgSessionAcquire(session_ID, TRUE, NULL));


	size_t lines_left_to_acquire = total_lines;
	size_t iteration_number = 0;
	uInt32 current_buf_num = 0;
	void* current_buf_ptr = nullptr;
	try {
		while (lines_left_to_acquire) {
			//errChk(imgGrab(session_ID, reinterpret_cast<void**>(&buf_ptr), TRUE)); //do something with this!!!
			errChk(imgSessionExamineBuffer2(session_ID, iteration_number, &current_buf_num, &current_buf_ptr));
			memcpy(mini_buf, current_buf_ptr, frame_size * sizeof(*mini_buf));
			++iteration_number;
			errChk(imgSessionReleaseBuffer(session_ID));
			size_t lines_acquired_this_iteration = min(max_height, lines_left_to_acquire);
			if (!big_buffer->CopyIn(mini_buf, samples_per_line * lines_acquired_this_iteration)) DisplayErrorAndThrow("Failed to copy into Databuffer!");
			lines_left_to_acquire -= lines_acquired_this_iteration;
		}
	}
	catch (const boost::thread_interrupted&) {}

	big_buffer->UnbindForAcquisition();
	UI_pointer->AfterStreamAutoStop();
	isSetUp = true;
	acquire = false;
	WE_ARE_HERE();
	WE_ARE_HERE_DPRINTF();
	int i;
	errChk(imgGetAttribute(session_ID, IMG_ATTR_LOST_FRAMES, &i));
	std::string s = std::to_string(i);
	MessageBoxA(NULL, s.c_str(), "num of frames missed :(", MB_OK);
}

void NI1429Streamer::StartAcquiring(void) {
	WE_ARE_HERE_DPRINTF();
	if (!isSetUp) DisplayErrorAndThrow("Did not set up NI1429Streamer!");
	acquire = true;
}

void NI1429Streamer::StopAcquiring(void) {
	WE_ARE_HERE_DPRINTF();
	if (!isSetUp) DisplayErrorAndThrow("Did not set up NI1429Streamer!");
	acquire = false;
	isSetUp = false;
}

void NI1429Streamer::DisplayIMAQError(Int32 error, int line)
{
	WE_ARE_HERE_DPRINTF();
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
	WE_ARE_HERE_DPRINTF();
	StackWalker sw;
	sw.ShowCallstack();
	MessageBoxA(NULL, error, "NI1429Streamer Error", MB_OK | MB_ICONERROR);
	throw std::logic_error(error);
}

void NI1429Streamer::ReleaseNIDAQResources(void) {
	WE_ARE_HERE_DPRINTF();
	if (session_ID) imgSessionAbort(session_ID, NULL);
	if (buflist_ID) imgMemUnlock(buflist_ID);
	if (buflist_ID) imgDisposeBufList(buflist_ID, FALSE);
	buflist_ID = 0;
	if (session_ID) imgClose(session_ID, TRUE);
	session_ID = 0;
	if (interface_ID) imgClose(interface_ID, TRUE);
	interface_ID = 0;
	isSetUp = false;
	acquire = false;
}