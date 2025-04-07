#pragma once

#include "imgui.h"
#include "Utilities.hpp"

#include <iostream>

#define APP_DATA auto& appData = AppData::getInstance()

namespace app
{
    class AppData
    {
    public:
        // Static member function to access the singleton instance
        static AppData& getInstance();

        uint32_t mAppTimeSec = 0;

        void save();
        void load();

        void loadDefaults()
        {
            *this = AppData();
        }
    };
}