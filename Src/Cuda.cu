#include "Interop/Interop.hpp"
#include "Cuda.hpp"
#include "CudaBufferManager.hpp"
#include "Log.hpp"
#include "Utilities.hpp"

#include <stdexcept>

using namespace glm;

namespace fre
{
	cudaDeviceProp Cuda::mCudaProp;

	Cuda::Cuda()
		: mCudaExternalHeightMem(nullptr)
		, mCudaWaitSemaphore(nullptr)
		, mCudaSignalSemaphore(nullptr)
		, mSrcValid(true)
	{
	}

	bool Cuda::init(VulkanRenderer* renderer)
	{
		int currentDevice = 0;
		int deviceCount = 0;
		int devices_prohibited = 0;

		CUDA_CHECK(cudaGetDeviceCount(&deviceCount));

		if(deviceCount == 0)
		{
			LOG_ERROR("CUDA error: no devices supporting CUDA.");
			throw std::runtime_error("CUDA error: no devices supporting CUDA.");
		}

		// Find the GPU which is selected by Vulkan
		while(currentDevice < deviceCount)
		{
			CUDA_CHECK(cudaGetDeviceProperties(&mCudaProp, currentDevice));

			if((mCudaProp.computeMode != cudaComputeModeProhibited))
			{
				// Compare the cuda device UUID with vulkan UUID
				int ret = memcmp((void*)&mCudaProp.uuid, renderer->getDeviceUID(), VK_UUID_SIZE);
				if(ret == 0)
				{
					CUDA_CHECK(cudaSetDevice(currentDevice));
					CUDA_CHECK(cudaGetDeviceProperties(&mCudaProp, currentDevice));
					LOG_TRACE("GPU Device {}: {} with compute capability {}.{}",
						currentDevice, mCudaProp.name, mCudaProp.major,
						mCudaProp.minor);

					mDevice = currentDevice;
					break;
				}
			}
			else
			{
				devices_prohibited++;
			}

			currentDevice++;
		}

		if(devices_prohibited == deviceCount)
		{
			LOG_ERROR("CUDA error: No Vulkan-CUDA Interop capable GPU found");
			throw std::runtime_error("CUDA error: No Vulkan-CUDA Interop capable GPU found.\n");
		}

		CUDA_CHECK(cudaSetDevice(mDevice));
		LOG_TRACE("Cuda maxGridSize: {}, {}, {}", mCudaProp.maxGridSize[0], mCudaProp.maxGridSize[1], mCudaProp.maxGridSize[2]);
		LOG_TRACE("Cuda maxThreadsDim: {}, {}, {}", mCudaProp.maxThreadsDim[0], mCudaProp.maxThreadsDim[1], mCudaProp.maxThreadsDim[2]);
		LOG_TRACE("Cuda maxThreadsPerBlock: {}", mCudaProp.maxThreadsPerBlock);
		LOG_TRACE("Cuda sharedMemPerBlock: {}", mCudaProp.sharedMemPerBlock);
		LOG_TRACE("Cuda concurrentKernels: {}", mCudaProp.concurrentKernels);
		LOG_TRACE("Cuda asynchronous engines count: {}", mCudaProp.asyncEngineCount);

		CUDA_CHECK(cudaStreamCreateWithFlags(&mCudaStream, cudaStreamNonBlocking));

		//Vulkan will wait for cuda's signal
		auto mExternalVulkanWaitSemaphore = renderer->getExternalWaitSemaphore();
		importCudaExternalSemaphore(mCudaSignalSemaphore, mExternalVulkanWaitSemaphore,
			getDefaultSemaphoreHandleType(), renderer);

		//Cuda will wait for Vulkan's signal
		auto mExternalVulkanSignalSemaphore = renderer->getExternalSignalSemaphore();
		importCudaExternalSemaphore(mCudaWaitSemaphore, mExternalVulkanSignalSemaphore,
			getDefaultSemaphoreHandleType(), renderer);

		LOG_TRACE("Cuda-Vulkan interoperation initialized");

		return true;
	}

	void Cuda::importExternalMemory(void** externalData, void* cudaData, const ivec2& size)
	{
		if(mDevice != -1)
		{
			//Import the Vulkan memory into CUDA memory and retrieve a device pointer to use
			importCudaExternalMemory((void**)&mCudaHeights.mData, mCudaExternalHeightMem,
				vulkanExternalHeightMem, actualImageSize,
				getDefaultMemHandleType(), renderer);

			mActualHeightMapSize = ivec2(actualImageSize / image->mStride / heightMapSize.y, heightMapSize.y);

			mCudaHeights.mElementsCount.x = mActualHeightMapSize.x;
			mCudaHeights.mElementsCount.y = mActualHeightMapSize.y;
			mCudaHeights.mElementsCount.z = 1;

			//Here we know actual heightmap texture image size and we can create all temporary arrays
			mMaxActualHMDim = max(mActualHeightMapSize.x, mActualHeightMapSize.y);
				}
			}

			if(mCudaExternalHeightMem != nullptr)
			{
				//int bufElementsCount = mSrcImage.mElementsCount.x * mSrcImage.mElementsCount.y;
				auto& bufferManager = mBufferManager;

				CUDA_CHECK_ERROR();
			}
		}
	}
	}

	void Cuda::destroyExternalMemory()
	{
		if(mCudaExternalHeightMem != nullptr)
		{
			CUDA_CHECK(cudaDestroyExternalMemory(mCudaExternalHeightMem));
			mCudaExternalHeightMem = nullptr;
		}
	}

	void Cuda::destroy()
	{
		if(mDevice != -1)
		{
			if(!mLocked)
			{
				CUDA_CHECK(cudaStreamSynchronize(mCudaStream));
			}
			mBufferManager.unlock(mSrcImage);

			mBufferManager.requestCleanup();
			mBufferManager.cleanup(false);

			destroyExternalMemory();

			if(mCudaWaitSemaphore != nullptr)
			{
				CUDA_CHECK(cudaDestroyExternalSemaphore(mCudaWaitSemaphore));
			}

			if(mCudaSignalSemaphore != nullptr)
			{
				CUDA_CHECK(cudaDestroyExternalSemaphore(mCudaSignalSemaphore));
			}

			CUDA_CHECK(cudaStreamDestroy(mCudaStream));

			LOG_TRACE("Cuda-Vulkan interoperation destroyed");
		}
	}

	void Cuda::lock()
	{
		if(mDevice != -1)
		{
			cudaExternalSemaphoreWaitParams waitParams = {};
			waitParams.flags = 0;
			waitParams.params.fence.value = 0;

			// Wait for vulkan to complete it's work
			CUDA_CHECK(cudaWaitExternalSemaphoresAsync(&mCudaWaitSemaphore, &waitParams, 1, mCudaStream));
			mLocked = true;

			mBufferManager.cleanup(true);
		}
	}

	void Cuda::unlock()
	{
		if(mDevice != -1)
		{
			cudaExternalSemaphoreSignalParams signalParams = {};
			signalParams.flags = 0;
			signalParams.params.fence.value = 0;

			// Signal vulkan to continue with the updated buffers
			CUDA_CHECK(cudaSignalExternalSemaphoresAsync(&mCudaSignalSemaphore, &signalParams, 1, mCudaStream));
			mLocked = false;
		}
	}
}