#pragma once

#include "Log.hpp"
#include "Utilities.hpp"

#include <cufft.h>
#include <npp.h>

#include <stdexcept>

namespace fre
{
	//#define NVTX_DISABLE

	using InternalHeightmapFormat = uint16_t;
	using VulkanHeightmapFormat = uint16_t;

	/*#ifdef NDEBUG
		#define CUDA_CHECK(f) (f)

		#define CUDA_CHECK_ERROR()

		#define CUDA_LAUNCH_KERNEL(f) (f)

		#define CUFFT_CHECK(f) (f)

		#define NPP_CHECK(f) (f)
	#else*/
		void onCudaError(cudaError_t cudaResult, const char* f);

		#define CUDA_CHECK(f)\
		{\
			auto cudaResult = (f);\
			if(cudaResult != cudaSuccess)\
			{\
				onCudaError(cudaResult, #f);\
			}\
		}

		#define CUDA_CHECK_ERROR() CUDA_CHECK(cudaGetLastError())

		#define CUDA_LAUNCH_KERNEL(f)\
		{\
			f;\
			auto cudaResult = cudaGetLastError();\
			if(cudaResult != cudaSuccess)\
			{\
				onCudaError(cudaResult, #f);\
			}\
		}

		const char* cufftGetErrorString(cufftResult error);

		inline void onCufftError(cufftResult result)
		{
		}

		#define CUFFT_CHECK(f)\
		{\
			auto result = (f);\
			if (result != CUFFT_SUCCESS)\
			{\
				onCufftError(result);\
				LOG_ERROR("CUFFT error: {}. Call: {}", cufftGetErrorString(result), #f);\
				throw std::runtime_error(fre::formatString("CUFFT error: %d, %s. Call: %s", static_cast<unsigned int>(result), cufftGetErrorString(result), #f));\
			}\
		}

		inline void onNppError(NppStatus status)
		{
		}

		#define NPP_CHECK(f)\
		{\
			auto status = (f);\
			if (status != NPP_SUCCESS)\
			{\
				onNppError(status);\
				LOG_ERROR("NPP error: {}. Call: {}", status, #f);\
				throw std::runtime_error(fre::formatString("NPP error: %d. Call: %s", status, #f));\
			}\
		}
	//#endif
}