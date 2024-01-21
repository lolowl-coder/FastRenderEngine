#pragma once

#include <glm/glm.hpp>

namespace fre
{
    struct Camera
    {
        void setPerspectiveProjection(float fov, float aspectRatio, float near, float far);
        void setOrthogonalProjection(float l, float r, float b, float t, float near, float far);
        void set(const glm::vec3& eye, const glm::vec3& look);
        
        float mFov = 45.0f;
		float mNear = 0.1f;
		float mFar = 100.0f;

        glm::vec3 mEye = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 mLook = glm::vec3(0.0f);

        glm::mat4 mProjection;
        glm::mat4 mView;
    };
}