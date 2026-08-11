#pragma once

#include <cstdint>
#include <string>

#include "Platform.h"

namespace utils
{

	class UTILS_API Image
	{
		std::string mFilepath;

		size_t mWidth{ 0 }, mHeight{ 0 };
		
		uint32_t mBitsPerPixel{ 0 };

		uint8_t* mData{ nullptr };

	public:

		Image() = default;

		~Image();

		void loadFromFile(std::string const& filepath, bool flipVertically = false);

		void loadFromData(size_t width, size_t height, uint32_t bitsPerPixel, uint8_t const* data);

		void saveToFile(std::string const& filepath);

		size_t getWidth() const;

		size_t getHeight() const;

		uint32_t getBitsPerPixel() const;

		uint8_t* const getData() const;
	};

} // utils
