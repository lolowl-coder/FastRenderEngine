#include "FileSystem/FileSystem.hpp"
#include "Renderer/AppRenderer.hpp"
#include "Renderer/VulkanRenderer.hpp"
#include "Renderer/VulkanBufferManager.hpp"
#include "Macros/Member.hpp"
#include "AppData.hpp"
#include "AppEngine.hpp"
#include "Options.hpp"
#include "Stat.hpp"
#include "ThreadPool.hpp"

#include "imgui.h"
#include "implot.h"
#include "implot_internal.h"

#include "resource.h"

#include <filesystem>

#include <tchar.h>

using namespace fre;
using namespace glm;

namespace app
{
    //Creates all features like height map, grid, dissection
    AppEngine::AppEngine()
        : Engine()
    {
        FS;
        //Add search path
        fs.addPath("Fonts");
        fs.addPath("Models/coordSystem");
        fs.addPath("Models/unitCube");
        fs.addPath("Models/unitQuad");
        fs.addPath("Shaders");
        fs.addPath("Textures");

        mRenderer.reset(new AppRenderer(mThreadPool));

        mRenderer->setShaderMetaDataProvider(this);

        initUI();

        OPTIONS;
        options.load();
    }

    AppEngine::~AppEngine()
    {
    }

    void AppEngine::initUI()
    {
        mRenderer->addUIRenderCallback
        (
            [this]()
            {
                ImGuiStyle& style = ImGui::GetStyle();
                style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0, 0, 0, 0);
                style.Colors[ImGuiCol_MenuBarBg] = style.Colors[ImGuiCol_WindowBg];

                OPTIONS;

                //Main menu
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                if(ImGui::BeginMainMenuBar())
                {
                    mMainMenuHeight = ImGui::GetWindowHeight();
                    if(ImGui::BeginMenu("File"))
                    {
                        if(ImGui::MenuItem("Exit"))
                        {
                            exit();
                        }
                        ImGui::EndMenu();
                    }
                    ImGui::EndMainMenuBar();
                }
                ImGui::PopStyleVar(1);
            }
        );
    }

    void AppEngine::createAS()
    {
        // Setup vertices and indices for a single triangle
        struct Vertex
        {
            float pos[3];
        };
        std::vector<Vertex> vertices = {
            {{1.0f, 1.0f, 0.0f}},
            {{-1.0f, 1.0f, 0.0f}},
            {{0.0f, -1.0f, 0.0f}} };
        std::vector<uint32_t> indices = { 0, 1, 2 };

        auto vertex_buffer_size = vertices.size() * sizeof(Vertex);
        auto index_buffer_size = indices.size() * sizeof(uint32_t);
        VkTransformMatrixKHR transform_matrix = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f };

        // Create buffers for the bottom level geometry
        // For the sake of simplicity we won't stage the vertex data to the GPU memory

        // Note that the buffer usage flags for buffers consumed by the bottom level acceleration structure require special flags
        const VkBufferUsageFlags buffer_usage_flags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

        mVertexBuffer = mRenderer->createBuffer(buffer_usage_flags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertices.data(), vertex_buffer_size);
        mIndexBuffer = mRenderer->createBuffer(buffer_usage_flags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indices.data(), index_buffer_size);

        // Setup a single transformation matrix that can be used to transform the whole geometry for a single bottom level acceleration structure
        mTransformMatrixBuffer = mRenderer->createBuffer(
            buffer_usage_flags,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &transform_matrix, sizeof(transform_matrix));

        mBLAS = mRenderer->createBLAS(mVertexBuffer, mIndexBuffer, mTransformMatrixBuffer);
        mTLAS = mRenderer->createTLAS(mBLAS.mDeviceAddress, transform_matrix);
    }

    void AppEngine::createScene()
    {
        createAS();
    }

	bool AppEngine::createCoreGPUResources()
	{
		bool result = Engine::createCoreGPUResources();

		return result;
	}

    bool AppEngine::createDynamicGPUResources()
    {
        setApplicationIcons(IDI_APP_ICON, 8);
        bool result = Engine::createDynamicGPUResources();
        return result;
    }

    void AppEngine::setupCamera(const BoundingBox2D& viewport)
    {
        updateCameraNearFar();
        updateCameraProjection(getCurrentViewport());
    }

    bool AppEngine::createMeshGPUResources()
    {
		bool result = Engine::createMeshGPUResources();
        return result;
    }

    bool AppEngine::createLoadableGPUResources()
    {   
        bool result = Engine::createLoadableGPUResources();

        FS;
        //Keep it in memory, because ImGuiIO::IniFilename is a pointer
        static std::string iniFileName(fs.getDocumentsDir() + "/imgui.ini");
        ImGui::GetIO().IniFilename = iniFileName.c_str();
        loadFonts();

        return result;
    }

    bool AppEngine::postCreate()
    {
        return Engine::postCreate();
    }

    void AppEngine::destroyGPUResources()
    {
        //Order matters since ApplicationData depends on values calculated in Device::stopOperating()
        {
            APP_DATA;
            appData.save();
        }
        Engine::destroyGPUResources();
    }

    ShaderMetaDatum AppEngine::getShaderMetaData(const std::string& shaderFileName)
    {
        ShaderMetaDatum result;

        return result;
    }

    BoundingBox2D AppEngine::getMainViewport()
    {
        auto mainMenuHeight = getMainMenuHeight();
	    
        return BoundingBox2D(
            vec2(0, mainMenuHeight),
            vec2(mMaxViewport.mMax.x, mMaxViewport.mMax.y));
    }
    
    BoundingBox2D AppEngine::getCurrentViewport()
    {
		return getMainViewport();
    }

    void AppEngine::update()
    {
        STAT_CPU("Main thread");

        auto bgColor = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
        mRenderer->setClearColor(vec4(bgColor.x, bgColor.y, bgColor.z, 0.0f));
        
        OPTIONS;
        mLight.mDiffuseColor = vec3(options.mCommon.mDiffuseIntensity);
        mLight.mSpecularColor = vec3(options.mCommon.mSpecularIntensity);
        Engine::update();
        
        {
            APP_DATA;
            appData.mAppTimeSec += mTimeDelta;
        }
    }

    void AppEngine::draw()
    {
        Engine::draw();

        mFrameNumber++;
    }

    void AppEngine::onFrameEnd(VulkanRenderer* renderer)
    {
        Engine::onFrameEnd(renderer);
    }

    void AppEngine::onButtonEvent(int button, int action, int mods)
    {
        Engine::onButtonEvent(button, action, mods);

        ImGuiIO& io = ImGui::GetIO();
        if(!io.WantCaptureMouse)
        {
        }
    }

    void AppEngine::updateCameraRotation(const vec2& delta)
    {
        Engine::updateCameraRotation(delta);
    }

    void AppEngine::onTouchEvent(const vec2& position, const vec2& delta)
    {
        Engine::onTouchEvent(position, delta);
    }

    void AppEngine::onKeyEvent(int key, int action, int mods)
    {
        Engine::onKeyEvent(key, action, mods);
    }

    void AppEngine::onCustomMessage(uint32_t id)
    {
        if(id == WM_CUSTOM_MESSAGE)
        {
            LOG_TRACE("Custom message received");
        }
    }

    void AppEngine::updateCameraNearFar()
    {
        if(mIsCameraOrthogonal)
        {
            mCamera.mNear = -40000.0f;
            mCamera.mFar = 20000.0f;
        }
        else
        {
            mCamera.mNear = 0.1f;
            mCamera.mFar = 10000.0f;
        }
        mCamera.updateViewMatrix();
    }

    void AppEngine::updateCameraProjection(const BoundingBox2D& viewport)
    {
        if(mIsCameraOrthogonal)
        {
            auto size = viewport.getSize();
            auto l = -size.x * 0.5f;
            auto r = size.x * 0.5f;
            auto b = -size.y * 0.5f;
            auto t = size.y * 0.5f;
		    mCamera.setOrthogonalProjection(l, r, b, t);
        }
        else
        {
            //For perspective camera use Parent method
            Engine::setupCamera(viewport);
        }
    }

    void AppEngine::setCameraProjection(bool ortho, const BoundingBox2D& viewport)
    {
        mIsCameraOrthogonal = ortho;
        updateCameraNearFar();
        updateCameraProjection(viewport);
    }

    void AppEngine::toggleCameraProjection()
    {
        setCameraProjection(!mIsCameraOrthogonal, getCurrentViewport());
    }

    int AppEngine::getMainMenuHeight() const
    {
        return mMainMenuHeight;
    }

    void AppEngine::loadFonts()
    {
	    ImGuiIO& io = ImGui::GetIO();
        
        //Font 0
        {
            //Main UI font
            io.Fonts->AddFontFromFileTTF("Fonts/Roboto-Regular.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesGreek());
        }
    }
}