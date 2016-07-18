#pragma once

#include <memory>

class DataBuffer
{
public:
	typedef std::shared_ptr<DataBuffer> Pointer;
	DataBuffer(size_t size);
	~DataBuffer();
	void CopyIn(const short* in, size_t len);
	void ResetCounter();
	void BindForAcquisition();
	void UnbindForAcquisition();
private:
	std::unique_ptr<short[]> buf;
	size_t counter;
};

