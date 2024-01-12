#pragma once

#include <cstdint>
#include <string>
#include <memory>

namespace fre
{
    class VulkanRenderer;

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
        virtual ~Engine(){}
        virtual bool create(std::string wName, const int width, const int height);
        virtual void run();
        virtual void tick();
        virtual void destroy();
    private:
        void positionWindow(const int width, const int height);
    protected:
        std::shared_ptr<VulkanRenderer> mRenderer;
    };
}