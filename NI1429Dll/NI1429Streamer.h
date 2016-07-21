#pragma once

#include <cstdint>
#include <string>
#include "niimaq.h"
#include "DataBuffer.h"
#include "IDataAcquisition.h"

class NI1429Streamer
{
public:
	NI1429Streamer(void);
	~NI1429Streamer(void);
	void Setup(const std::string& interface_name, size_t samples_per_line, size_t total_lines, DataBuffer::Pointer big_buffer, IUserInterface* UI_pointer);
	void AcquisitionLoop(void);
	void StartAcquiring(void);
	void StopAcquiring(void);
	void ReleaseNIDAQResources(void);

	bool isSetUp;

private:
	static void DisplayIMAQError(Int32 error, int line);
	static void DisplayErrorAndThrow(const char* error);
	void CreateBuflist(void);

	int16_t* ring_buffer;
	int16_t* mini_buf;
	DataBuffer::Pointer big_buffer;
	volatile bool acquire;
	size_t samples_per_line;
	size_t total_lines;
	size_t frame_size;
	SESSION_ID session_ID;
	INTERFACE_ID interface_ID;
	BUFLIST_ID buflist_ID;
	int bytes_per_sample;
	IUserInterface* UI_pointer;
	std::string interface_name;
	bool buffer_needs_reallocation;

	static const size_t max_height = 512;
	static const size_t num_frames = 4;
};

