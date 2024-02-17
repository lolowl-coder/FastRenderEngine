#pragma once

#include <map>
#include <string>

namespace fre
{
    struct Metrics
    {
        float mStartTime;
        float mEndTime;
    };

    struct Statistics
    {
        void startMeasure(const std::string& name, float t);
        void stopMeasure(const std::string& name, float t);
        void print();

        std::map<std::string, Metrics> mMetrics;
    };
}