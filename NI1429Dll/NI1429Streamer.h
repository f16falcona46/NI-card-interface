#pragma once

#include <cstdint>
#include "niimaq.h"
#include "DataBuffer.h"

class NI1429Streamer
{
public:
	NI1429Streamer(void);
	~NI1429Streamer(void);
	void Setup(const char* interface_name, size_t samples_per_line, size_t lines_per_image, size_t total_lines, DataBuffer::Pointer big_buffer);
	void AcquisitionLoop(void);
	void StartAcquiring(void);
	void StopAcquiring(void);

	bool isSetUp;

private:
	void DisplayIMAQError(Int32 error);
	void NI1429Streamer::DisplayErrorAndThrow(const char* error);

	uint8_t* ring_buffer;
	DataBuffer::Pointer big_buffer;
	volatile bool acquire;
	size_t samples_per_line;
	size_t lines_per_image;
	size_t total_lines;
	unsigned int ring_buf_size;
	SESSION_ID session_ID;
	INTERFACE_ID interface_ID;
	BUFLIST_ID buflist_ID;
};

