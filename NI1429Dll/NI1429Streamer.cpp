#include "StdAfx.h"
#include "NI1429Streamer.h"
#include "StackWalker.h"
#include "DataBuffer.h"

#include <stdexcept>
#include <memory>
#include "niimaq.h"

#define errChk(fCall) if (error = (fCall), error < 0) DisplayIMAQError(error); else (void)0

NI1429Streamer::NI1429Streamer(void) : big_buffer() {
	ring_buffer = nullptr;
	acquire = false;
	samples_per_line = 0;
	lines_per_image = 0;
	total_lines = 0;
	ring_buf_size = 0;
	session_ID = 0;
	interface_ID = 0;
	buflist_ID = 0;
	isSetUp = false;
}

NI1429Streamer::~NI1429Streamer(void)
{
	if (session_ID) imgSessionAbort(session_ID, NULL);
	if (buflist_ID) imgMemUnlock(buflist_ID);
	if (buflist_ID) imgDisposeBufList(buflist_ID, FALSE);
	if (session_ID) imgClose(session_ID, TRUE);
	if (interface_ID) imgClose(interface_ID, TRUE);
	if (ring_buffer) delete[] ring_buffer;
}

void NI1429Streamer::Setup(const char* interface_name, size_t samples_per_line, size_t lines_per_image, size_t total_lines, DataBuffer::Pointer big_buffer)
{
	const int image_width = 512; //image size
	const int image_height = 1;

	int error = 0;
	unsigned int bytes_per_pixel;

	this->lines_per_image = lines_per_image;
	this->samples_per_line = samples_per_line;
	this->total_lines = total_lines;
	this->big_buffer = big_buffer;

	interface_ID = 0;
	session_ID = 0;
	errChk(imgInterfaceOpen(interface_name, &interface_ID)); //open interface and session
	errChk(imgSessionOpen(interface_ID, &session_ID));

	errChk(imgSetAttribute2(session_ID, IMG_ATTR_ROI_WIDTH, image_width)); //set image width and height to get
	errChk(imgSetAttribute2(session_ID, IMG_ATTR_ROI_HEIGHT, image_height));
	errChk(imgSetAttribute2(session_ID, IMG_ATTR_ROWPIXELS, image_width));

	buflist_ID = 0;
	errChk(imgCreateBufList(lines_per_image, &buflist_ID)); //make a buffer list
	errChk(imgGetAttribute(session_ID, IMG_ATTR_BYTESPERPIXEL, &bytes_per_pixel)); //make the buffer and pointers to it
	ring_buf_size = image_width * image_height * bytes_per_pixel;

	ring_buffer = new uint8_t[ring_buf_size * lines_per_image];

	for (int i = 0; i < lines_per_image; ++i) {
		errChk(imgSetBufferElement2(buflist_ID, i, IMG_BUFF_ADDRESS, ring_buffer + i*ring_buf_size)); //register the ptr to the buffer
		errChk(imgSetBufferElement2(buflist_ID, i, IMG_BUFF_SIZE, ring_buf_size));
		unsigned int buf_cmd = (i == lines_per_image - 1) ? IMG_CMD_STOP : IMG_CMD_NEXT;
		errChk(imgSetBufferElement2(buflist_ID, i, IMG_BUFF_COMMAND, buf_cmd));
	}

	errChk(imgMemLock(buflist_ID));
	errChk(imgSessionConfigure(session_ID, buflist_ID));

	errChk(imgSessionAcquire(session_ID, TRUE, NULL)); //start acquisition, async, no callback
	acquire = false;
	isSetUp = true;
}

void NI1429Streamer::AcquisitionLoop(void) {
	if (!isSetUp) DisplayErrorAndThrow("Did not set up NI1429Streamer!");
	int error = 0;
	uInt32 buf_num = 0;
	uInt32 current_buf = 0;
	void* buf_addr = nullptr;

	std::unique_ptr<uint8_t[]> framebuffer(new uint8_t[ring_buf_size]);
	while (!acquire);
	while (acquire) {
		errChk(imgSessionExamineBuffer2(session_ID, buf_num, &current_buf, &buf_addr));
		memcpy(framebuffer.get() + (current_buf%lines_per_image)*samples_per_line, buf_addr, samples_per_line * sizeof(*framebuffer.get()));
		errChk(imgSessionReleaseBuffer(session_ID));
		buf_num = current_buf + 1;
		if ((buf_num % lines_per_image) == 0) {
			big_buffer->CopyIn(reinterpret_cast<short*>(framebuffer.get()), ring_buf_size / (sizeof(short) / sizeof(*framebuffer.get())));
		}
		if (buf_num == total_lines) break;
	}
	big_buffer->UnbindForAcquisition();
	if (big_buffer) {
		big_buffer->UnbindForAcquisition();
	}
	isSetUp = false;
	acquire = false;
}

void NI1429Streamer::StartAcquiring(void) {
	if (!isSetUp) DisplayErrorAndThrow("Did not set up NI1429Streamer!");
	acquire = true;
}

void NI1429Streamer::StopAcquiring(void) {
	if (!isSetUp) DisplayErrorAndThrow("Did not set up NI1429Streamer!");
	acquire = false;
}

void NI1429Streamer::DisplayIMAQError(Int32 error)
{
	static Int8 ErrorMessage[256];

	memset(ErrorMessage, 0x00, sizeof(ErrorMessage));

	// converts error code to a message
	imgShowError(error, ErrorMessage);

	StackWalker sw;
	sw.ShowCallstack();
	MessageBoxA(NULL, ErrorMessage, "IMAQ Error", MB_OK | MB_ICONERROR);
	throw std::runtime_error(ErrorMessage);
}

void NI1429Streamer::DisplayErrorAndThrow(const char* error)
{
	StackWalker sw;
	sw.ShowCallstack();
	MessageBoxA(NULL, error, "NI1429Streamer Error", MB_OK | MB_ICONERROR);
	throw std::logic_error(error);
}
