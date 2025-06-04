#include "Renderer/ShaderInputParser.hpp"
#include "Renderer/VulkanDescriptorSetLayout.hpp"
#include "Log.hpp"

#include "spirv_reflect.h"

namespace fre
{
#define CHECK(c)\
	{\
		SpvReflectResult result = c;\
		if(result != SPV_REFLECT_RESULT_SUCCESS)\
		{\
			LOG_ERROR("SPIRV-Reflect error {}, {}", result, #c);\
		}\
	}

	void getLayouts(
		const SpvReflectShaderModule& module,
		std::vector<VulkanDescriptorSetLayoutInfo>& layoutInfos)
	{
		uint32_t count = 0;
		CHECK(spvReflectEnumerateDescriptorSets(&module, &count, NULL));
		std::vector<SpvReflectDescriptorSet*> sets(count);
		CHECK(spvReflectEnumerateDescriptorSets(&module, &count, sets.data()));

		for(const auto s : sets)
		{
			const SpvReflectDescriptorSet& reflSet = *s;
            if(reflSet.set >= layoutInfos.size())
            {
				layoutInfos.resize(reflSet.set + 1);
            }
			
			for(uint32_t j = 0; j < reflSet.binding_count; ++j)
			{
				const SpvReflectDescriptorBinding& reflBinding = *(reflSet.bindings[j]);

				if(reflBinding.binding >= layoutInfos[reflSet.set].mDescriptorTypes.size())
				{
					layoutInfos[reflSet.set].mDescriptorTypes.resize(reflBinding.binding + 1);
					layoutInfos[reflSet.set].mBindings.resize(reflBinding.binding + 1);
					layoutInfos[reflSet.set].mDescriptorCount.resize(reflBinding.binding + 1);
					layoutInfos[reflSet.set].mStageFlags.resize(reflBinding.binding + 1);
				}

				layoutInfos[reflSet.set].mBindings[reflBinding.binding] = reflBinding.binding;
				layoutInfos[reflSet.set].mDescriptorTypes[reflBinding.binding] = static_cast<VkDescriptorType>(reflBinding.descriptor_type);
				layoutInfos[reflSet.set].mDescriptorCount[reflBinding.binding] = 1;
				layoutInfos[reflSet.set].mStageFlags[reflBinding.binding] = module.shader_stage;
				for(uint32_t k = 0; k < reflBinding.array.dims_count; ++k)
				{
					layoutInfos[reflSet.set].mDescriptorCount[reflBinding.binding] *= reflBinding.array.dims[k];
				}
			}
		}
	}

	void ShaderInputParser::parseShaderInput(
		const std::vector<char>& source,
		std::vector<VulkanDescriptorSetLayoutInfo>& layouts)
	{
		if(!source.empty())
		{
			SpvReflectShaderModule module = {};
			CHECK(spvReflectCreateShaderModule(source.size(), source.data(), &module));

			uint32_t count = 0;

			getLayouts(module, layouts);

			CHECK(spvReflectEnumeratePushConstantBlocks(&module, &count, NULL));
			std::vector<SpvReflectBlockVariable*> push_constant(count);
			CHECK(spvReflectEnumeratePushConstantBlocks(&module, &count, push_constant.data()));

			for(auto* pc : push_constant)
			{
				LOG_INFO("Push constant {}: offset {}, size {}", pc->name, pc->offset, pc->size);
			}

			spvReflectDestroyShaderModule(&module);
		}
	}
}