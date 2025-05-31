#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include "Light.hpp"
#include "Pointers.hpp"

#include <glm/glm.hpp>
#include <functional>

namespace fre
{
	struct Camera;
	struct Material;

	using PushConstantCallback = std::function<void(const MeshPtr& mesh, const glm::mat4& modelMatrix,
		const Camera& camera, const Light& light, VkPipelineLayout pipelineLayout, uint32_t instanceId)>;

	using BindDescriptorSetsCallback = std::function<void(const MeshPtr& mesh,
		const Material& material, VkPipelineLayout pipelineLayout, uint32_t instanceId)>;
}