#pragma once

#include <cstdint>
#include <string>

namespace fre
{
    //Concept:
    //Applicaton side:
    //create window/view and engine instance
    //call following instance methods:
    //  Engine::init() after window/view is accessible
    //  Engine::tick() in main loop to render onto surface
    //  Engine::destroy() on exit
    class Engine
    {
    public:
        bool init(std::string wName, const int width, const int height);
        void run();
        void tick();
        void destroy();
    };
}