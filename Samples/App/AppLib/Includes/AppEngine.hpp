#pragma once

#include "Renderer/VulkanShader.hpp"
#include "Renderer/VulkanBufferManager.hpp"
#include "Renderer/VulkanAccelerationStructure.hpp"
#include "Engine.hpp"
#include "Utilities.hpp"
#include "Options.hpp"
#include "Pointers.hpp"

#include <filesystem>

namespace app
{
    class AppEngine : public fre::Engine, fre::ShaderMetaDataProvider
    {
    public:
        //Creates all features like height map, grid, dissection
        AppEngine();
        virtual ~AppEngine() override;

        void initUI();

        virtual bool createDynamicGPUResources() override;

        virtual void setupCamera(const fre::BoundingBox2D& viewport);

        virtual bool createCoreGPUResources() override;
        virtual bool createMeshGPUResources() override;
        virtual bool createLoadableGPUResources() override;

        virtual bool postCreate() override;

        virtual void destroyGPUResources() override;

        //Provides custom shader meta data for features
        virtual fre::ShaderMetaDatum getShaderMetaData(const std::string& shaderFileName) override;

        virtual void update() override;

        virtual void draw() override;

        virtual void onFrameEnd(fre::VulkanRenderer* renderer);

        virtual void onButtonEvent(int button, int action, int mods);

        virtual void updateCameraRotation(const glm::vec2& delta);

        virtual void onTouchEvent(const glm::vec2& position, const glm::vec2& delta);

        virtual void onKeyEvent(int key, int action, int mods);
    
        virtual void onCustomMessage(uint32_t id);

        void updateCameraNearFar();

        void updateCameraProjection(const fre::BoundingBox2D& viewport);

        void setCameraProjection(bool ortho, const fre::BoundingBox2D& viewport);

        void toggleCameraProjection();

    private:
        void loadFonts();
        int getMainMenuHeight() const;
        fre::BoundingBox2D getMainViewport();
        fre::BoundingBox2D getCurrentViewport();
        void createAS();
        void createScene();

    private:
        //Render camera
        bool mIsCameraOrthogonal = true;
        bool mLastIsCameraOrthogonal = true;
        fre::Camera mLastCamera;
		uint64_t mFrameNumber = 0;
        int mMainMenuHeight = 24;

        fre::VulkanBuffer mVertexBuffer;
        fre::VulkanBuffer mIndexBuffer;
        fre::VulkanBuffer mTransformMatrixBuffer;
        fre::AccelerationStructure mBLAS;
        fre::AccelerationStructure mTLAS;
    };
}