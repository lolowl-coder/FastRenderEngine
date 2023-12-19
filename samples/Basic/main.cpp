#include "Engine.hpp"

int main()
{
    fre::Engine engine;
    if(engine.init("Test Vulkan", 800, 600))
    {
        engine.run();
        engine.destroy();
    }
    return 0;
}