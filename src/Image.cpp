#include "FileSystem/FileSystem.hpp"
#include "Image.hpp"
#include "Log.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "tiffio.h"

#include <stdexcept>

using namespace glm;

namespace fre
{
	void Image::create(const ivec2& dimension, const VkFormat format)
	{
		mDimension = dimension;
		mFormat = format;

		calculateStrideAndDataSize();
		
		mData = new uint8_t[mDataSize];
		
		mIsOwner = true;
	}

	void Image::calculateStrideAndDataSize()
	{
		switch(mFormat)
		{
		case VK_FORMAT_R8_UNORM:
			mStride = 1;
			break;
		case VK_FORMAT_R16_UNORM:
			mStride = 2;
			break;
		case VK_FORMAT_R8G8B8A8_UNORM:
			mStride = 4;
			break;
		}
		mDataSize = mDimension.x * mDimension.y * mStride;
	}

	void Image::load()
	{
		if(isFileNameValid())
		{
			FS;
			//Load pixel data of image
			if(mFileName.find(".tiff") != std::string::npos || mFileName.find(".tif") != std::string::npos)
			{
				loadTIFF(fs.find(mFileName));
			}
			else if(mFileName.find(".png") != std::string::npos || mFileName.find(".jpg") != std::string::npos)
			{
				loadPng(fs.find(mFileName));
			}

			if (mData == nullptr)
			{
				throw std::runtime_error("Failed to load a texture file! (" + mFileName + ")");
			}

			mIsOwner = true;
		}
		
		calculateStrideAndDataSize();
 	}

	void Image::loadPng(const std::string& fileName)
	{
		//Number of channels image uses
		int channels;

		auto format = STBI_rgb_alpha;
		switch(mFormat)
		{
		case VK_FORMAT_R8_UNORM:
			format = STBI_grey;
			break;
		}
		mData = stbi_load(fileName.c_str(), &mDimension.x, &mDimension.y, &channels, format);
		mIsPNG = true;
	}

	void Image::loadTIFF(const std::string& fileName)
	{
		TIFF* tiff = TIFFOpen(fileName.c_str(), "r");
		if (!tiff)
		{
			TIFFClose(tiff);
			throw std::runtime_error("Failed to load a texture file! (" + mFileName + ")");
		}

		uint32 width, height;
		uint16 bitsPerSample, samplePerPixel, photometric;
		TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width);
		TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height);
		TIFFGetField(tiff, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
		TIFFGetField(tiff, TIFFTAG_SAMPLESPERPIXEL, &samplePerPixel);
		TIFFGetField(tiff, TIFFTAG_PHOTOMETRIC, &photometric);

		if (bitsPerSample != 16 || samplePerPixel != 1 || photometric != PHOTOMETRIC_MINISBLACK) {
			TIFFClose(tiff);
			throw std::runtime_error("Unsupported TIFF format: 16-bit grayscale image expected");
		}

		uint64 imageSize = width * height;
		//mData = malloc(imageSize * sizeof(uint16));
		uint32_t imageLength;
		TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &imageLength);
		uint32_t scanlineSize = TIFFScanlineSize(tiff);
		mData = _TIFFmalloc(scanlineSize * imageLength);
		mIsTIFF = true;
		
		for(int row = 0; row < imageLength; row++)
		{
			if (!TIFFReadScanline(tiff, &((uint8_t*)mData)[row * scanlineSize], row, 0))
			{
				TIFFClose(tiff);
				free(mData);
				throw std::runtime_error("Failed to read image data");
			}
		}

		mDimension.x = width;
		mDimension.y = height;
		
		TIFFClose(tiff);
	}

	void Image::saveTIFF(const std::string& fileName, uint16_t* data, int width, int height)
	{
		TIFF* tiff = TIFFOpen(fileName.c_str(), "w");
		if(!tiff)
		{
			TIFFClose(tiff);
			throw std::runtime_error("Failed to open TIFF file for writing");
		}

		// Set TIFF tags
		TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, width);
		TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, height);
		TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 1);
		TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 16);
		TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
		TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
		TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
		TIFFSetField(tiff, TIFFTAG_COMPRESSION, COMPRESSION_NONE);

		// Write image data
		for(int row = 0; row < height; ++row)
		{
			if(TIFFWriteScanline(tiff, &data[row * width], row, 0) < 0)
			{
				TIFFClose(tiff);
				throw std::runtime_error("Failed to write TIFF scanline");
			}
		}

		TIFFClose(tiff);
	}

    void Image::destroy()
    {
		if(mIsOwner)
		{
			if(mIsTIFF)
			{
				_TIFFfree(mData);
			}
			else if(mIsPNG)
			{
				stbi_image_free(mData);
			}
			else
			{
				delete [] mData;
			}
		}
    }

	bool Image::isFileNameValid() const
	{
		return !mFileName.empty() && mFileName.find('#') == std::string::npos;
	}
	
    ivec2 Image::getDimensions(const std::string& fileName)
    {
        ivec2 result(0);
        int numChannels = 0;

		if(fileName.find(".tiff") != std::string::npos || fileName.find(".tif") != std::string::npos)
		{
			TIFF* tiff = TIFFOpen(fileName.c_str(), "r");
			if(tiff)
			{
				// Get the width and height
				if (TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &result.x) != 1 ||
					TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &result.y) != 1)
				{
					LOG_ERROR("Could not retrieve the dimensions of the TIFF file: {}", fileName);
				}

				// Close the TIFF file
				TIFFClose(tiff);
			}
			else
			{
				LOG_ERROR("Can't open TIFF file to retrieve the dimensions: {}", fileName);
			}
		}
		else if(fileName.find(".png") != std::string::npos)
		{
			stbi_info(fileName.c_str(), &result.x, &result.y, &numChannels);
		}

		return result;
    }
}