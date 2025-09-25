#pragma once

#include "CudaUtilities.hpp"
#include "Utilities.hpp"

#include <stdexcept>
#include <glm/glm.hpp>
#include <nppdefs.h>
#include <cuda_fp16.h>
#include <cufft.h>

#define CUDA_BUFFER_1D(n,t,x) auto n = bufferManager.lock<t>(#n, x);
#define CUDA_BUFFER_2D(n,t,x,y) auto n = 
#define CUDA_BUFFER_2D_CREATE(n,t,x,y) n = bufferManager.lock<t>(#n, x, y);
#define CUDA_BUFFER_3D(n,t,x,y,z) auto n = bufferManager.lock<t>(#n, x, y, z);
#define CUDA_BUFFER_SAME(n,t,same) auto n = bufferManager.lock<t>(#n, same.mDimensions);
#define CUDA_PRINT_BUFFER(b,c) LOG_INFO("Buffer {}", #b); printCudaBuffer(b.mData, b.mDimensions, c);
#define CUDA_DUMP_BUFFER_R_TIFF(b, n) bufferManager.saveToTiffReal(formatString("textures/%s.tiff", #b), n, b, 0);
#define CUDA_DUMP_BUFFER_R_CSV(b) bufferManager.saveToCSVReal(formatString("textures/%s.csv", #b), b, 0);
#define CUDA_DUMP_BUFFER_C_TIFF(b,re,im) bufferManager.saveToTiffComplex(formatString("textures/%s.tiff", #b), b, re, im, 0);
#define CUDA_DUMP_BUFFER_C_CSV(b) bufferManager.saveToCSVComplex(formatString("textures/%sRe.csv", #b), formatString("textures/%sIm.csv", #b), b, 0);

namespace fre
{
	struct ExternalBufferData;

	enum class EDataType
	{
		Uint16,
		Float,
		Double,
		Complex,
		DoubleComplex,
		Npp8u,
		Half,
		Float4,
		Count
	};

	inline EDataType getDataTypeImpl(uint16_t* value) { return EDataType::Uint16; }
	inline EDataType getDataTypeImpl(float* value) { return EDataType::Float; }
	inline EDataType getDataTypeImpl(double* value) { return EDataType::Double; }
	inline EDataType getDataTypeImpl(cufftComplex* value) { return EDataType::Complex; }
	inline EDataType getDataTypeImpl(cufftDoubleComplex* value) { return EDataType::DoubleComplex; }
	inline EDataType getDataTypeImpl(Npp8u* value) { return EDataType::Npp8u; }
	inline EDataType getDataTypeImpl(__half* value) { return EDataType::Half; }
	inline EDataType getDataTypeImpl(float4* value) { return EDataType::Float4; }

	inline void printBufferElement(const uint16_t& value)
	{
		printf("%i ", value);
	}
	
	inline void printBufferElement(const float& value)
	{
		printf("%f ", value);
	}

	inline void printBufferElement(const double& value)
	{
		printf("%f ", value);
	}

	inline void printBufferElement(const cufftComplex& value)
	{
		printf("%f : %f ", value.x, value.y);
	}

	inline void printBufferElement(const cufftDoubleComplex& value)
	{
		printf("%f : %f ", value.x, value.y);
	}
	
	inline void printBufferElement(const __half& value)
	{
		printf("%f ", __half2float(value));
	}

	inline void printBufferElement(const float4& value)
	{
		printf("%f, %f, %f, %f ", value.x, value.y, value.z, value.w);
	}
	
	template<typename T>
	void printCudaBuffer(const T* d_data, const glm::u64vec3& dim, uint64_t count)
	{
		T* h_data = new T[count];

		//Copy rows
		for(int i = 0; i < min(dim.y, count); i++)
		{
			CUDA_CHECK(cudaMemcpy(h_data, &d_data[i * dim.x], count * sizeof(T), cudaMemcpyDeviceToHost));
			printf("row %i\n", i);
			for(uint64_t j = 0u; j < min(dim.x, count); j++)
			{
				printBufferElement(h_data[j]);
			}
			printf("\n");
		}

		delete [] h_data;
	}

	//Typed representation of buffer for convenience
	template<typename T>
	struct CudaBuffer
	{
		//EDataType mDataType = EDataType::Count;
		glm::u64vec3 mDimensions = glm::u64vec3(0);
		//uint64_t mSize;
		T* mData = nullptr;

		//CudaBuffer(const u64vec3& elementsCount)
		//	: mData(data)
		//	: mDimensions(elementsCount)
		//	//, mDataType = getDataType(mData);
		//	//, mSize = sizeof(T) * mDimensions;
		//{
		//}

		EDataType getDataType() const { return getDataTypeImpl(mData); }

		int getStride() const
		{
			EDataType dataType = getDataTypeImpl(mData);
			switch(dataType)
			{
			case EDataType::Uint16:
				return sizeof(uint16_t);
			case EDataType::Float:
				return sizeof(float);
			case EDataType::Double:
				return sizeof(double);
			case EDataType::Complex:
				return sizeof(cufftComplex);
			case EDataType::DoubleComplex:
				return sizeof(cufftDoubleComplex);
			case EDataType::Npp8u:
				return sizeof(Npp8u);
			case EDataType::Half:
				return sizeof(__half);
			case EDataType::Float4:
				return sizeof(float4);
			default:
				throw std::runtime_error(fre::formatString("Wrong CUDA buffer data type: %i!", static_cast<int>(dataType)));
			}

			return 0;
		}

		int getRowSize() const
		{
			return mDimensions.x * getStride();
		}

		constexpr int getSize() const
		{
			return getStride() * mDimensions.x * mDimensions.y * mDimensions.z;
		}

		constexpr int getLength() const
		{
			return mDimensions.x * mDimensions.y * mDimensions.z;
		}

		void clear(uint8_t value, cudaStream_t stream) { CUDA_CHECK(cudaMemsetAsync(mData, value, getSize(), stream)); }

		void uploadData(void* data) { CUDA_CHECK(cudaMemcpy(mData, data, mDimensions.x * mDimensions.y * mDimensions.z * sizeof(T), cudaMemcpyHostToDevice)); }
		void getData(void* data) const { CUDA_CHECK(cudaMemcpy(data, mData, mDimensions.x * mDimensions.y * mDimensions.z * sizeof(T), cudaMemcpyDeviceToHost)); }
	};
}