#pragma once

#include "Renderer/VulkanRenderer.hpp"
#include "CudaBuffer.hpp"
#include "CudaUtilities.hpp"
#include "Log.hpp"
#include "Utilities.hpp"

#include <glm/glm.hpp>
#include <map>
#include <set>

#include <cuda_fp16.h>

namespace fre
{
	//Internal buffer data stored in BufferManager. Abstract representation with untyped data.
	struct InternalBuffer
	{
		void* mData = nullptr;
		std::string mName;
		uint64_t mUsageFrame = 0;

		//Differentiate by data address
		bool operator<(const InternalBuffer& other) const {
			return mData < other.mData;
		}
	};

	class CudaBufferManager
	{
	public:
		using TBuffers = std::map<uint64_t, std::set<InternalBuffer>>;

		CudaBufferManager(const std::string& name)
			: mName(name)
		{
			LOG_INFO("{} CudaBufferManager created", mName);
		}

		~CudaBufferManager()
		{
			LOG_INFO("{} CudaBufferManager efficiency: {}%", mName, mLockRequests > 0 ? static_cast<float>(mHits) / static_cast<float>(mLockRequests) : 0);
		}

		void lockImpl(const uint64_t size, InternalBuffer& result, const std::string& name);

		template<typename T>
		CudaBuffer<T> lock(const std::string& name, uint64_t elementsCountX, uint64_t elementsCountY = 1, uint64_t elementsCountZ = 1)
		{
			CudaBuffer<T> result;
			//static T* dummy;
			//result.mDataType = getDataType(dummy);
			result.mDimensions = glm::u64vec3(elementsCountX, elementsCountY, elementsCountZ);

			InternalBuffer internalBuffer;
			lockImpl(result.getSize(), internalBuffer, name);
			//Update name, because it can be changed
			result.mData = static_cast<T*>(internalBuffer.mData);
			return result;
		}

		template<typename T>
		CudaBuffer<T> lock(const std::string& name, const glm::u64vec3& elementsCount)
		{
			return lock<T>(name, elementsCount.x, elementsCount.y, elementsCount.z);
		}

		template<typename T>
		CudaBuffer<T> lock(const std::string& name, const glm::u64vec2& elementsCount)
		{
			return lock<T>(name, elementsCount.x, elementsCount.y, 1);
		}

		template<typename T>
		CudaBuffer<T> createExternalBuffer(const uint32_t textureId, VulkanRenderer* renderer)
		{
            CudaBuffer<T> result;
			cudaExternalMemory_t cudaExternalMem;
			// Get Vulkan texture memory
			VkDeviceMemory vulkanExternalMem = renderer->getTextureManager()->getTextureMemory(textureId);
			// Get image for actual size
            Image& image = renderer->getTextureInfo(textureId)->mImage;
			VulkanTexturePtr& texture = renderer->getTexture(textureId);
			importCudaExternalMemory((void**)&result.mData, cudaExternalMem, vulkanExternalMem,
				texture->mActualSize,
				getDefaultMemHandleType(), renderer);
			glm::ivec2 actualImageDimensions = ivec2(
				texture->mActualSize / image.mStride / image.mDimension.y,
				image.mDimension.y);
			result.mDimensions.x = actualImageDimensions.x;
			result.mDimensions.y = actualImageDimensions.y;
			result.mDimensions.z = 1;
            mExternalMems.push_back(cudaExternalMem);

			return result;
		}

		void unlockImpl(const uint64_t size, InternalBuffer& buffer);

		template<typename T>
		void unlock(T& buffer)
		{
			if(buffer.getLength() > 0 && buffer.mData != nullptr)
			{
				InternalBuffer internalBuffer;
				internalBuffer.mData = buffer.mData;
				internalBuffer.mName = "unlocked";
				unlockImpl(buffer.getSize(), internalBuffer);
				//buffer.mData = static_cast<T*>(internalBuffer.mData);
				buffer.mData = nullptr;
			}
		}

		void freeImpl(const uint64_t size, InternalBuffer& buffer);

		template<typename T>
		void free(T& buffer)
		{
			InternalBuffer internalBuffer;
			internalBuffer.mData = buffer.mData;
			freeImpl(buffer.getSize(), internalBuffer);
			buffer.mData = nullptr;
		}

		void requestCleanup()
		{
			mCleanupRequested = true;
		}

		void cleanup(bool unusedOnly);

		template<typename TDst, typename TSrc>
		static void copy(CudaBuffer<TDst>& dst, const CudaBuffer<TSrc>& src, cudaStream_t stream)
		{
			CUDA_CHECK(cudaMemcpyAsync(dst.mData, src.mData, src.getSize(), cudaMemcpyDeviceToDevice, stream));
		}

		template<typename T>
		void saveToTiffReal(const std::string& fileName, const bool normalize, const CudaBuffer<T>& buffer, cudaStream_t stream);

		template<typename C>
		void saveToTiffComplex(const std::string& fileName, const CudaBuffer<C>& buffer, const bool dumpRe, const bool dumpIm, cudaStream_t stream);
		
		template<typename T>
		bool saveToCSVReal(const std::string& fileName, const CudaBuffer<T>& buffer, cudaStream_t stream);

		template<typename T>
		bool saveToCSVComplex(const std::string& fileNameRe, const std::string& fileNameIm, const CudaBuffer<T>& buffer, cudaStream_t stream);

	private:
		const std::string mName;
		TBuffers mBuffers;
		TBuffers mLockedBuffers;
		std::vector<cudaExternalMemory_t> mExternalMems;
		bool mCleanupRequested = false;
		uint64_t mFrameNumber = 0;
		uint64_t mLockRequests = 0;
		uint64_t mHits = 0;
		uint64_t mTotalMemoryAllocated = 0;
	};
}