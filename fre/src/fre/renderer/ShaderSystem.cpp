#include "fre/renderer/IShader.hpp"
#include "fre/renderer/IGpuResourceFactory.hpp"
#include "fre/renderer/ShaderSystem.hpp"

namespace fre
{
	ShaderSystem::ShaderSystem()
	{
	}

	void ShaderSystem::createGpuResources()
	{
		for(auto& asset : mShaderAssets)
		{
			for(auto& [stage, blob] : asset.second.blobs)
			{
				asset.second.shader = mGpuResourceFactory->createGpuShader(asset.first, asset.second.blobs);
			}
		}
	}

	void ShaderSystem::registerBlob(const std::string& name, ShaderStage stage, ShaderStageBlob&& blob)
	{
		mShaderAssets[name].blobs[stage] = std::move(blob);
	}

	IShader* ShaderSystem::getShader(const std::string& name)
	{
		auto it = mShaderAssets.find(name);
		if(it != mShaderAssets.end())
		{
			if(it->second.shader != nullptr)
			{
				return it->second.shader.get();
			}
		}
		return nullptr;
	}
}