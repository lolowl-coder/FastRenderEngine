#include "Renderer/Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace fre
{
    void Camera::setPerspectiveProjection(float fov, float aspectRatio, float near, float far)
    {
        mFov = fov;
        mNear = near;
        mFar = far;
        mProjection = glm::perspective(
			glm::radians(fov),
			aspectRatio,
			mNear, mFar);
		//In Vulkan Up direction points down
		mProjection[1][1] *= -1.0f;
    }

    void Camera::setOrthogonalProjection(float l, float r, float b, float t, float near, float far)
    {
        mNear = near;
        mFar = far;
        mProjection = glm::ortho(l, r, b, t, mNear, mFar);
    }

    void Camera::set(const glm::vec3 &eye, const glm::vec3 &look)
    {
        mEye = eye;
        mLook = look;
        mView = glm::lookAt(mEye, mLook, glm::vec3(0.0f, 1.0f, 0.0f));
    }
}