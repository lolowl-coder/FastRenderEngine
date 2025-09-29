#pragma once

#include <optix.h>

#include <cuda_runtime.h>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iomanip>
#include <vector>

namespace fre
{
    class Denoiser
    {
    public:
        struct Data
        {
            uint32_t  width = 0;
            uint32_t  height = 0;
            float* color = nullptr;
            float* albedo = nullptr;
            float* normal = nullptr;
            float* flow = nullptr;
            float* flowtrust = nullptr;
            std::vector< float* > aovs;     // input AOVs
            std::vector< float* > outputs;  // denoised beauty, followed by denoised AOVs
        };

        // Initialize the API and push all data to the GPU -- normaly done only once per session.
        // tileWidth, tileHeight: If nonzero, enable tiling with given dimension.
        // kpMode: If enabled, use kernel prediction model even if no AOVs are given.
        // temporalMode: If enabled, use a model for denoising sequences of images.
        // applyFlowMode: Apply flow vectors from current frame to previous image (no denoising).
        void init(const Data& data,
            unsigned int tileWidth = 0,
            unsigned int tileHeight = 0,
            bool         kpMode = false,
            bool         temporalMode = false,
            bool         applyFlowMode = false,
            bool         upscale2xMode = false,
            OptixDenoiserAlphaMode alphaMode = OPTIX_DENOISER_ALPHA_MODE_COPY,
            bool         specularMode = false);

        // Execute the denoiser. In interactive sessions, this would be done once per frame/subframe.
        void exec();

        // Update denoiser input data on GPU from host memory.
        void update(const Data& data);

        // Copy results from GPU to host memory.
        void getResults();

        // Copy results from device buffer to another device buffer
        void copyResultDevice(void* data);

        // Return internal guide layer data for temporal models, if available. Returned memory must be freed.
        void getInternalGuideLayerData(unsigned char** data, size_t* sizeInBytes);

        // Cleanup state, deallocate memory -- normally done only once per render session.
        void finish();

    private:
        // --- Test flow vectors: Flow is applied to noisy input image and written back to result.
        // --- No denoising.
        void applyFlow();

    private:
        OptixDeviceContext    m_context = nullptr;
        OptixDenoiser         m_denoiser = nullptr;
        OptixDenoiserParams   m_params = {};

        bool                  m_temporalMode;
        bool                  m_applyFlowMode;

        CUdeviceptr           m_intensity = 0;
        CUdeviceptr           m_avgColor = 0;
        CUdeviceptr           m_scratch = 0;
        uint32_t              m_scratch_size = 0;
        CUdeviceptr           m_state = 0;
        uint32_t              m_state_size = 0;

        unsigned int          m_tileWidth = 0;
        unsigned int          m_tileHeight = 0;
        unsigned int          m_overlap = 0;

        OptixDenoiserGuideLayer           m_guideLayer = {};
        std::vector< OptixDenoiserLayer > m_layers;
        std::vector< float* >             m_host_outputs;
    };
}