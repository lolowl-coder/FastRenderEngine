#pragma once

#include "Options.hpp"
#include <nvtx3/nvtx3.hpp>
#include <string>

//#define COLLECT_GPU_STAT
//#define COLLECT_CPU_STAT

#ifdef COLLECT_GPU_STAT
    #define STAT_GPU(n) StatGPU stat(n);
#else
    #define STAT_GPU(n)
#endif

#ifdef COLLECT_CPU_STAT
    #define STAT_CPU(n) StatCPU stat(n);
#else
    #define STAT_CPU(n)
#endif

namespace fre
{
    class StatGPU
    {
    public:
        StatGPU(const std::string& name)
        {
            OPTIONS;
            if(options.mEnableStat)
            {
                nvtxRangePushA(name.c_str());
            }
        }
        ~StatGPU()
        {
            OPTIONS;
            if(options.mEnableStat)
            {
                nvtxRangePop();
            }
        }
    };

    class StatCPU
    {
    public:
        StatCPU(const std::string& name)
        {
            nvtxRangePushA(name.c_str());
        }
        ~StatCPU()
        {
            nvtxRangePop();
        }
    };
}