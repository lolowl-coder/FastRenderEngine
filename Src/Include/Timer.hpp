#pragma once

#include <GLFW/glfw3.h>

#include <mutex>

namespace fre
{
    class Timer {
    public:
        static Timer& getInstance()
        {
            static Timer instance;
            return instance;
        }

        Timer(const Timer&) = delete;
        Timer& operator=(const Timer&) = delete;

        //Time since application start, seconds
        double getTime()
        {
            return glfwGetTime();
        }

    private:
        Timer() {} // Private constructor to prevent instantiation
    };
}