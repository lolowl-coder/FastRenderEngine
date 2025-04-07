#pragma once

#include "imgui.h"
#include "Utilities.hpp"

#include <glm/glm.hpp>

#include <iostream>

#define OPTIONS auto& options = fre::Options::getInstance()

namespace fre
{
    class Options
    {
    public:
        Options();
        // Static member function to access the singleton instance
        static Options& getInstance();

        struct Common
        {
            float mDiffuseIntensity = 1.0f;
            float mSpecularIntensity = 0.25f;
        } mCommon;

        bool mEnableStat = true;
		
        void save();
        void load();

        void loadDefaults()
        {
            *this = Options();
        }
    };
}