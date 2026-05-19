#pragma once

#include "fre/core/Pointers.hpp"
#include "fre/renderer/ShaderAsset.hpp"
#include "fre/renderer/ShaderStage.hpp"
#include "fre/renderer/ShaderStageBlob.hpp"

namespace fre
{
	class ShaderSystem
	{
	public:
		ShaderSystem();
		void setResourceFactory(IGpuResourceFactory* resourceFactory) { mGpuResourceFactory = resourceFactory; }
		void registerBlob(const std::string& name, ShaderStage stage, ShaderStageBlob&& blob);
		void createGpuResources();
		IShader* getShader(const std::string& name);
	private:
		IGpuResourceFactory* mGpuResourceFactory = nullptr;
		std::unordered_map<std::string, ShaderAsset> mShaderAssets;
	};
}