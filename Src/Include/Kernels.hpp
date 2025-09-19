#pragma once

#include <cuda_runtime_api.h>
#include <cuda_fp16.h>
#include <cufft.h>

#if !defined(_NVHPC_CUDA) && !defined(__CUDACC__)
	#error "Ensure this header is compiled by CUDA compiler and NOT usual c/c++ one.";
#endif

namespace fre
{
	__device__ const __half HALF_NEG_ONE = __half_raw{0xBC00};  // -1.0
	__device__ const __half HALF_ZERO    = __half_raw{0x0000};  // 0.0
	__device__ const __half HALF_ONE     = __half_raw{0x3C00};  // 1.0

	__device__ __forceinline__ cufftComplex convertDoubleComplexToComplex(cufftDoubleComplex input) {
		cufftComplex output;

		// Real part conversion with range handling
		if (isnan(input.x)) {
			output.x = NAN;
		} else if (isinf(input.x)) {
			output.x = copysign(FLT_MAX, input.x);
		} else if (input.x > FLT_MAX) {
			output.x = FLT_MAX;
		} else if (input.x < -FLT_MAX) {
			output.x = -FLT_MAX;
		} else if (fabs(input.x) < FLT_MIN) {
			output.x = 0.0f;  // Can handle subnormal as 0 if desired
		} else {
			output.x = static_cast<float>(input.x);
		}

		// Imaginary part conversion with range handling
		if (isnan(input.y)) {
			output.y = NAN;
		} else if (isinf(input.y)) {
			output.y = copysign(FLT_MAX, input.y);
		} else if (input.y > FLT_MAX) {
			output.y = FLT_MAX;
		} else if (input.y < -FLT_MAX) {
			output.y = -FLT_MAX;
		} else if (fabs(input.y) < FLT_MIN) {
			output.y = 0.0f;  // Handle subnormal as 0 if desired
		} else {
			output.y = static_cast<float>(input.y);
		}

		return output;
	}


	__device__ __forceinline__ cufftDoubleComplex dhmMakeComplex(double r, double i)
	{
		return make_cuDoubleComplex(r, i);
	}

	__device__ __forceinline__ cufftComplex dhmMakeComplex(float r, float i)
	{
		return make_cuComplex(r, i);
	}

	__device__ __forceinline__ cufftComplex dhmComplexConj(const cufftComplex& c)
	{
		return cuConjf(c);
	}

	__device__ __forceinline__ cufftDoubleComplex dhmComplexConj(const cufftDoubleComplex& c)
	{
		return cuConj(c);
	}

    template<typename T>
    __device__ T dhmAtan2(T x, T y)
	{
		return T(atan2(T(x), T(y)));
	}

    /*__device__ __forceinline__ __half dhmSin(__half x)
	{
		return hsin(x);
	}*/

    /*__device__ __forceinline__ __half dhmCos(__half x)
	{
		return hcos(x);
	}*/

    __device__ __forceinline__ float dhmSin(float x)
	{
		return sin(x);
	}

    __device__ __forceinline__ float dhmCos(float x)
	{
		return cos(x);
	}

	__device__ __host__ __forceinline__ double dhmCAbs(const cufftDoubleComplex& z)
	{
		return cuCabs(z);
		//return sqrt(z.x * z.x + z.y * z.y);
	}

	__device__ __host__ __forceinline__ float dhmCAbs(const cufftComplex& z)
	{
		return cuCabsf(z);
		//return sqrtf(z.x * z.x + z.y * z.y);
	}

	__device__ __forceinline__ float dhmAbs(float x)
	{
		return abs(x);
	}
	
	__device__ __forceinline__ __half dhmAbs(__half x)
	{
		return __habs(x);
	}
	
	__device__ __forceinline__ float dhmPow(float x, float y)
	{
		return pow(x, y);
	}
	
	__device__ __forceinline__ __half dhmPow(float x, __half y)
	{
		return __float2half(pow(x, __half2float(y)));
	}
	
	__device__ __forceinline__ float dhmMax(float x, float y)
	{
		return max(x, y);
	}
	
	__device__ __forceinline__ __half dhmMax(__half x, __half y)
	{
		return __hmax(x, y);
	}

	__device__ __forceinline__ __half dhmSign(__half h)
	{
		unsigned short h_raw = *reinterpret_cast<unsigned short*>(&h);
		// Extract the sign bit (16th bit)
		unsigned short sign_bit = h_raw & 0x8000;

		// If the sign bit is set, return -1, otherwise return 1 or 0

		// Special case for zero
		if (h_raw == 0) return HALF_ZERO;
		return sign_bit ? HALF_NEG_ONE : HALF_ONE;
	}

	__device__ __forceinline__ float dhmSign(float f)
	{
		return signbit(f);
	}

	void diff(float* dst, float* src, int width, int height, int actualWidth, int blocks, int threads, cudaStream_t stream);

	void applyRFLeftToRight(float* dst, float* src, float* derivatives, int width, int height, int actualWidth, float feedbackFactor, int blocks, int threads, cudaStream_t stream);
	void applyRFRightToLeft(float* dst, float* src, float* derivatives, int width, int height, int actualWidth, float feedbackFactor, int blocks, int threads, cudaStream_t stream);

	template<typename TDst, typename TSrc>
	__global__ void copyImageKernel(
		TDst* __restrict__ dst, const TSrc* __restrict__ src,
		const int dstWidth, const int srcWidth,
		const int dstRoiOffsetX, const int dstRoiOffsetY,
		const int srcRoiOffsetX, const int srcRoiOffsetY,
		const int roiWidth, const int roiHeight)
	{
		const int tid = blockIdx.x * blockDim.x + threadIdx.x;
		const int stride = blockDim.x * gridDim.x;
		const int size = roiWidth * roiHeight;
		for (int i = tid; i < size; i += stride)
		{
			int y = i / roiWidth;
			int x = i % roiWidth;
			if(x < roiWidth && y < roiHeight)
			{
				dst[(dstRoiOffsetY + y) * dstWidth + dstRoiOffsetX + x] = src[(srcRoiOffsetY + y) * srcWidth + srcRoiOffsetX + x];
			}
		}
	}
	
	template<typename TDst, typename TSrc, typename TVec>
	__global__ void copyImageVectorized8Kernel(
		TDst* __restrict__ dst, const TSrc* __restrict__ src,
		const int dstWidth, const int srcWidth,
		const int dstRoiOffsetX, const int dstRoiOffsetY,
		const int srcRoiOffsetX, const int srcRoiOffsetY,
		const int roiWidth, const int roiHeight,
		const int size)
	{
		const int tid = (blockIdx.x * blockDim.x + threadIdx.x) * 8;
		const int stride = blockDim.x * gridDim.x;
		for (int i = tid; i < size; i += stride)
		{
			const int y = i / roiWidth;
			const int x = i % roiWidth;
			if(x < roiWidth && y < roiHeight)
			{
				TDst* dstLocal = &dst[(dstRoiOffsetY + y) * dstWidth + dstRoiOffsetX + x];
				const TSrc* srcLocal = &src[(srcRoiOffsetY + y) * srcWidth + srcRoiOffsetX + x];
				TVec* dstV = (TVec*)(dstLocal);
				const TVec* srcV = (const TVec*)(srcLocal);
				*dstV = *srcV;
			}
		}
	}

	template<typename TDst, typename TSrc>
	__global__ void copyImageSameSizeKernel(TDst* __restrict__ dst, const TSrc* __restrict__ src, const int size)
	{
		int tid = blockIdx.x * blockDim.x + threadIdx.x;
		const int stride = blockDim.x * gridDim.x;
		for (int i = tid; i < size; i += stride)
		{
			dst[i] = src[i];
		}
	}

	template<typename TDst, typename TSrc>
	void copyImage(CudaBuffer<TDst>& dst, const CudaBuffer<TSrc>& src, const glm::ivec2& roi, cudaStream_t stream)
	{
		if(dst.getDataType() == src.getDataType() && dst.mElementsCount == src.mElementsCount && glm::ivec2(dst.mElementsCount) == roi)
		{
			CUDA_CHECK(cudaMemcpyAsync(dst.mData, src.mData, src.getSize(), cudaMemcpyHostToHost, stream));
		}
		else
		{
			KM;
			if(dst.mElementsCount == src.mElementsCount && glm::ivec2(dst.mElementsCount) == roi)
			{
				km.launchKernel(copyImageSameSizeKernel<TDst, TSrc>, stream, roi.x * roi.y, dst.mData, src.mData, roi.x * roi.y);
			}
			else
			{
				/*if(src.mElementsCount.x % 8 == 0 && dst.mElementsCount.x % 8 == 0 && sizeof(TSrc) == 2 && sizeof(TDst) == 2 && dst.getDataType() == src.getDataType())
				{
					//km.launchKernel(copyImageVectorized8Kernel<TDst, TSrc, int4>, stream, (roi.x / 8) * roi.y,
					//	dst.mData, src.mData, dst.mElementsCount.x, src.mElementsCount.x, 0, 0, 0, 0, roi.x, roi.y, roi.x * roi.y);
					const int threads = 1024;
					int blocks = (roi.x * roi.y / 8 + threads - 1) / threads;
					copyImageVectorized8Kernel<TDst, TSrc, int4><<<blocks, threads, 0, stream>>>(dst.mData, src.mData, dst.mElementsCount.x,
						src.mElementsCount.x, 0, 0, 0, 0, roi.x, roi.y, roi.x * roi.y);
				}
				else if(src.mElementsCount.x % 8 == 0 && dst.mElementsCount.x % 8 == 0 && sizeof(TSrc) == 4 && sizeof(TDst) == 4 && dst.getDataType() == src.getDataType())
				{
					km.launchKernel(copyImageVectorized8Kernel<TDst, TSrc, int4>, stream, (roi.x / 8) * roi.y,
						dst.mData, src.mData, dst.mElementsCount.x, src.mElementsCount.x, 0, 0, 0, 0, roi.x, roi.y, roi.x * roi.y);
				}
				else*/
				{
					km.launchKernel(copyImageKernel<TDst, TSrc>, stream, roi.x * roi.y, dst.mData, src.mData,
						dst.mElementsCount.x, src.mElementsCount.x, 0, 0, 0, 0, roi.x, roi.y);
					//const int threads = 1024;
					//int blocks = (roi.x * roi.y + threads - 1) / threads;
					//copyImageKernel<<<blocks, threads, 0, stream>>>(dst.mData, src.mData, dst.mElementsCount.x, src.mElementsCount.x, 0, 0, 0, 0, roi.x, roi.y);
				}
			}
		}
	}

	__global__ void toFloatComplex(cufftComplex* __restrict__ dst, const cufftDoubleComplex* __restrict__ src, const int size);

	__global__ void toDoubleComplex(cufftDoubleComplex* __restrict__ dst, const cufftComplex* __restrict__ src, const int size);

	template<typename TDst, typename TSrc>
	void copyImage(CudaBuffer<TDst>& dst, const CudaBuffer<TSrc>& src, const glm::ivec2& dstRoiOffset, const glm::ivec2& srcRoiOffset, const glm::ivec2& roi, cudaStream_t stream)
	{
		LAUNCH_KERNEL
		(
			roi.x * roi.y,
			stream,
			(copyImageKernel<TDst,TSrc>),
			dst.mData, src.mData,
			dst.mElementsCount.x, src.mElementsCount.x,
			dstRoiOffset.x, dstRoiOffset.y,
			srcRoiOffset.x, srcRoiOffset.y,
			roi.x, roi.y
		);
	}
	
	template<typename TDst, typename Ta, typename Tb>
	__global__ void compareKernel(
		TDst* __restrict__ dst, const Ta* __restrict__ a, const Tb* __restrict__ b,
		const int dstWidth, const int aWidth, const int bWidth,
		const int dstRoiOffsetX, const int dstRoiOffsetY,
		const int aRoiOffsetX, const int aRoiOffsetY,
		const int bRoiOffsetX, const int bRoiOffsetY,
		const int roiWidth, const int roiHeight)
	{
		const int tid = blockIdx.x * blockDim.x + threadIdx.x;
		const int stride = blockDim.x * gridDim.x;
		const int size = roiWidth * roiHeight;
		for (int i = tid; i < size; i += stride)
		{
			int y = i / roiWidth;
			int x = i % roiWidth;
			if(x < roiWidth && y < roiHeight)
			{
				dst[(dstRoiOffsetY + y) * dstWidth + dstRoiOffsetX + x] = abs((double)a[(aRoiOffsetY + y) * aWidth + aRoiOffsetX + x] - (double)b[(bRoiOffsetY + y) * bWidth + bRoiOffsetX + x]);
			}
		}
	}

	template<typename TDst, typename Ta, typename Tb>
	void compareImages(CudaBuffer<TDst>& dst, const CudaBuffer<Ta>& a, const CudaBuffer<Tb>& b, const glm::ivec2& dstRoiOffset, const glm::ivec2& aRoiOffset, const glm::ivec2& bRoiOffset, const glm::ivec2& roi, const bool logOutput, cudaStream_t stream)
	{
		KM;
		km.launchKernel(
			compareKernel<TDst, Ta, Tb>, stream, roi.x * roi.y,
			dst.mData, a.mData, b.mData,
			dst.mElementsCount.x, a.mElementsCount.x, b.mElementsCount.x,
			dstRoiOffset.x, dstRoiOffset.y,
			aRoiOffset.x, aRoiOffset.y,
			bRoiOffset.x, bRoiOffset.y,
			roi.x, roi.y);

		if(logOutput)
		{
			CUDA_CHECK(cudaDeviceSynchronize());
			//Get values to CPU side;
			Ta* aH = new Ta[a.getLength()];
			Tb* bH = new Tb[b.getLength()];
			a.getData(aH);
			b.getData(bH);
			CUDA_CHECK(cudaDeviceSynchronize());
			double sum = 0.0;
			double maxDeviation = -DBL_MAX;
			const int size = roi.x * roi.y;
			for(int i = 0; i < size; i++)
			{
				const int ay = i / a.mElementsCount.x;
				const int ax = i % a.mElementsCount.x;
				const int by = i / b.mElementsCount.x;
				const int bx = i % b.mElementsCount.x;
				const double diff = aH[(ay + aRoiOffset.y) * a.mElementsCount.x + ax + aRoiOffset.x] - bH[(by + bRoiOffset.y) * b.mElementsCount.x + bx + bRoiOffset.x];
				sum += diff * diff;
				const double absDeviation = abs(diff);
				maxDeviation = max(maxDeviation, absDeviation);
			}
			LOG_WARNING("Abs max deviation = {}", maxDeviation);

			const double mse = sum / size;
			LOG_WARNING("MSE = {}", mse);
			const double psnr = 10.0 * log10((65535.0 * 65535.0) / mse);
			LOG_WARNING("PSNR = {}", psnr);

			delete aH;
			delete bH;
		}
	}

	template <typename TDst, typename TSrc>
	void transposeImage(TDst* dst, const TSrc* src, int dstWidth, int dstHeight, int srcWidth, int srcHeight, int roiWidth, int roiHeight, cudaStream_t stream);

	//void fillBorder(float *dst, float* src, int actualWidth, int height, int marginStart, int marginWidth, int blocks, int threads, cudaStream_t stream);

	void filterFlip(float *dst, float th, float mn, float mx, int width, int height, int actualWidth, cudaStream_t stream);

	template<typename T>
	__device__ __forceinline__ float getValue(T* src, int x, int y, int width, int height)
	{
		int addr = min(height - 1, max(0, y)) * width + min(width - 1, max(0, x));
		T value = src[addr];

		return value;
	}

	__host__ __device__ int getPatternCount();

	void filterExplicitNoise(float* src, int width, float th, int blocks, int threads, cudaStream_t stream);

	void f32Tof16(__half* dst, const float* src, const int dstWidth, const int srcWidth, const int roiWidth, const int roiHeight, cudaStream_t stream);

	void f16Tof32(float* dst, const __half* src, const int dstWidth, const int srcWidth, const int roiWidth, const int roiHeight, cudaStream_t stream);

	void f16Tou16(uint16_t* dst, const __half* src, const int dstWidth, const int srcWidth, const int roiWidth, const int roiHeight, cudaStream_t stream);

	void f32Tou16(uint16_t* dst, const float* src, const int dstWidth, const int srcWidth, const int roiWidth, const int roiHeight, cudaStream_t stream);

	template<typename T, typename C>
	__global__ void toComplex(const T* __restrict__ input, C* __restrict__ output, int inputWidth, int outputWidth, int mnX, int mnY, int width, int height)
	{
		const int x = blockIdx.x * blockDim.x + threadIdx.x;
		const int y = blockIdx.y * blockDim.y + threadIdx.y;

		if(x < width && y < height)
		{
			const int srcX = mnX + x;
			const int srcY = mnY + y;

			C& result = output[y * outputWidth + x];
			result.x = input[srcY * inputWidth + srcX];
			result.y = 0.0;
		}
	}

	template<typename C>
	__global__ void complexMulRealKernel(const C* __restrict__ cSrc, C* __restrict__ cDst, const double* __restrict__ r, const int size)
	{
		const int tid = blockIdx.x * blockDim.x + threadIdx.x;
		const int stride = blockDim.x * gridDim.x;
		for (int i = tid; i < size; i += stride)
		{
			const C& src = cSrc[i];
			C& dst = cDst[i];
			const double& rValue = r[i];
			//dst.x = __dmul_rn(src.x, rValue);
			//dst.y = __dmul_rn(src.y, rValue);
			dst.x = src.x * rValue;
			dst.y = src.y * rValue;
		}
	}

	template<typename C>
	void __forceinline__ complexMulReal(const CudaBuffer<C>& cSrc, CudaBuffer<C>& cDst, const CudaBuffer<double>& r, cudaStream_t stream)
	{
		KM;
		const auto& config = km.getKernelConfig(complexMulRealKernel<C>, cSrc.getLength());
		complexMulRealKernel<<<config.first, config.second, 0, stream>>>(cSrc.mData, cDst.mData, r.mData, cSrc.getLength());
	}

	template<typename R, typename C>
	__global__ void complexMagnitude(R* __restrict__ dst, const C* __restrict__ src, const int size)
	{
		const int tid = blockIdx.x * blockDim.x + threadIdx.x;
		if (tid < size)
		{
			//cufftDoubleComplex tmp = make_cuDoubleComplex(src[tid].x, src[tid].y);
			//dst[tid] = log(dhmCAbs(tmp));
			dst[tid] = log(dhmCAbs(src[tid]));
		}
	}

	template<typename C, typename R>
	__global__ void complexScale(C* __restrict__ dst, R scale, const int size)
	{
		const int tid = blockIdx.x * blockDim.x + threadIdx.x;
		if (tid < size)
		{
			C& dstLocal = dst[tid];
			dstLocal.x *= scale;
			dstLocal.y *= scale;
		}
	}

	template<typename TSrc, typename TDst>
	__global__ void circshiftKernel(const TSrc* __restrict__ input, TDst* __restrict__ output, int rows, int cols, int shiftRow, int shiftCol)
	{
		int row = blockIdx.y * blockDim.y + threadIdx.y;
		int col = blockIdx.x * blockDim.x + threadIdx.x;

		if (row < rows && col < cols) {
			// Calculate the new positions after the circular shift
			int newRow = (row + shiftRow + rows) % rows;
			int newCol = (col + shiftCol + cols) % cols;

			// Assign the value from the input to the new position in the output
			output[newRow * cols + newCol] = input[row * cols + col];
		}
	}

	template<typename TSrc, typename TDst>
	void circShift(const CudaBuffer<TSrc>& input, CudaBuffer<TDst>& output, int rows, int cols, cudaStream_t stream) {
		dim3 blockSize(32, 32); // Define block size
		dim3 gridSize((input.mElementsCount.x + blockSize.x - 1) / blockSize.x, (input.mElementsCount.y + blockSize.y - 1) / blockSize.y);

		// Launch the kernel
		circshiftKernel<<<gridSize, blockSize, 0, stream>>>(input.mData, output.mData, input.mElementsCount.y, input.mElementsCount.x, rows, cols);
	}

	template<typename T>
	__global__ void fftShift(const T* __restrict__ input, T* __restrict__ output, int rows, int cols)
	{
		int row = blockIdx.y * blockDim.y + threadIdx.y;
		int col = blockIdx.x * blockDim.x + threadIdx.x;

		// Check if within bounds
		if (row < rows && col < cols) {
			int new_row = (row + (rows / 2)) % rows; // Shift the row index
			int new_col = (col + (cols / 2)) % cols; // Shift the column index

			// Calculate the corresponding index in the output array
			output[new_row * cols + new_col] = input[row * cols + col];
		}
	}

	template<typename T>
	__global__ void ifftShift(const T* __restrict__ input, T* __restrict__ output, const int rows, const int cols, const int rowsMiddle, const int colsMiddle)
	{
		int row = blockIdx.y * blockDim.y + threadIdx.y;
		int col = blockIdx.x * blockDim.x + threadIdx.x;

		// Check if within bounds
		if (row < rows && col < cols) {
			int new_row = (row + rowsMiddle) % rows; // Shift the row index
			int new_col = (col + colsMiddle) % cols; // Shift the column index

			// Calculate the corresponding index in the output array
			output[new_row * cols + new_col] = input[row * cols + col];
		}
	}

	// Function to compute the complex exponential
	__device__ __forceinline__ cufftComplex complexExp(cufftComplex z)
	{
		float exp_real = expf(z.x);  // Exponential of the real part
		float cos_imag;
		float sin_imag;
		sincosf(z.y, &sin_imag, &cos_imag);
		return { __fmul_rn(exp_real, cos_imag), __fmul_rn(exp_real, sin_imag) };
	}

	// Function to compute the complex exponential
	__device__ __forceinline__ cufftDoubleComplex complexExp(cufftDoubleComplex z)
	{
		double exp_real = exp(z.x);  // Exponential of the real part.x
		double cos_imag;
		double sin_imag;
		sincos(z.y, &sin_imag, &cos_imag);
		return { __dmul_rn(exp_real, cos_imag), __dmul_rn(exp_real, sin_imag) };
	}

	template<typename R, typename C>
	__global__ void complexAbsKernel(R* __restrict__ output, const C* __restrict__ input, const int size)
	{
		const int tid = blockIdx.x * blockDim.x + threadIdx.x;
		if (tid < size)
		{
			const C& inputLocal = input[tid];
			const cufftComplex inputLocalReduced = { (float)inputLocal.x, (float)inputLocal.y };
			output[tid] = dhmCAbs(inputLocalReduced);
		}
	}

	template<typename T>
	void minMax(const CudaBuffer<T>& src, CudaBuffer<float>& mnMx, cudaStream_t stream);

	template<typename TSrc, typename TMnMx>
    __global__ void normalizeKernel(TSrc* __restrict__ src, const TMnMx* __restrict__ mnMx, const int size)
    {
        const TSrc mn = mnMx[0];
        const TSrc mx = mnMx[1];
        const TSrc diff = mx - mn;
        int tid = blockIdx.x * blockDim.x + threadIdx.x;
		if(tid < size)
		{
			src[tid] = (src[tid] - mn) / diff;
		}
    }

    template<typename T>
    __global__ void unnormalize(uint16_t* dst, T* src, const uint16_t* __restrict__ mnMx, const int size)
    {
        const T mn = mnMx[0];
        const T mx = mnMx[1];
        const T diff = mx - mn;

	    int tid = blockIdx.x * blockDim.x + threadIdx.x;
		dst[tid] = src[tid] * diff + mn;
		//src[tid] = src[tid] * 65535.0f;
    }

	template<typename T>
    __global__ void rescaleKernel(T* __restrict__ src, const float* __restrict__ mnMx, const float scale, const int size)
    {
        const float mn = mnMx[0];
        const float mx = mnMx[1];
        const float diff = mx - mn;
        int tid = blockIdx.x * blockDim.x + threadIdx.x;
		if(tid < size)
		{
			src[tid] = ((float)src[tid] - mn) / diff * scale;
		}
    }

	template<typename T>
    __global__ void rescaleKernelPI(T* __restrict__ src, const T scale, const int size)
    {
        const T mn = -PI;
        const T mx = PI;
        const T diff = mx - mn;
        int tid = blockIdx.x * blockDim.x + threadIdx.x;
		if(tid < size)
		{
			T& srcLocal = src[tid];
			srcLocal = (srcLocal - mn) / diff * scale;
		}
    }

	template<typename C>
    __global__ void complexConjKernel(const C* __restrict__ dst, const C* __restrict__ src, const int size)
    {
		const int tid = blockIdx.x * blockDim.x + threadIdx.x;
		const int stride = blockDim.x * gridDim.x;
		for (int i = tid; i < size; i += stride)
		{
			dst[tid] = dhmComplexConj(src[tid]);
		}
    }
}