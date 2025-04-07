#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <volk.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <windows.h>

#include "Camera.hpp"
#include "Light.hpp"
#include "ThreadPool.hpp"
#include "Utilities.hpp"

#include <cstdint>
#include <string>
#include <map>
#include <memory>

namespace fre
{
    class VulkanRenderer;

    //Angstrom Rendering Engine
    class Engine 
    {
    public:
        Engine();
        virtual ~Engine(){}
        virtual bool createCoreGPUResources();
        virtual bool createDynamicGPUResources();
        virtual bool createMeshGPUResources();
        virtual bool createLoadableGPUResources();
        void setApplicationIcons(int firstIcon, int count);
        //Creates window
        virtual bool create(std::string wName, const int width, const int height, int argc, char* argv[]);
        virtual bool postCreate();
        virtual void update();
        virtual void draw();
        //Enters main loop
        virtual void run();
        //Called every frame before render
        virtual void tick();
        //Called after frame is rendered
        virtual void onFrameEnd(VulkanRenderer* renderer) {};
        //Destroy GPU resource
        virtual void destroyGPUResources();
        //Destroys engine
        virtual void destroy();
        //Returns renderer
        std::shared_ptr<VulkanRenderer>& getRenderer();
        //Sets the camera
        virtual void setupCamera(const BoundingBox2D& viewport);
        //Returns the camera
        Camera& getCamera();
        //Returns camera rotation speed
        float getCameraRotationSpeed() const;
        //Returns camera zoom speed
        float getCameraZoomSpeed() const;
        //Returns light
        Light& getLight();
        //Key event
        virtual void onKeyEvent(int key, int action, int mods);
        virtual void updateCameraRotation(const glm::vec2& delta);
        virtual void onTouchEvent(const glm::vec2& position, const glm::vec2& delta);
        virtual void onButtonEvent(int button, int action, int mods);
        virtual void onScrollEvent(float xOffset, float yOffset);
        virtual void onViewSizeChanged(const glm::vec2& newSize);
        virtual void onCustomMessage(uint32_t id);
        static void sendMessage(uint32_t id);

        bool isKeyPressed(int key);

        void exit() { mExit = true; }

        static void* getWindowHandle();
    private:
        //Sets window position
        void positionWindow(const int width, const int height);
        void setCameraMovement(Camera::EMovement movement, Camera& camera, bool movementEnabled);
    protected:
        ThreadPool mThreadPool;
        GLFWwindow* mWindow;
        std::shared_ptr<VulkanRenderer> mRenderer;
        double mTime = 0.0;
        float mTimeDelta = 0.0f;

		Camera mCamera;
		float mCameraRotationSpeed = 0.3f;
		float mCameraZoomSpeed = 1.0f;
        float mCameraPanSpeed = 1.0f;

        Light mLight;
        glm::vec2 mTouchPosition = glm::vec2(0.0f);
        fre::BoundingBox2D mMaxViewport;
        std::map<int, bool> mButtonStates;
        bool mExit = false;
        //Frame time, ms
        float mFrameTime = 0.0f;
        float mDesiredFPS = 60.0f;
        float mFPS = 0.0f;
        int mArgC = 0;
        char** mArgV = nullptr;
    };
}