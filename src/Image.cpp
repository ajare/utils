#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "Image.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

namespace utils
{

	using namespace std;

	namespace
	{
		string getFileExtension(string const& filepath)
		{
			auto const separator = filepath.find_last_of("/\\");
			auto const dot = filepath.find_last_of('.');
			if (dot == string::npos || (separator != string::npos && dot < separator))
			{
				return {};
			}

			string extension = filepath.substr(dot);
			transform(extension.begin(), extension.end(), extension.begin(),
				[](unsigned char character) { return static_cast<char>(tolower(character)); });
			return extension;
		}

		size_t getDataSize(size_t width, size_t height, uint32_t bitsPerPixel)
		{
			if (width == 0 || height == 0 || bitsPerPixel == 0 || bitsPerPixel % 8 != 0)
			{
				throw invalid_argument("Image dimensions and bits per pixel must be non-zero, and bits per pixel must be byte-aligned.");
			}

			auto const bytesPerPixel = static_cast<size_t>(bitsPerPixel / 8);
			if (width > numeric_limits<size_t>::max() / bytesPerPixel)
			{
				throw overflow_error("Image row size is too large.");
			}

			auto const rowSize = width * bytesPerPixel;
			if (height > numeric_limits<size_t>::max() / rowSize)
			{
				throw overflow_error("Image data size is too large.");
			}

			return rowSize * height;
		}
	}

	Image::~Image()
	{
		delete[] mData;
	}

	void Image::loadFromFile(string const& filepath, bool flipVertically)
	{
		stbi_set_flip_vertically_on_load_thread(flipVertically ? 1 : 0);

		int width = 0;
		int height = 0;
		int channels = 0;
		unique_ptr<stbi_uc, void (*)(void*)> loadedData(
			stbi_load(filepath.c_str(), &width, &height, &channels, 0), stbi_image_free);

		if (!loadedData)
		{
			auto const reason = stbi_failure_reason();
			throw runtime_error("Couldn't open '" + filepath + "': " + (reason ? reason : "unknown STB error"));
		}

		auto const bitsPerPixel = static_cast<uint32_t>(channels * 8);
		auto const dataSize = getDataSize(static_cast<size_t>(width), static_cast<size_t>(height), bitsPerPixel);
		auto newData = make_unique<uint8_t[]>(dataSize);
		memcpy(newData.get(), loadedData.get(), dataSize);

		delete[] mData;
		mData = newData.release();
		mWidth = static_cast<size_t>(width);
		mHeight = static_cast<size_t>(height);
		mBitsPerPixel = bitsPerPixel;
		mFilepath = filepath;
	}

	void Image::loadFromData(size_t width, size_t height, uint32_t bitsPerPixel, uint8_t const* data)
	{
		auto const dataSize = getDataSize(width, height, bitsPerPixel);
		if (!data)
		{
			throw invalid_argument("Image data cannot be null.");
		}

		auto newData = make_unique<uint8_t[]>(dataSize);
		memcpy(newData.get(), data, dataSize);

		delete[] mData;
		mData = newData.release();
		mFilepath.clear();
		mWidth = width;
		mHeight = height;
		mBitsPerPixel = bitsPerPixel;
	}

	void Image::saveToFile(string const& filepath)
	{
		if (!mData)
		{
			throw runtime_error("Cannot save an empty image.");
		}

		if (mWidth > static_cast<size_t>(numeric_limits<int>::max()) ||
			mHeight > static_cast<size_t>(numeric_limits<int>::max()))
		{
			throw overflow_error("Image dimensions are too large for STB.");
		}

		auto const channels = static_cast<int>(mBitsPerPixel / 8);
		if (channels < 1 || channels > 4)
		{
			throw runtime_error("STB supports images with one to four 8-bit channels.");
		}

		auto const width = static_cast<int>(mWidth);
		auto const height = static_cast<int>(mHeight);
		auto const extension = getFileExtension(filepath);
		int result = 0;

		if (extension == ".png")
		{
			result = stbi_write_png(filepath.c_str(), width, height, channels, mData, 0);
		}
		else if (extension == ".bmp")
		{
			result = stbi_write_bmp(filepath.c_str(), width, height, channels, mData);
		}
		else if (extension == ".tga")
		{
			result = stbi_write_tga(filepath.c_str(), width, height, channels, mData);
		}
		else if (extension == ".jpg" || extension == ".jpeg")
		{
			result = stbi_write_jpg(filepath.c_str(), width, height, channels, mData, 90);
		}
		else
		{
			throw invalid_argument("Unsupported image output format '" + extension + "'.");
		}

		if (!result)
		{
			throw runtime_error("Couldn't save '" + filepath + "'.");
		}

		mFilepath = filepath;
	}

	size_t Image::getWidth() const
	{
		return mWidth;
	}

	size_t Image::getHeight() const
	{
		return mHeight;
	}

	uint32_t Image::getBitsPerPixel() const
	{
		return mBitsPerPixel;
	}

	uint8_t* const Image::getData() const
	{
		return mData;
	}

} // utils
