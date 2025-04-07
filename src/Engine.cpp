#include "Engine.hpp"
#include "Renderer/VulkanRenderer.hpp"
#include "Timer.hpp"
#include "Log.hpp"

#include "imgui.h"

#include <filesystem>

using namespace glm;

namespace fre
{
    HWND hwnd;
    WNDPROC originalWndProc;

    Engine::Engine()
        : mThreadPool(std::thread::hardware_concurrency())
    {
		Log::initialize(true, true);
        LOG_DEBUG("Working directory: {}", std::filesystem::current_path().string());
    }

    void Engine::positionWindow(const int width, const int height)
    {
        int count;
        int monitorX, monitorY;
        GLFWmonitor** monitors = glfwGetMonitors(&count);
        const GLFWvidmode* videoMode = glfwGetVideoMode(monitors[0]);
        glfwGetMonitorPos(monitors[0], &monitorX, &monitorY);
        glfwSetWindowPos(mWindow,
                 monitorX + (videoMode->width - width) / 2,
                 monitorY + (videoMode->height - height) / 2);
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
    {
        Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
        engine->onViewSizeChanged(vec2(width, height));
        auto& renderer = engine->getRenderer();
        renderer->setFramebufferResized(true);
    }

    void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
    {
        Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
        engine->onButtonEvent(button, action, mods);
    }

    void mouseCallback(GLFWwindow* window, double x, double y)
    {
        static float lastX = static_cast<float>(x);
        static float lastY = static_cast<float>(y);

		float deltaX = static_cast<float>(x - lastX);
        float deltaY = static_cast<float>(y - lastY);

		//std::cout << deltaX << std::endl;

		lastX = static_cast<float>(x);
        lastY = static_cast<float>(y);
        
        Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
        engine->onTouchEvent(vec2(x,y), vec2(deltaX, deltaY));
    }

    void scrollCallback(GLFWwindow* window, double xOffset, double yOffset)
    {
        Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
        engine->onScrollEvent(xOffset, yOffset);
    }

    void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
        engine->onKeyEvent(key, action, mods);
    }

    bool Engine::createCoreGPUResources()
    {
        return mRenderer->createCoreGPUResources(mWindow) == 0;
    }

    bool Engine::createDynamicGPUResources()
    {
        return mRenderer->createDynamicGPUResources() == 0;
    }

    bool Engine::createMeshGPUResources()
    {
        return mRenderer->createMeshGPUResources() == 0;
    }

    bool Engine::createLoadableGPUResources()
    {
        return mRenderer->createLoadableGPUResources() == 0;
    }

    void Engine::onCustomMessage(uint32_t id)
    {
    }

    struct EnumWindowsData
    {
        HWND mCurrentHwnd;
        std::string mClassName;
        std::vector<HWND> mMatchingWindows;
    };

    // Callback function for window enumeration
    BOOL CALLBACK EnumWindowsProc(HWND wnd, LPARAM lParam)
    {
        EnumWindowsData* data = reinterpret_cast<EnumWindowsData*>(lParam);

        // Get the class name of the window
        char mClassName[256];
        GetClassName(wnd, mClassName, sizeof(mClassName));

        // Check if it matches the target class name and is not the current window
        if (data->mClassName == mClassName)
        {
            data->mMatchingWindows.push_back(wnd);
        }

        return TRUE; // Continue enumeration
    }

    void Engine::sendMessage(uint32_t id)
    {
        char className[256]; 
        GetClassName(hwnd, className, sizeof(className));
        EnumWindowsData data = { hwnd, className, {} };
        EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data));

        for(const auto& wnd : data.mMatchingWindows)
        {
            PostMessage(wnd, id, 0, 0);
        }
    }

    LRESULT CALLBACK CustomWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (uMsg == WM_CUSTOM_MESSAGE)
        {
            Engine* engine = reinterpret_cast<Engine*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
            engine->onCustomMessage(WM_CUSTOM_MESSAGE);

            return 0;  // Message handled
        }
        // Call the original window procedure for default processing
        return CallWindowProc(originalWndProc, hwnd, uMsg, wParam, lParam);
    }

	// Load the .ico file from resources
	HICON loadIconFromResource(int resourceID)
	{
		HMODULE hModule = GetModuleHandle(NULL);
		// Find the resource
		HRSRC hResource = FindResourceEx(hModule, RT_ICON, MAKEINTRESOURCE(resourceID), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
		if(!hResource)
		{
			LOG_ERROR("Failed to find the icon resource.");
			return NULL;
		}

		// Load the resource
		HGLOBAL hLoadedResource = LoadResource(hModule, hResource);
		if(!hLoadedResource)
		{
			LOG_ERROR("Failed to load the icon resource.");
			return NULL;
		}

		// Lock the resource to get a pointer to the icon data
		void* pResourceData = LockResource(hLoadedResource);
		DWORD resourceSize = SizeofResource(hModule, hResource);

		// Create an icon from the resource data
		HICON hIcon = CreateIconFromResourceEx(
			(PBYTE)pResourceData,
			resourceSize,
			TRUE,
			0x00030000, // Version of the icon format (Windows 3.x)
			0,
			0,
			LR_SHARED
		);

		if(!hIcon)
		{
			LOG_ERROR("Failed to create the icon from resource.");
		}

		return hIcon;
	}

	// Convert HICON to RGBA image that GLFW can use
	bool convertIconToGLFWImage(HICON hIcon, GLFWimage& image)
	{
		ICONINFO iconInfo;
		if(!GetIconInfo(hIcon, &iconInfo))
		{
			return false;
		}

		BITMAP bmpColor;
		GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bmpColor);

		image.width = bmpColor.bmWidth;
		image.height = bmpColor.bmHeight;
		image.pixels = new unsigned char[4 * image.width * image.height];

		HDC hdc = CreateCompatibleDC(NULL);
		HBITMAP hBitmapOld = (HBITMAP)SelectObject(hdc, iconInfo.hbmColor);

		BITMAPINFO bmi;
		ZeroMemory(&bmi, sizeof(BITMAPINFO));
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = image.width;
		bmi.bmiHeader.biHeight = -image.height; // top-down
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;

		GetDIBits(hdc, iconInfo.hbmColor, 0, image.height, image.pixels, &bmi, DIB_RGB_COLORS);

		// Cleanup
		SelectObject(hdc, hBitmapOld);
		DeleteDC(hdc);
		DeleteObject(iconInfo.hbmColor);
		DeleteObject(iconInfo.hbmMask);

		return true;
	}

    void Engine::setApplicationIcons(int firstIcon, int count)
    {
        std::vector<GLFWimage> images;
        for(int i = 0; i < count; i++)
        {
            auto icon = loadIconFromResource(firstIcon + i);
            GLFWimage img = {0, 0, nullptr};
            convertIconToGLFWImage(icon, img);
            if(img.pixels != nullptr)
            {
                images.push_back(img);
            }
        }

        if(!images.empty())
        {
            glfwSetWindowIcon(mWindow, images.size(), &images[0]);
        }

        for (int i = 0; i < images.size(); ++i)
        {
            delete [] images[i].pixels;
        }
    }

    bool Engine::create(std::string wName, const int width, const int height, int argc, char* argv[])
    {
        mArgC = argc;
        mArgV = argv;
        mMaxViewport.mMin = vec2(0.0f);
        mMaxViewport.mMax = vec2(width, height);

        //Initialize GLFW
        glfwInit();

        //Set FGLW to not work with OpenGL
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        mWindow = glfwCreateWindow(width, height, wName.c_str(), nullptr, nullptr);
        glfwSetWindowUserPointer(mWindow, this);
        glfwSetFramebufferSizeCallback(mWindow, framebufferResizeCallback);

        positionWindow(width, height);

        //glfwSetCursorPos(mWindow, width / 2, height / 2);
        glfwSetCursorPosCallback(mWindow, mouseCallback);
        glfwSetMouseButtonCallback(mWindow, mouseButtonCallback);
        glfwSetScrollCallback(mWindow, scrollCallback);
        glfwSetKeyCallback(mWindow, keyCallback);

        hwnd = glfwGetWin32Window(mWindow);
        originalWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)CustomWndProc);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

        onViewSizeChanged(vec2(width, height));

        bool result = false;
        try
        {
            result = mWindow != nullptr && mRenderer != nullptr
                && createCoreGPUResources()
                && createDynamicGPUResources()
                && createMeshGPUResources()
                && createLoadableGPUResources()
                && postCreate();
        }
        catch (std::runtime_error& e)
		{
			LOG_ERROR(e.what());
			destroy();
		}

        return result;
    }

    bool Engine::postCreate()
    {
        bool result = false;
        if(mRenderer != nullptr)
        {
            result = mRenderer->postCreate() == 0;
        }

        LOG_INFO("Post-create stage completed");

        return result;
    }

    void Engine::update()
    {
        if(isKeyPressed(GLFW_MOUSE_BUTTON_LEFT))
        {
            mRenderer->requestRedraw();
        }
        mRenderer->update(mCamera, mLight);
    }

    void Engine::draw()
    {
        mRenderer->draw(mCamera, mLight);
    }

    void Engine::run()
    {
        //Loop until closed
        try
        {
            while (!glfwWindowShouldClose(mWindow) && !mExit)
            {
                glfwPollEvents();
                bool needRedraw = mRenderer->needRedraw();
                auto t0 = std::chrono::high_resolution_clock::now();
            
                tick();

                if(mRenderer != nullptr)
                {
                    update();
                    mRenderer->preprocessUI();
                    draw();
                }

                onFrameEnd(mRenderer.get());
                auto desiredFrameTime = 1000.0f / mDesiredFPS;
                if(needRedraw)
                {
                    auto t1 = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<double> duration = t1 - t0;
                    mFrameTime = duration.count() * 1000;
                    static auto lastUpdateTime = t1;
                    std::chrono::duration<double> timeSinceLastUpdate = t1 - lastUpdateTime;
                    auto waitTime = max(0.0f, desiredFrameTime - mFrameTime);
                    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<uint32>(waitTime)));
                    if(timeSinceLastUpdate.count() * 1000 > 500)
                    {
                        lastUpdateTime = t1;
                    }
                    mFPS = 1000.0f / (mFrameTime + waitTime);
                }
                else
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<uint32>(desiredFrameTime)));
                }
            }
        }
        catch (std::runtime_error& e)
		{
			LOG_ERROR(e.what());
		}
    }

    void Engine::tick()
    {
        static double lastTime = 0.0f;
        mTime = Timer::getInstance().getTime();
        mTimeDelta = static_cast<float>(mTime - lastTime);
        lastTime = mTime;
        
        mCamera.update(mTimeDelta);
    }

    void Engine::destroyGPUResources()
    {
        if(mRenderer != nullptr)
        {
            mRenderer->destroyGPUResources();
        }
    }

    void Engine::destroy()
    {
        glfwDestroyWindow(mWindow);
        if(mRenderer != nullptr)
        {
            mRenderer->destroy();
        }
		mThreadPool.destroy();
        destroyGPUResources();
        //Destroy window
        glfwTerminate();
        Log::shutdown();
    }

    std::shared_ptr<VulkanRenderer>& Engine::getRenderer()
    {
        return mRenderer;
    }

    void Engine::setupCamera(const BoundingBox2D& viewport)
	{
        auto sz = viewport.getSize();
        if(sz.x != 0 && sz.y != 0)
		{
            float aspectRatio = sz.x / sz.y;
		    mCamera.setPerspectiveProjection(45.0f, aspectRatio);
        }
	}

    Camera& Engine::getCamera()
    {
        return mCamera;
    }

    float Engine::getCameraRotationSpeed() const
    {
        return mCameraRotationSpeed;
    }
    float Engine::getCameraZoomSpeed() const
    {
        return mCameraZoomSpeed;
    }

    Light& Engine::getLight()
    {
        return mLight;
    }

    void Engine::setCameraMovement(Camera::EMovement movement, Camera& camera, bool movementEnabled)
    {
        ImGuiIO& io = ImGui::GetIO();
        if(!io.WantCaptureMouse && isKeyPressed(GLFW_MOUSE_BUTTON_LEFT))
        {
            camera.setMovement(movement, movementEnabled);
        }
    }

    void Engine::onKeyEvent(int key, int action, int mods)
    {
        mButtonStates[key] = action == GLFW_PRESS || action == GLFW_REPEAT;
        /*if (action == GLFW_PRESS || action == GLFW_RELEASE)
        {
            bool movementEnabled = action == GLFW_PRESS;
            switch (key) {
                case GLFW_KEY_W:
                    setCameraMovement(Camera::M_FORWARD, mCamera, movementEnabled);
                    break;
                case GLFW_KEY_S:
                    setCameraMovement(Camera::M_BACKWARD, mCamera, movementEnabled);
                    break;
                case GLFW_KEY_A:
                    setCameraMovement(Camera::M_LEFT, mCamera, movementEnabled);
                    break;
                case GLFW_KEY_D:
                    setCameraMovement(Camera::M_RIGHT, mCamera, movementEnabled);
                    break;
                case GLFW_KEY_Q:
                    setCameraMovement(Camera::M_DOWN, mCamera, movementEnabled);
                    break;
                case GLFW_KEY_E:
                    setCameraMovement(Camera::M_UP, mCamera, movementEnabled);
                    break;
                case GLFW_KEY_L:
                    mLight.mPosition = -mCamera.getEye();

                    std::cout << "Light position: " << mLight.mPosition.x << " "
                        << mLight.mPosition.y << " " << mLight.mPosition.z << std::endl;

                    break;
            }
        }*/
        mRenderer->requestRedraw();
    }

    void Engine::updateCameraRotation(const vec2& delta)
    {
        mCamera.rotateBy(vec3(
            getCameraRotationSpeed() * delta.y,
            0.0f,
            getCameraRotationSpeed() * delta.x));
    }

    void Engine::onTouchEvent(const vec2& position, const vec2& delta)
    {
        ImGuiIO& io = ImGui::GetIO();
        if(!io.WantCaptureMouse)
        {
            if(isKeyPressed(GLFW_MOUSE_BUTTON_LEFT) && !io.WantCaptureMouse)
            {
                updateCameraRotation(delta);
            }
            else if(isKeyPressed(GLFW_MOUSE_BUTTON_MIDDLE) && !io.WantCaptureMouse)
            {
                mCamera.translateBy(glm::vec3(delta.x * mCameraPanSpeed, -delta.y * mCameraPanSpeed, 0.0f));
            }
        }
        mTouchPosition = position;
        mRenderer->requestRedraw();
    }

    void Engine::onButtonEvent(int button, int action, int mods)
    {
        mButtonStates[button] = action == GLFW_PRESS || action == GLFW_REPEAT;

        ImGuiIO& io = ImGui::GetIO();
        if(!io.WantCaptureMouse)
        {
            if(button == GLFW_MOUSE_BUTTON_LEFT)
            {
                if(action == GLFW_RELEASE)
                {
                    for(uint8_t i = 0; i < static_cast<int>(Camera::EMovement::M_COUNT); i++)
                    {
                        mCamera.setMovement(static_cast<Camera::EMovement>(i), false);
                    }
                }
            }
        }

        mRenderer->requestRedraw();
    }

    void Engine::onScrollEvent(float xOffset, float yOffset)
    {
        ImGuiIO& io = ImGui::GetIO();
        if(!io.WantCaptureMouse)
        {
            //const vec3 forward = mCamera.getForward();
		    //mCamera.translateBy(engine->getCameraZoomSpeed() * forward * static_cast<float>(yOffset));
            //mCamera.translateBy(vec3(0.0f, 0.0f, engine->getCameraZoomSpeed() * static_cast<float>(yOffset)));
            mCamera.mZoom = std::max(0.05f, mCamera.mZoom + getCameraZoomSpeed() * static_cast<float>(yOffset));
        }

        mRenderer->requestRedraw();
    }

    void Engine::onViewSizeChanged(const vec2& newSize)
    {
        mMaxViewport.mMin = vec2(0.0f);
        mMaxViewport.mMax = newSize;
        setupCamera(mMaxViewport);

        mRenderer->requestRedraw();
    }

    bool Engine::isKeyPressed(int key)
    {
        const auto& found = mButtonStates.find(key);
        return found != mButtonStates.end() && found->second == true;
    }

    void* Engine::getWindowHandle()
    {
        return &hwnd;
    }
}