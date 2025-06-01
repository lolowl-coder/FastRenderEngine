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
				getInfoFromTiff(mFileName, mDimension, mFormat, mNumChannels);
				loadTIFF(fs.find(mFileName));
			}
			else if(mFileName.find(".png") != std::string::npos || mFileName.find(".jpg") != std::string::npos)
			{
				getInfoFromTiff(mFileName, mDimension, mFormat, mNumChannels);
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
		int width;
		int height;
		mData = stbi_load(fileName.c_str(), &width, &height, &channels, mNumChannels);
		mIsPNG = true;
	}

	void Image::loadTIFF(const std::string& fileName)
	{
		TIFF* tiff = TIFFOpen(fileName.c_str(), "r");
		if (!tiff)
		{
			TIFFClose(tiff);
			throw std::runtime_error("Failed to load a texture file! (" + fileName + ")");
		}

		uint32 width, height;
		uint16 bitsPerSample, samplePerPixel, photometric;
		TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width);
		TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height);
		TIFFGetField(tiff, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
		TIFFGetField(tiff, TIFFTAG_SAMPLESPERPIXEL, &samplePerPixel);
		TIFFGetField(tiff, TIFFTAG_PHOTOMETRIC, &photometric);

		if(samplePerPixel != 1 || photometric != PHOTOMETRIC_MINISBLACK) {
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

	void getInfoFromTiff(const std::string& fileName, ivec2& size, VkFormat& format, int& numChannels)
	{
		size = ivec2(0);
		format = VK_FORMAT_UNDEFINED;
		TIFF* tiff = TIFFOpen(fileName.c_str(), "r");
		if(tiff)
		{
			if(TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &size.x) != 1 ||
				TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &size.y) != 1)
			{
				LOG_ERROR("Could not retrieve the dimensions of the TIFF file: {}", fileName);
			}

			uint16_t samplesPerPixel = 1;
			uint16_t bitsPerSample = 1;
			uint16_t sampleFormat = SAMPLEFORMAT_UINT;

			if(
				TIFFGetFieldDefaulted(tiff, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel) != 1 ||
				TIFFGetFieldDefaulted(tiff, TIFFTAG_BITSPERSAMPLE, &bitsPerSample) != 1 ||
				TIFFGetFieldDefaulted(tiff, TIFFTAG_SAMPLEFORMAT, &sampleFormat) != 1)
			{
				LOG_ERROR("Could not retrieve the dimensions of the TIFF file: {}", fileName);
			}
			else
			{
				bool isFloat = (sampleFormat == SAMPLEFORMAT_IEEEFP);
				bool isSigned = (sampleFormat == SAMPLEFORMAT_INT);
				bool isUnsigned = (sampleFormat == SAMPLEFORMAT_UINT);

				// Map based on channels
				if(samplesPerPixel == 1) {
					if(bitsPerSample == 8)
						format = isSigned ? VK_FORMAT_R8_SNORM :
						VK_FORMAT_R8_UNORM;
					if(bitsPerSample == 16)
						format = isFloat ? VK_FORMAT_R16_SFLOAT :
						isSigned ? VK_FORMAT_R16_SINT :
						VK_FORMAT_R16_UINT;
					if(bitsPerSample == 32)
						format = isFloat ? VK_FORMAT_R32_SFLOAT :
						isSigned ? VK_FORMAT_R32_SINT :
						VK_FORMAT_R32_UINT;
				}
				if(samplesPerPixel == 2) {
					if(bitsPerSample == 8)
						format = isSigned ? VK_FORMAT_R8G8_SNORM :
						VK_FORMAT_R8G8_UNORM;
					if(bitsPerSample == 16)
						format = isFloat ? VK_FORMAT_R16G16_SFLOAT :
						isSigned ? VK_FORMAT_R16G16_SINT :
						VK_FORMAT_R16G16_UINT;
					if(bitsPerSample == 32)
						format = isFloat ? VK_FORMAT_R32G32_SFLOAT :
						isSigned ? VK_FORMAT_R32G32_SINT :
						VK_FORMAT_R32G32_UINT;
				}
				if(samplesPerPixel == 3) {
					if(bitsPerSample == 8)
						format = isSigned ? VK_FORMAT_R8G8B8_SNORM :
						VK_FORMAT_R8G8B8_UNORM;
					if(bitsPerSample == 16)
						format = isFloat ? VK_FORMAT_R16G16B16_SFLOAT :
						isSigned ? VK_FORMAT_R16G16B16_SINT :
						VK_FORMAT_R16G16B16_UINT;
					if(bitsPerSample == 32)
						format = isFloat ? VK_FORMAT_R32G32B32_SFLOAT :
						isSigned ? VK_FORMAT_R32G32B32_SINT :
						VK_FORMAT_R32G32B32_UINT;
				}
				if(samplesPerPixel == 4) {
					if(bitsPerSample == 8)
						format = isSigned ? VK_FORMAT_R8G8B8A8_SNORM :
						VK_FORMAT_R8G8B8A8_UNORM;
					if(bitsPerSample == 16)
						format = isFloat ? VK_FORMAT_R16G16B16A16_SFLOAT :
						isSigned ? VK_FORMAT_R16G16B16A16_SINT :
						VK_FORMAT_R16G16B16A16_UINT;
					if(bitsPerSample == 32)
						format = isFloat ? VK_FORMAT_R32G32B32A32_SFLOAT :
						isSigned ? VK_FORMAT_R32G32B32A32_SINT :
						VK_FORMAT_R32G32B32A32_UINT;
				}

				numChannels = samplesPerPixel;

				if(format == VK_FORMAT_UNDEFINED)
				{
					LOG_ERROR("Unsupported TIFF format: channels or bit depth not mapped");
				}
			}
			TIFFClose(tiff);
		}

		LOG_ERROR("Can't open TIFF file to retrieve the dimensions: {}", fileName);
	}
	
    void getInfoFromPng(const std::string& fileName, ivec2& size, VkFormat& format, int& numChannels)
    {
		stbi_info(fileName.c_str(), &size.x, &size.y, &numChannels);

		switch(numChannels)
		{
			case 1:
				format = VK_FORMAT_R8_UNORM;
				break;
			case 2:
				format = VK_FORMAT_R8G8_UNORM;
				break;
			case 3:
				format = VK_FORMAT_R8G8B8_UNORM;
				break;
			case 4:
				format = VK_FORMAT_R8G8B8A8_UNORM;
				break;
			default: LOG_ERROR("Unsupported channel count for file: {}", fileName);
		}
	}

	void Image::getInfo(const std::string& fileName, ivec2& size, VkFormat& format, int& numChannels)
	{
		if(fileName.find(".tiff") != std::string::npos || fileName.find(".tif") != std::string::npos)
		{
			getInfoFromTiff(fileName, size, format, numChannels);
		}
		else if(fileName.find(".png") != std::string::npos)
		{
			getInfoFromPng(fileName, size, format, numChannels);
		}
    }
}