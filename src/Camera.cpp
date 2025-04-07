#include "Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

using namespace glm;

namespace fre
{
    void Camera::setPerspectiveProjection(float fov, float aspectRatio)
    {
        mFov = fov;
        mProjection = perspective(
			radians(fov),
			aspectRatio,
			mNear, mFar);
		//In Vulkan Up direction points down
        mProjection[1][1] *= -1.0f;
    }

    void Camera::setOrthogonalProjection(float l, float r, float b, float t)
    {
        mProjection = ortho(l, r, b, t, mNear, mFar);
		mProjection[1][1] *= -1.0f;
    }

    void Camera::rotateBy(const vec3& rotationAngles)
    {
        mRotationAngles += rotationAngles;
        mRotationAngles.x = std::min(mMaxPitch, std::max(mMinPitch, mRotationAngles.x));
    }

    void Camera::translateBy(const vec3& translation)
    {
        setEye(mEye + translation);
    }

    void Camera::setMovement(EMovement movement, bool value)
    {
        mMovement[static_cast<int>(movement)] = value;
    }

    void Camera::update(float timeDelta)
    {
        updateMovement(timeDelta);
        updateViewMatrix();
        updateVectors();
    }

    void Camera::updateMovement(float timeDelta)
    {
        //std::cout << "movement: " << mMovement[0] << " " << mMovement[1] << " " << mMovement[2] << " " << mMovement[3] << std::endl;
        if(mMovement[static_cast<int>(EMovement::M_FORWARD)])
        {
            translateBy(normalize(vec3(mForward.x, 0.0f, mForward.z)) * mMovementSpeed * timeDelta);
        }
        else if(mMovement[static_cast<int>(EMovement::M_BACKWARD)])
        {
            translateBy(normalize(- vec3(mForward.x, 0.0f, mForward.z)) * mMovementSpeed * timeDelta);
        }
        
        if(mMovement[static_cast<int>(EMovement::M_RIGHT)])
        {
            translateBy(mRight * mMovementSpeed * timeDelta);
        }
        else if(mMovement[static_cast<int>(EMovement::M_LEFT)])
        {
            translateBy(-mRight * mMovementSpeed * timeDelta);
        }
        else if(mMovement[static_cast<int>(EMovement::M_DOWN)])
        {
            translateBy(vec3(0.0f, 1.0f, 0.0f) * mMovementSpeed * timeDelta);
        }
        else if(mMovement[static_cast<int>(EMovement::M_UP)])
        {
            translateBy(vec3(0.0f, -1.0f, 0.0f) * mMovementSpeed * timeDelta);
        }
    }

    void Camera::updateViewMatrix()
    {
		mat4 rotation = mat4(1.0f);

		rotation = rotate(rotation, radians(mRotationAngles.x), vec3(1.0f, 0.0f, 0.0f));
		rotation = rotate(rotation, radians(mRotationAngles.y), vec3(0.0f, 1.0f, 0.0f));
        rotation = rotate(rotation, radians(mRotationAngles.z), vec3(0.0f, 0.0f, 1.0f));

		mat4 translation;
		translation = translate(mat4(1.0f), mEye);
        mat4 invTranslation;
		invTranslation = translate(mat4(1.0f), -mEye);

        mat4 scaling = scale(mat4(1.0f), vec3(mZoom));

        if(mIsFirstPerson)
        {
            mView = scaling * rotation * translation;
        }
        else
        {
            mView = translation * rotation * scaling;
        }
    }

    void Camera::updateVectors()
    {
        //if(mIsFirstPerson)
        {
            mForward.x = -cos(radians(mRotationAngles.x)) * sin(radians(mRotationAngles.y));
            mForward.y = sin(radians(mRotationAngles.x));
            mForward.z = cos(radians(mRotationAngles.x)) * cos(radians(mRotationAngles.y));
            mForward = normalize(mForward);
        }
        /*else
        {
            mForward.x = -cos(radians(-mRotationAngles.x)) * sin(radians(-mRotationAngles.y));
            mForward.y = sin(radians(-mRotationAngles.x));
            mForward.z = cos(radians(-mRotationAngles.x)) * cos(radians(-mRotationAngles.y));
            mForward = normalize(mForward);
        }*/

        mRight = normalize(cross(mForward, vec3(0.0f, 1.0f, 0.0f)));
        mUp = normalize(cross(mRight, mForward));

        //std::cout << "forward: " << mForward.x << " " << mForward.y << " " << mForward.z << std::endl;
    }

    const vec3& Camera::getForward() const
    {
        return mForward;
    }

    const vec3& Camera::getRight() const
    {
        return mRight;
    }

    const vec3& Camera::getUp() const
    {
        return mUp;
    }

    const vec3& Camera::getEye() const
    {
        return mEye;
    }

    void Camera::setEye(const vec3& eye)
    {
        mEye = eye;
        //std::cout << "Camera eye: " << mEye.x << ", " << mEye.y << ", " << mEye.z << std::endl;
    }
}