#include "fre/core/FileSystem.hpp"
#include "fre/core/Log.hpp"
#include "fre/core/PlatformFactory.hpp"
#include "fre/core/Requirement.hpp"
#include "fre/core/VirtualFileSystem.hpp"
#include "fre/core/WindowManager.hpp"
#include "fre/renderer/CommonRendererConfig.hpp"
#include "fre/renderer/IGraphicsContext.hpp"
#include "fre/renderer/IRenderer.hpp"
#include "fre/renderer/RendererFactory.hpp"

int main()
{
    using namespace fre;

    FileSystemPtr mFS;
    VirtualFileSystemPtr mVFS;
    mFS = createFileSystem();
    mVFS = std::make_unique<VirtualFileSystem>(*mFS);
    Log::initialize(*mFS, true, true);

    const int width = 1920;
    const int height = 1080;
    WindowManagerPtr windowManager = fre::createWindowManager();
    auto window = windowManager->createWindow({ width, height, "Renderer Test" });

    CommonRendererConfig commonConfig;
    commonConfig.mEnableValidation = true;
    commonConfig.mHeadless = false;
    commonConfig.mGPUSelectionMode = GPUSelectionMode::Auto;
    commonConfig.mWidth = width;
    commonConfig.mHeight = height;
    commonConfig.mBackend = Backend::Vulkan;

    auto context = createGraphicsContext(commonConfig.mBackend, window.get(), commonConfig.mEnableValidation);

    auto surface = context->createSurface(window.get());

    commonConfig.mContext = context.get();
	commonConfig.mSurface = surface.get();

    auto renderer = createRenderer(commonConfig);

    renderer->beginFrame();
    renderer->endFrame();

    return 0;
}