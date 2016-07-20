#pragma once

#include <memory>
#include <string>
#include <cstdio>

class DataBuffer
{
public:
	typedef std::shared_ptr<DataBuffer> Pointer;
	DataBuffer(size_t lines, size_t width, const std::string& file);
	~DataBuffer();
	bool CopyIn(const short* in, size_t len);
	void ResetCounter();
	void BindForAcquisition();
	void UnbindForAcquisition();
	std::unique_ptr<short[]> buf;
	size_t maxlen;
	size_t counter;
	std::string filename;
	size_t width;
	size_t lines;
};

