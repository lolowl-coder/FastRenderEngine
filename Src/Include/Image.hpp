#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include "stb_image.h"

#include <glm/glm.hpp>

#include <string>
#include <limits>

namespace fre
{
    struct Image
    {
        void create(const glm::ivec2& dimension, const VkFormat format);
        void calculateStrideAndDataSize();
        //Loads image. Supported formats - the same as stb_image does.
        void load();
        void loadPng(const std::string& fileName);
        void loadTIFF(const std::string& fileName);
        static void saveTIFF(const std::string& fileName, uint16_t* data, int width, int height);
        //Destroy image data
        void destroy();
        //Chack file name for validity
        bool isFileNameValid() const;

        glm::ivec2 mDimension;
        //Data size stored in mData, bytes
        uint32_t mDataSize = 0;
        void* mData = nullptr;
        VkFormat mFormat = VK_FORMAT_R8G8B8A8_UNORM;
        std::string mFileName;
        bool mIsExternal = false;
        //Pixel size, bytes
        uint32_t mStride = 0;
        //Does this image own the data
        bool mIsOwner = false;
        bool mIsTIFF = false;
        bool mIsPNG = false;
        int mNumChannels = 0;

        static void getInfo(const std::string& fileName, glm::ivec2& size, VkFormat& format, int& numChannels);

        bool operator == (const Image& other) const
        {
            return
                mDimension == other.mDimension &&
                mDataSize == other.mDataSize &&
                mFormat == other.mFormat &&
                mIsExternal == other.mIsExternal &&
                mStride == other.mStride &&
                mIsOwner == other.mIsOwner &&
                mIsTIFF == other.mIsTIFF &&
                mIsPNG == other.mIsPNG &&
                mNumChannels == other.mNumChannels &&
                mFileName == other.mFileName &&
                mData == other.mData;
        }
    };
}