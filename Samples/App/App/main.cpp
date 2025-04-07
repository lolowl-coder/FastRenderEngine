#include "AppEngine.hpp"

using namespace app;

int main(int argc, char* argv[])
{
    AppEngine engine;
    if(engine.create("App", 1800, 900, argc, argv))
    {
        engine.run();
        engine.destroy();
    }
    return 0;
}