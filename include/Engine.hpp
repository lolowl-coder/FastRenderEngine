#pragma once

#include "Camera.hpp"
#include "Light.hpp"

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
        std::shared_ptr<VulkanRenderer>& getRenderer();
        virtual void setupCamera(int width, int height);
        Camera& getCamera();
        float getCameraRotationSpeed() const;
        float getCameraZoomSpeed() const;
        Light& getLight();
    private:
        void positionWindow(const int width, const int height);
    protected:
        std::shared_ptr<VulkanRenderer> mRenderer;
        double mTime = 0.0;
        float mTimeDelta = 0.0f;

		Camera mCamera;
		float mCameraRotationSpeed = 0.3f;
		float mCameraZoomSpeed = 1.0f;

        Light mLight;
    };
}