#pragma once

#include <glm/glm.hpp>

namespace fre
{
	//Lighting data for shader
	struct Lighting
	{
		glm::vec4 cameraEye = glm::vec4(0.0f);
		glm::vec4 lightPos = glm::vec4(0.0f);
		glm::vec4 lightDiffuseColor = glm::vec4(0.0f);
		glm::vec4 lightSpecularColor = glm::vec4(0.0f);
		glm::mat4 normalMatrix = glm::mat4(1.0f);
	};

	//Light
    struct Light
    {
        glm::vec3 mPosition = glm::vec3(100.0f);
        glm::vec3 mDiffuseColor = glm::vec3(1.0f);
        glm::vec3 mSpecularColor = glm::vec3(1.0f);
    };
}