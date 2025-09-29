#pragma once

#include "CudaUtilities.hpp"

#include <cuda_runtime_api.h>

#include <map>

#define KM auto& km = CudaKernelManager::getInstance()

#define LAUNCH_KERNEL(size,stream,kernel,...)\
{\
	KM;\
	auto config = km.getKernelConfig(kernel, size);\
	CUDA_LAUNCH_KERNEL((kernel<<<config.first, config.second, 0, stream>>>(__VA_ARGS__)));\
}

namespace fre
{
	class CudaKernelManager
	{
	public:
		using KernelConfig = std::pair<int, int>;

		CudaKernelManager(const CudaKernelManager&) = delete;
		CudaKernelManager& operator=(const CudaKernelManager&) = delete;

		static CudaKernelManager& getInstance()
		{
			// Static local variable to ensure only one instance is created
			static CudaKernelManager km;
			return km;
		}

		template <typename KernelFunc>
		KernelConfig getKernelConfig(KernelFunc kernel, int N)
		{
			// Check if kernel config is already stored
			auto it = kernelConfigMap.find((void*)kernel);
        
			if (it == kernelConfigMap.end()) {
				// First-time launch, calculate optimal config
				int minGridSize = 0, optimalBlockSize = 0;
				CUDA_CHECK(cudaOccupancyMaxPotentialBlockSize(&minGridSize, &optimalBlockSize, kernel, 0, 0));

				int threads = optimalBlockSize;
				//int threads = min(Cuda::mCudaProp.maxThreadsDim[0], N) / 16;
				int blocks = (N + threads - 1) / threads;
				// Store the config
				kernelConfigMap[(void*)kernel] = { blocks, threads };
			}

			// Get the config
			auto config = kernelConfigMap[(void*)kernel];

			return config;
		}

		template <typename KernelFunc, typename... Args>
		void launchKernel(KernelFunc kernel, cudaStream_t stream, int N, Args... args)
		{
			// Get the config
			auto config = getKernelConfig(kernel, N);
			dim3 blocks = config.first;
			dim3 threads = config.second;

			// Pack arguments
			void* packedArgs[] = { &args... };

			// Launch the kernel
			CUDA_CHECK(cudaLaunchKernel((void*)kernel, blocks, threads, packedArgs, 0, stream));
			CUDA_CHECK_ERROR();
		}

	private:
		CudaKernelManager() {}
		std::map<void*, KernelConfig> kernelConfigMap;
	};
}