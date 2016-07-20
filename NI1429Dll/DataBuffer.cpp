#include "stdafx.h"
#include "DataBuffer.h"
#include <iostream>
#include <string>
#include <cstdio>
#include <stdexcept>
#include <vector>
#include <csetjmp>
#include <functional>
#include <algorithm>
#include <png.h>


DataBuffer::DataBuffer(size_t lines, size_t width, const std::string& filename) : buf(new short[width * lines]()), filename(filename)
{
	this->width = width;
	this->lines = lines;
	this->maxlen = lines * width;
	this->counter = 0;
}


DataBuffer::~DataBuffer()
{
}

bool DataBuffer::CopyIn(const short* in, size_t len)
{
	memcpy(buf.get() + counter, in, len * 2);
	counter += len;
	return true;
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
	std::unique_ptr<std::FILE, int(*)(std::FILE*)> file(std::fopen(filename.c_str(), "wb"), std::fclose);
	std::unique_ptr<png_struct, std::function<void(png_struct*)>> png_ptr(png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr), [](png_structp p) {png_destroy_write_struct(&p, nullptr); });
	std::unique_ptr<png_info, std::function<void(png_info*)>> info_ptr(png_create_info_struct(png_ptr.get()), [&png_ptr](png_infop p) {png_free_data(png_ptr.get(), p, PNG_FREE_ALL, -1); });
	std::vector<png_byte> row(width * 3);

	if (file == nullptr) {
		throw std::runtime_error(std::string("Could not open PNG file ") + filename);
	}
	if (png_ptr == nullptr) {
		throw std::runtime_error("Could not allocate PNG write struct");
	}
	if (info_ptr == nullptr) {
		throw std::runtime_error("Could not allocate PNG info struct");
	}
	if (setjmp(png_jmpbuf(png_ptr.get()))) {
		throw std::runtime_error("PNG writing failed");
	}
	png_init_io(png_ptr.get(), file.get());
	png_set_IHDR(png_ptr.get(), info_ptr.get(), width, maxlen/width, 8, PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	png_write_info(png_ptr.get(), info_ptr.get());

	for (int y = 0; y < lines; ++y) {
		std::fill(row.begin(), row.end(), 0);
		for (int x = 0; x < width; ++x) {
			for (size_t i = 0; i < 3; ++i) {
				row[x * 3 + i] = (png_byte)(((double)(buf[y * width + x])) / 16);
			}
		}
		png_write_row(png_ptr.get(), row.data());
	}
	png_write_end(png_ptr.get(), nullptr);

	std::cout << "Unbound for acquisition." << std::endl;
}