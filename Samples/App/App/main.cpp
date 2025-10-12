#include "AppEngine.hpp"

using namespace app;

int main(int argc, char* argv[])
{
    AppEngine engine;
    if(engine.create("App", 1920, 1080, argc, argv))
    {
        engine.run();
        engine.destroy();
    }
    return 0;
}