#pragma once

#include <glm/glm.hpp>

namespace fre
{
    // Function to check if a point is inside a circle
    bool isPointInCircle(const glm::vec2& p, const glm::vec2& circleCenter, float radius);

    // Function to check if a point is inside a rectangle
    bool isPointInRectangle(const glm::vec2& p, const glm::vec2& rectCenter, const glm::vec2& rectSize);

    // Function to find the closest point on the rectangle to a given point
    glm::vec2 closestPointOnRectangle(const glm::vec2& p, const glm::vec2& rectCenter, const glm::vec2& rectSize);

    //Circle-Rectangle intersection test
    bool testCircleRectangleIntersection(const glm::vec2& rectCenter, const glm::vec2& rectSize, const glm::vec2& circleCenter, float radius);
    bool testCircleContainsRectangle(const glm::vec2& rectCenter, const glm::vec2& rectSize, const glm::vec2& circleCenter, float radius);
}