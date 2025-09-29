#include "CudaUtilities.hpp"
#include "Kernels.hpp"
#include "CudaKernelManager.hpp"
#include "Cuda.hpp"

namespace fre
{
	template void transposeImage<float, float>(float* dst, const float* src, int dstWidth, int dstHeight, int srcWidth, int srcHeight, int roiWidth, int roiHeight, cudaStream_t stream);
	template void transposeImage<float, __half>(float* dst, const __half* src, int dstWidth, int dstHeight, int srcWidth, int srcHeight, int roiWidth, int roiHeight, cudaStream_t stream);
	template void transposeImage<__half, float>(__half* dst, const float* src, int dstWidth, int dstHeight, int srcWidth, int srcHeight, int roiWidth, int roiHeight, cudaStream_t stream);
	template void transposeImage<__half, __half>(__half* dst, const __half* src, int dstWidth, int dstHeight, int srcWidth, int srcHeight, int roiWidth, int roiHeight, cudaStream_t stream);
	template void transposeImage<float, uint16_t>(float* dst, const uint16_t* src, int dstWidth, int dstHeight, int srcWidth, int srcHeight, int roiWidth, int roiHeight, cudaStream_t stream);
	template void transposeImage<uint16_t, __half>(uint16_t* dst, const __half* src, int dstWidth, int dstHeight, int srcWidth, int srcHeight, int roiWidth, int roiHeight, cudaStream_t stream);
	template void transposeImage<uint16_t, uint16_t>(uint16_t* dst, const uint16_t* src, int dstWidth, int dstHeight, int srcWidth, int srcHeight, int roiWidth, int roiHeight, cudaStream_t stream);

	template void minMax<float>(const CudaBuffer<float>& src, CudaBuffer<float>& mnMx, cudaStream_t stream);
	template void minMax<__half>(const CudaBuffer<__half>& src, CudaBuffer<float>& mnMx, cudaStream_t stream);

	__global__ void toFloatComplex(cufftComplex* __restrict__ dst, const cufftDoubleComplex* __restrict__ src, const int size)
	{
		int tid = blockIdx.x * blockDim.x + threadIdx.x;
		if(tid < size)
		{
			const cufftDoubleComplex& srcLocal = src[tid];
			cufftComplex& dstLocal = dst[tid];
			dstLocal.x = (float)srcLocal.x;
			dstLocal.y = (float)srcLocal.y;
		}
	}

	__global__ void toDoubleComplex(cufftDoubleComplex* __restrict__ dst, const cufftComplex* __restrict__ src, const int size)
	{
		int tid = blockIdx.x * blockDim.x + threadIdx.x;
		if(tid < size)
		{
			const cufftComplex& srcLocal = src[tid];
			cufftDoubleComplex& dstLocal = dst[tid];
			dstLocal.x = (double)srcLocal.x;
			dstLocal.y = (double)srcLocal.y;
		}
	}

	template <typename TDst, typename TSrc>
	__global__ void transposeImageKernel(TDst* __restrict__ dst, const TSrc* __restrict__ src, int dstWidth, int dstHeight, int srcWidth, int srcHeight, int roiWidth, int roiHeight) {
		__shared__ float tile[32][33]; // Use 33 to avoid bank conflicts

		int xIndex = blockIdx.x * 32 + threadIdx.x;
		int yIndex = blockIdx.y * 32 + threadIdx.y;

		// Load data into shared memory
		tile[threadIdx.y][threadIdx.x] = src[max(0, min(roiHeight, yIndex)) * srcWidth + max(0, min(roiWidth, xIndex))];

		__syncthreads();

		// Calculate transposed indices
		int transposedXIndex = blockIdx.y * 32 + threadIdx.x;
		int transposedYIndex = blockIdx.x * 32 + threadIdx.y;

		if (transposedXIndex < roiWidth && transposedYIndex < roiHeight) {
			// Write transposed data from shared memory to global memory
			dst[transposedYIndex * dstWidth + transposedXIndex] = tile[threadIdx.x][threadIdx.y];
		}
	}

	template <typename TDst, typename TSrc>
	void transposeImage(TDst* __restrict__ dst, const TSrc* __restrict__ src, int dstWidth, int dstHeight, int srcWidth, int srcHeight, int roiWidth, int roiHeight, cudaStream_t stream) {
		dim3 blockSize(32, 32);
		dim3 gridSize((roiWidth + 31) / 32, (roiHeight + 31) / 32);

		CUDA_LAUNCH_KERNEL((transposeImageKernel<<<gridSize, blockSize, 0, stream>>>(dst, src, dstWidth, dstHeight, srcWidth, srcHeight, roiWidth, roiHeight)));
	}

	__global__ void f32Tof16Kernel(__half* __restrict__ dst, const float* __restrict__ src, const int dstWidth, const int srcWidth, const int roiWidth, const int roiHeight)
    {
        const int stride = gridDim.x * blockDim.x;

	    for(int tid = blockIdx.x * blockDim.x + threadIdx.x; tid < roiWidth * roiHeight; tid += stride)
        {
            const int y = tid / roiWidth;
			const int x = tid - y * roiWidth;
            if (x < roiWidth && y < roiHeight)
            {
                dst[y * dstWidth + x] = src[y * srcWidth + x];
            }
        }
    }

	void f32Tof16(__half* __restrict__ dst, const float* __restrict__ src, const int dstWidth, const int srcWidth, const int roiWidth, const int roiHeight, cudaStream_t stream)
	{
		const int threads = min(Cuda::mCudaProp.maxThreadsDim[0], roiWidth) / 8;
		const int blocks = ceil(roiWidth * roiHeight / static_cast<float>(threads));

		CUDA_LAUNCH_KERNEL((f32Tof16Kernel<<<blocks, threads, 0, stream>>>(dst, src, dstWidth, srcWidth, roiWidth, roiHeight)));
	}

	__global__ void f16Tof32Kernel(float* __restrict__ dst, const __half* __restrict__ src, const int dstWidth, const int srcWidth, const int roiWidth, const int roiHeight)
    {
        const int stride = gridDim.x * blockDim.x;

	    for(int tid = blockIdx.x * blockDim.x + threadIdx.x; tid < roiWidth * roiHeight; tid += stride)
        {
            const int y = tid / roiWidth;
			const int x = tid - y * roiWidth;
			if (x < roiWidth && y < roiHeight)
			{
				float tmp = __half2float(src[y * srcWidth + x]);
				//Clamp to avoid abnormal values
				tmp = max(0.0f, min(65535.0f, tmp));
				dst[y * dstWidth + x] = tmp;
			}
        }
    }

	void f16Tof32(float* dst, const __half* src, const int dstWidth, const int srcWidth, const int roiWidth, const int roiHeight, cudaStream_t stream)
	{
		const int threads = min(Cuda::mCudaProp.maxThreadsDim[0], roiWidth) / 8;
		const int blocks = ceil(roiWidth * roiHeight / static_cast<float>(threads));

		CUDA_LAUNCH_KERNEL((f16Tof32Kernel<<<blocks, threads, 0, stream>>>(dst, src, dstWidth, srcWidth, roiWidth, roiHeight)));
	}
	
	__global__ void f16Tou16Kernel(uint16_t* __restrict__ dst, const __half* __restrict__ src, const int dstWidth, const int srcWidth, const int roiWidth, const int roiHeight)
    {
        const int stride = gridDim.x * blockDim.x;

	    for(int tid = blockIdx.x * blockDim.x + threadIdx.x; tid < roiWidth * roiHeight; tid += stride)
        {
            const int y = tid / roiWidth;
			const int x = tid - y * roiWidth;
            dst[y * dstWidth + x] = __half2float(src[y * srcWidth + x]);
        }
    }

	void f16Tou16(uint16_t* __restrict__ dst, const __half* __restrict__ src, const int dstWidth, const int srcWidth, const int roiWidth, const int roiHeight, cudaStream_t stream)
	{
		const int threads = min(Cuda::mCudaProp.maxThreadsDim[0], roiWidth) / 8;
		const int blocks = roiWidth * roiHeight / threads;
		CUDA_LAUNCH_KERNEL((f16Tou16Kernel<<<blocks, threads, 0, stream>>>(dst, src, dstWidth, srcWidth, roiWidth, roiHeight)));
	}
	
	__global__ void f32Tou16Kernel(uint16_t* __restrict__ dst, const float* __restrict__ src, const int dstWidth, const int srcWidth, const int roiWidth, const int roiHeight)
    {
        const int stride = gridDim.x * blockDim.x;

	    for(int tid = blockIdx.x * blockDim.x + threadIdx.x; tid < roiWidth * roiHeight; tid += stride)
        {
            const int y = tid / roiWidth;
			const int x = tid - y * roiWidth;
            if (x < roiWidth && y < roiHeight)
            {
				float tmp = src[y * srcWidth + x];
				tmp = max(0.0f, min(65535.0f, tmp));
                dst[y * dstWidth + x] = tmp;
            }
        }
    }

	void f32Tou16(uint16_t* __restrict__ dst, const float* __restrict__ src, const int dstWidth, const int srcWidth, const int roiWidth, const int roiHeight, cudaStream_t stream)
	{
		const int threads = min(Cuda::mCudaProp.maxThreadsDim[0], roiWidth) / 8;
		const int blocks = ceil(roiWidth * roiHeight / static_cast<float>(threads));

		CUDA_LAUNCH_KERNEL((f32Tou16Kernel<<<blocks, threads, 0, stream>>>(dst, src, dstWidth, srcWidth, roiWidth, roiHeight)));
	}

	__device__ __forceinline__ float atomicMinFloat(float* addr, const float value)
	{
		float old = *addr, assumed;
		do
		{
			assumed = old;
			old = atomicCAS((int*)addr, __float_as_int(assumed), __float_as_int(fminf(value, assumed)));
		} while(assumed != old);
		return old;
	}

	__device__ __forceinline__ float atomicMaxFloat(float* addr, const float value)
	{
		float old = *addr, assumed;
		do
		{
			assumed = old;
			old = atomicCAS((int*)addr, __float_as_int(assumed), __float_as_int(fmaxf(value, assumed)));
		} while(assumed != old);
		return old;
	}

	template<typename T>
	__global__ void minMaxKernel(const T* __restrict__ src, float* mnMx, const int size)
	{
		extern __shared__ float sharedData[];

		// Each thread loads one element into shared memory
		const int tid = threadIdx.x + blockIdx.x * blockDim.x;
		const int index = threadIdx.x;

		// Initialize shared memory
		float* sharedMin = sharedData;
		float* sharedMax = sharedData + blockDim.x;

		// Set initial values in shared memory
		if(threadIdx.x == 0)
		{
			sharedMin[0] = FLT_MAX;
			sharedMax[0] = -FLT_MAX;
		}
		__syncthreads();

		// Load data into shared memory
		if(tid < size)
		{
			const float value = (float)src[tid];
			sharedMin[index] = value;
			sharedMax[index] = value;
		}
		else
		{
			sharedMin[index] = FLT_MAX;
			sharedMax[index] = -FLT_MAX;
		}
		__syncthreads();

		// Perform reduction within the block
		for(int s = blockDim.x / 2; s > 0; s >>= 1)
		{
			if(index < s)
			{
				sharedMin[index] = min(sharedMin[index], sharedMin[index + s]);
				sharedMax[index] = max(sharedMax[index], sharedMax[index + s]);
			}
			__syncthreads();
		}

		// Write block-level min/max to global memory
		if(index == 0)
		{
			atomicMinFloat(&mnMx[0], sharedMin[0]);
			atomicMaxFloat(&mnMx[1], sharedMax[0]);
		}
	}

	template<typename T>
	void minMax(const CudaBuffer<T>& src, CudaBuffer<float>& mnMx, cudaStream_t stream)
	{
		int threads = 256; // Choose a number based on your hardware
        int blocks = (src.getLength() + threads - 1) / threads;
		float mnMxInit[2] = {FLT_MAX, FLT_MIN};
		CUDA_CHECK(cudaMemcpy(mnMx.mData, &mnMxInit[0], 2 * sizeof(float), cudaMemcpyHostToDevice));
		CUDA_CHECK(cudaDeviceSynchronize());
		minMaxKernel<<<blocks, threads, 2 * threads * src.getStride(), stream>>>(src.mData, mnMx.mData, src.getLength());
	}
}