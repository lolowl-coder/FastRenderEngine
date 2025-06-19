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

        mCamera.setEye(vec3(0, 0, -100.0f));

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

        /*auto bgColor = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
        mRenderer->setClearColor(vec4(bgColor.x, bgColor.y, bgColor.z, 0.0f));*/
        mRenderer->setClearColor(vec4(0.0f, 0.0f, 0.0f, 0.0f));
        
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