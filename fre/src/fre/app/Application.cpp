#include "fre/app/Application.hpp"
#include "fre/core/FileSystem.hpp"
#include "fre/core/Log.hpp"
#include "fre/core/PlatformFactory.hpp"
#include "fre/core/Requirement.hpp"
#include "fre/core/VirtualFileSystem.hpp"
#include "fre/core/WindowManager.hpp"
#include "fre/engine/Engine.hpp"

namespace fre
{
	Application::Application()
	{
        FileSystemPtr mFS;
        VirtualFileSystemPtr mVFS;
        mFS = createFileSystem();
        mVFS = std::make_unique<VirtualFileSystem>(*mFS);
        Log::initialize(*mFS, true, true);

        const int width = 1920;
        const int height = 1080;
        WindowManagerPtr windowManager = fre::createWindowManager();
        mWindow = windowManager->createWindow({ width, height, "Renderer Test" });

		mEngine = std::make_unique<Engine>(mWindow.get());
	}

    Application::~Application() = default;

    void Application::handleInput()
    {
        
	}

    void Application::mainLoop()
    {
        while(!mWindow->shouldClose())
        {
			mWindow->pollEvents();
            mEngine->update();
            mEngine->render();
        }
    }
}