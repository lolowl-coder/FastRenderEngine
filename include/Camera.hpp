#pragma once

#include <glm/glm.hpp>

namespace fre
{
    struct Camera
    {
        enum EMovement
        {
            M_FORWARD,
            M_BACKWARD,
            M_LEFT,
            M_RIGHT,
            M_DOWN,
            M_UP,
            M_COUNT
        };

        void setPerspectiveProjection(float fov, float aspectRatio, float near, float far);
        void setOrthogonalProjection(float l, float r, float b, float t, float near, float far);
        void rotateBy(const glm::vec3& rotationAngles);
        void translateBy(const glm::vec3& translation);
        void setMovement(EMovement movement, bool value);
        void update(float timeDelta);
        void updateMovement(float timeDelta);
        void updateViewMatrix();
        void updateVectors();
        const glm::vec3& getForward() const;
        const glm::vec3& getRight() const;
        const glm::vec3& getUp() const;
        const glm::vec3& getEye() const;
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
        bool mMovement[M_COUNT] = {false, false, false, false};
    };
}