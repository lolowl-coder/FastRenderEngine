#include "CudaUtilities.hpp"

namespace fre
{
	void onCudaError(cudaError_t cudaResult, const char* f)
	{
		LOG_ERROR("CUDA error: {}, {}. Call: {}", static_cast<unsigned int>(cudaResult), cudaGetErrorName(cudaResult), f);
	}

	const char* cufftGetErrorString(cufftResult result)
	{
		switch(result)
		{
		case CUFFT_SUCCESS:
			return "CUFFT_SUCCESS";
		case CUFFT_INVALID_PLAN:
			return "CUFFT_INVALID_PLAN";
		case CUFFT_ALLOC_FAILED:
			return "CUFFT_ALLOC_FAILED";
		case CUFFT_INVALID_TYPE:
			return "CUFFT_INVALID_TYPE";
		case CUFFT_INVALID_VALUE:
			return "CUFFT_INVALID_VALUE";
		case CUFFT_INTERNAL_ERROR:
			return "CUFFT_INTERNAL_ERROR";
		case CUFFT_EXEC_FAILED:
			return "CUFFT_EXEC_FAILED";
		case CUFFT_SETUP_FAILED:
			return "CUFFT_SETUP_FAILED";
		case CUFFT_INVALID_SIZE:
			return "CUFFT_INVALID_SIZE";
		case CUFFT_UNALIGNED_DATA:
			return "CUFFT_UNALIGNED_DATA";
		case CUFFT_INCOMPLETE_PARAMETER_LIST:
			return "CUFFT_INCOMPLETE_PARAMETER_LIST";
		case CUFFT_INVALID_DEVICE:
			return "CUFFT_INVALID_DEVICE";
		case CUFFT_PARSE_ERROR:
			return "CUFFT_PARSE_ERROR";
		case CUFFT_NO_WORKSPACE:
			return "CUFFT_NO_WORKSPACE";
		default:
			return "Unknown CUFFT error";
		}
	}
}