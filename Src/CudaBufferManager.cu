#include "CudaBufferManager.hpp"
#include "Image.hpp"

#include <iomanip>

using namespace glm;

namespace fre
{
	void CudaBufferManager::lockImpl(const uint64_t size, InternalBuffer& result, const std::string& name)
	{
		mLockRequests++;
		result.mData = nullptr;
		//uint64_t size = count * sizeof(T);
		auto foundGroup = mBuffers.find(size);
		if(foundGroup != mBuffers.end() && !foundGroup->second.empty())
		{
			//result = static_cast<T*>(*foundGroup->second.begin());
			result = *foundGroup->second.begin();
			result.mUsageFrame = mFrameNumber;
			result.mName = name;
			foundGroup->second.erase(result);
			mLockedBuffers[size].insert(result);
			mHits++;
		}
		else
		{
			result.mUsageFrame = mFrameNumber;
			result.mName = name;
			CUDA_CHECK(cudaMalloc((void **)&result.mData, size));
			mLockedBuffers[size].insert(result);
			mTotalMemoryAllocated += size;
			LOG_TRACE("{} Allocate cuda buffer: {}, {}, {} bytes. Total allocated: {}", mName, name, result.mData, size, mTotalMemoryAllocated);
		}
	}

	void CudaBufferManager::unlockImpl(const uint64_t size, InternalBuffer& buffer)
	{
		//uint64_t size = count * sizeof(T);
		auto foundGroup = mLockedBuffers.find(size);
		if(foundGroup != mLockedBuffers.end())
		{
			auto foundBuffer = foundGroup->second.find(buffer);
			if(foundBuffer != foundGroup->second.end())
			{
				mBuffers[size].insert(*foundBuffer);
				foundGroup->second.erase(foundBuffer);
				buffer.mData = nullptr;
			}
			else
			{
				throw std::runtime_error(fre::formatString("%s Cuda buffers consistency check failed! %s", mName, buffer.mName));
			}
		}
		else
		{
			throw std::runtime_error(fre::formatString("%s Cuda buffers consistency check failed! %s", mName, buffer.mName));
		}
	}

	void CudaBufferManager::freeImpl(const uint64_t size, InternalBuffer& buffer)
	{
		//uint64_t size = count * sizeof(T);
		auto foundGroup = mLockedBuffers.find(size);
		if(foundGroup != mLockedBuffers.end())
		{
			auto foundBuffer = foundGroup->second.find(buffer);
			if(foundBuffer != foundGroup->second.end())
			{
				CUDA_CHECK(cudaFree((*foundBuffer).mData));
				foundGroup->second.erase(foundBuffer);
				buffer.mData = nullptr;
				mTotalMemoryAllocated -= size;
			}
			else
			{
				throw std::runtime_error(fre::formatString("%s Cuda buffers consistency check failed!", mName));
			}
		}
		else
		{
			throw std::runtime_error(fre::formatString("%s Cuda buffers consistency check failed!", mName));
		}
	}

	void CudaBufferManager::cleanup(bool unusedOnly)
	{
		mFrameNumber++;
		if(mCleanupRequested)
		{
            // Clean up unused buffers
			CUDA_CHECK(cudaDeviceSynchronize());
			for(auto& buffers : mBuffers)
			{
				for(auto buffer = buffers.second.begin(); buffer != buffers.second.end();)
				{
					if(unusedOnly && mFrameNumber - buffer->mUsageFrame > 1 || !unusedOnly)
					{
 						CUDA_CHECK(cudaFree(buffer->mData));
						mTotalMemoryAllocated -= buffers.first;
						LOG_TRACE("{} Free cuda buffer: {}, {}. Total allocated: {}", mName, buffer->mName, buffer->mData, mTotalMemoryAllocated);
						buffer = buffers.second.erase(buffer);
					}
					else
					{
							buffer++;
					}
				}
			}

            // If requested, clean up all buffers and report errors if any buffer was not freed properly
			if(!unusedOnly)
			{
				std::vector<std::string> errors;
				if(!mLockedBuffers.empty())
				{
					for(auto& buffers : mLockedBuffers)
					{
						for(auto& buffer : buffers.second)
						{
							if(buffer.mData != nullptr)
							{
								errors.push_back(formatString("%s, 0x%p, %u bytes", buffer.mName.c_str(), buffer.mData, buffers.first));
							}
						}
					}
				}
				if(!errors.empty())
				{
					LOG_ERROR("{} Not all CUDA-buffer are cleaned up properly.", mName);
					for(auto& error : errors)
					{
						LOG_ERROR(error);
					}
				}
				for(auto& buffers : mLockedBuffers)
				{
					for(auto buffer = buffers.second.begin(); buffer != buffers.second.end(); buffer++)
					{
						mTotalMemoryAllocated -= buffers.first;
						LOG_TRACE("{} Free cuda buffer: {}, {}. Total allocated: {}", mName, buffer->mName, buffer->mData, mTotalMemoryAllocated);
						CUDA_CHECK(cudaFree(buffer->mData));
						//buffer = buffers.second.erase(buffer);
					}
				}
				mLockedBuffers.clear();

                for(auto& mem : mExternalMems)
                {
					CUDA_CHECK(cudaDestroyExternalMemory(mem));
                }
				mExternalMems.clear();
			}
		}
	}
}