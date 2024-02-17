#include "Image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <stdexcept>

namespace fre
{
	void Image::load(const std::string& fileName)
	{
		//Number of channels image uses
		int channels;

		//Load pixel data of image
		std::string fileLoc = "Textures/" + fileName;
		mData = stbi_load(fileLoc.c_str(), &mDimension.x, &mDimension.y, &channels, STBI_rgb_alpha);

		if (mData == nullptr)
		{
			throw std::runtime_error("Failed to load a texture file! (" + fileName + ")");
		}

		mDataSize = mDimension.x * mDimension.y * 4;
 	}

    void Image::destroy()
    {
        stbi_image_free(mData);
    }
}