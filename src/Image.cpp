#include "Image.h"

#include <freeimage/freeimage.h>

namespace utils
{

	using namespace std;
	
	void FreeImageErrorHandler(FREE_IMAGE_FORMAT fif, const char *message)
	{
		string errMsg;
		if (fif != FIF_UNKNOWN)
		{
			errMsg += string(FreeImage_GetFormatFromFIF(fif)) + "file: ";
		}

		errMsg += message;
	}

	Image::~Image()
	{
		delete[] mData;
	}

	void Image::loadFromFile(string const& filepath, bool flipVertically)
	{
		// Clear old data if any
		delete[] mData;

		// Load new data
		auto bitmap = FreeImage_Load(FreeImage_GetFIFFromFilename(filepath.c_str()), filepath.c_str());

		if (bitmap)
		{
			mWidth = FreeImage_GetWidth(bitmap);
			mHeight = FreeImage_GetHeight(bitmap);
			mBitsPerPixel = FreeImage_GetBPP(bitmap);

			auto dataSpan = mWidth * mBitsPerPixel / 8;
			auto dataSize = dataSpan * mHeight;

			mData = new uint8_t[dataSize];

			// Flip vertically?
			int y0, y1, inc;
			if (flipVertically)
			{
				y0 = mHeight - 1;
				y1 = -1;
				inc = -1;
			}
			else
			{
				y0 = 0;
				y1 = mHeight;
				inc = 1;
			}

			auto ptr = (unsigned char*)FreeImage_GetBits(bitmap);
			for (int y = y0; y != y1; y += inc)
			{
				memcpy(&mData[y * dataSpan], ptr, dataSpan);
				ptr += dataSpan;
			}

			FreeImage_Unload(bitmap);
		}
		else
		{
			string errMsg = "Couldn't open '" + filepath + "'.";
			throw exception(errMsg.c_str());
		}
	}

	void Image::loadFromData(size_t width, size_t height, uint32_t bitsPerPixel, uint8_t const* data)
	{
		// Clear old data if any
		delete[] mData;
		mFilepath = "";

		// Load new data
		mWidth = width;
		mHeight = height;
		mBitsPerPixel = bitsPerPixel;

		auto dataSpan = mWidth * mBitsPerPixel / 8;
		auto dataSize = dataSpan * mHeight;

		mData = new uint8_t[dataSize];
		memcpy(mData, data, dataSize);
	}

	void Image::saveToFile(string const& filepath)
	{
		auto channels = mBitsPerPixel / 8;
		auto bitmap = FreeImage_ConvertFromRawBits(
			mData,
			mWidth,
			mHeight,
			channels * mWidth,
			mBitsPerPixel,
			0xFF0000,
			0x00FF00,
			0x0000FF,
			false);

		FreeImage_Save(FreeImage_GetFIFFromFilename(filepath.c_str()), bitmap, filepath.c_str());
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
