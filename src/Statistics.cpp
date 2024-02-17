#include "Statistics.hpp"

#include <iostream>

namespace fre
{
    void Statistics::startMeasure(const std::string& name, float t)
    {
        mMetrics[name].mStartTime = t;
    }

    void Statistics::stopMeasure(const std::string& name, float t)
    {
        mMetrics[name].mEndTime = t;
    }

    void Statistics::print()
    {
        for(const auto& metric : mMetrics)
        {
            std::cout << "[STAT]" << metric.first << " " <<
                metric.second.mEndTime - metric.second.mStartTime;
        }
    }
}