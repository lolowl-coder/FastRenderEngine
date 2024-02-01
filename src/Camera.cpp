#include "Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

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
		mProjection[1][1] *= -1.0f;
    }

    void Camera::rotateBy(const glm::vec3& rotationAngles)
    {
        mRotationAngles += rotationAngles;
    }

    void Camera::translateBy(const glm::vec3& translation)
    {
        mEye += translation;
    }

    void Camera::setMovement(EMovement movement, bool value)
    {
        mMovement[movement] = value;
    }

    void Camera::update(double time, float timeDelta)
    {
        updateMovement(time, timeDelta);
        updateViewMatrix();
        updateVectors();
    }

    void Camera::updateMovement(double time, float timeDelta)
    {
        //std::cout << "movement: " << mMovement[0] << " " << mMovement[1] << " " << mMovement[2] << " " << mMovement[3] << std::endl;
        if(mMovement[M_FORWARD])
        {
            translateBy(mForward * mMovementSpeed * timeDelta);
        }
        else if(mMovement[M_BACKWARD])
        {
            translateBy(-mForward * mMovementSpeed * timeDelta);
        }
        
        if(mMovement[M_RIGHT])
        {
            translateBy(mRight * mMovementSpeed * timeDelta);
        }
        else if(mMovement[M_LEFT])
        {
            translateBy(-mRight * mMovementSpeed * timeDelta);
        }
    }

    void Camera::updateViewMatrix()
    {
		glm::mat4 rotation = glm::mat4(1.0f);

		rotation = glm::rotate(rotation, glm::radians(mRotationAngles.x), glm::vec3(1.0f, 0.0f, 0.0f));
		rotation = glm::rotate(rotation, glm::radians(mRotationAngles.y), glm::vec3(0.0f, 1.0f, 0.0f));
        rotation = glm::rotate(rotation, glm::radians(mRotationAngles.z), glm::vec3(0.0f, 0.0f, 1.0f));

		glm::mat4 translation;
		translation = glm::translate(glm::mat4(1.0f), mEye);

		mView = rotation * translation;
    }

    void Camera::updateVectors()
    {
        mForward.x = -cos(glm::radians(mRotationAngles.x)) * sin(glm::radians(mRotationAngles.y));
        mForward.y = sin(glm::radians(mRotationAngles.x));
        mForward.z = cos(glm::radians(mRotationAngles.x)) * cos(glm::radians(mRotationAngles.y));
        mForward = glm::normalize(mForward);

        mRight = glm::normalize(glm::cross(mForward, glm::vec3(0.0f, 1.0f, 0.0f)));
        mUp = glm::normalize(glm::cross(mRight, mForward));

        //std::cout << "forward: " << mForward.x << " " << mForward.y << " " << mForward.z << std::endl;
    }

    const glm::vec3& Camera::getForward() const
    {
        return mForward;
    }

    const glm::vec3& Camera::getRight() const
    {
        return mRight;
    }

    const glm::vec3& Camera::getUp() const
    {
        return mUp;
    }

    const glm::vec3& Camera::getEye() const
    {
        return mEye;
    }

    void Camera::setEye(const glm::vec3& eye)
    {
        mEye = eye;
    }
}