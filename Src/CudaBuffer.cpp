#include "CudaBuffer.hpp"
#include "Image.hpp"

namespace fre
{
	template struct CudaBuffer<__half>;
	template struct CudaBuffer<float>;
	template struct CudaBuffer<uint8_t>;
	template struct CudaBuffer<uint16_t>;
	template struct CudaBuffer<cufftComplex>;
	template struct CudaBuffer<float4>;
}