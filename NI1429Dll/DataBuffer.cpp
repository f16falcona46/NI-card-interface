#include "stdafx.h"
#include "DataBuffer.h"
#include <iostream>


DataBuffer::DataBuffer(size_t size) : buf(new short[size])
{
	counter = 0;
}


DataBuffer::~DataBuffer()
{
}

void DataBuffer::CopyIn(const short* in, size_t len)
{
	for (int i = 0; i < len; ++i) {
		buf[i + counter] = in[i];
		std::cout << "Sample " << i << ": " << in[i] << std::endl;
	}
}

void DataBuffer::ResetCounter() {
	counter = 0;
}

void DataBuffer::BindForAcquisition()
{
	std::cout << "Bound for acquisition." << std::endl;
}

void DataBuffer::UnbindForAcquisition()
{
	std::cout << "Unbound for acquisition." << std::endl;
}