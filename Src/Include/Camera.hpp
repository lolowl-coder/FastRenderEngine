#pragma once

#include <glm/glm.hpp>

namespace fre
{
    struct Camera
    {
        //Camera movement directions
        enum class EMovement
        {
            M_FORWARD,
            M_BACKWARD,
            M_LEFT,
            M_RIGHT,
            M_DOWN,
            M_UP,
            M_COUNT
        };
        //Sets perspective projection
        void setPerspectiveProjection(float fov, float aspectRatio);
        //Sets orthogonal projection
        void setOrthogonalProjection(float l, float r, float b, float t);
        //Rotates camera by axes
        void rotateBy(const glm::vec3& rotationAngles);
        //Translates camera
        void translateBy(const glm::vec3& translation);
        //Enables/disables movement of camera in given direction
        void setMovement(EMovement movement, bool value);
        //Updates all data
        void update(float timeDelta);
        //Moves camera
        void updateMovement(float timeDelta);
        //Updates view matrix
        void updateViewMatrix();
        //Updates camera vectors
        void updateVectors();
        //Returns forward vector
        const glm::vec3& getForward() const;
        //Returns rigth vector
        const glm::vec3& getRight() const;
        //Returns up vector
        const glm::vec3& getUp() const;
        //Returns camera's eye position
        const glm::vec3& getEye() const;
        //Sets camera's eye position
        void setEye(const glm::vec3& eye);

        float mFov = 45.0f;
		float mNear = 0.1f;
		float mFar = 100.0f;

        glm::mat4 mProjection;
        glm::mat4 mView;
        //yaw, pitch, roll
        glm::vec3 mRotationAngles = glm::vec3(0.0);
        glm::vec3 mForward = glm::vec3(0.0);
        glm::vec3 mRight = glm::vec3(0.0);
        glm::vec3 mUp = glm::vec3(0.0);
        glm::vec3 mEye = glm::vec3(0.0);
        float mMovementSpeed = 15.0f;
        bool mMovement[static_cast<int>(EMovement::M_COUNT)] = {false, false, false, false};
        float mMinPitch = -89.99f;
        float mMaxPitch = 89.99f;
        bool mIsFirstPerson = false;
        float mZoom = 1.0f;
    };
}