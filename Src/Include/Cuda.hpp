#pragma once

#include "CudaBuffer.hpp"
#include "CudaBufferManager.hpp"
#include "Pointers.hpp"
#include "Renderer/VulkanRenderer.hpp"

#include <glm/glm.hpp>

#include <filesystem>
#include <cuda_runtime_api.h>
#include <cstdint>

namespace fre
{
	class CudaBufferManager;

	//This class runs CUDA kernels. It is synchronized with Vulkan API.
	class Cuda
	{
	public:
		using ExportCallback = std::function<void(bool)>;

		Cuda();
		bool init(VulkanRenderer* renderer);
		void destroy();
		
		//Lock rendering
		void lock();
		//Unlock rendering
		void unlock();

		int* getDevice() { return &mDevice; }
		const cudaDeviceProp& getProps() { return mCudaProp; }

		cudaStream_t& getStream() { return mCudaStream; }
	
		// Import Vulkan owned external memory
		void importExternalMemory();
		// Destroys external memory
		void destroyExternalMemory();
	public:
		static cudaDeviceProp mCudaProp;

	private:
		//Prepares image before filtering
		void prepareFilteredImage(CudaBuffer<InternalHeightmapFormat>& filteredImage, const glm::ivec2& heightMapSize, const bool isCameraAccquing);
	
	private:
		//Imported CUDA memory handle
		cudaExternalMemory_t mCudaExternalHeightMem;

		//CUDA device id
		int mDevice = -1;

		//Locked flag
		bool mLocked = false;

		//Streams for concurent kernel execution
		cudaStream_t mCudaStream = 0;

		//Cuda-Vulkan synchronization primitives
		cudaExternalSemaphore_t mCudaWaitSemaphore;
		cudaExternalSemaphore_t mCudaSignalSemaphore;
	};
}