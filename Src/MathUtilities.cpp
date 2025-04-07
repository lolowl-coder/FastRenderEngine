#include "MathUtilities.hpp"

using namespace glm;

namespace fre
{
	// Function to check if a point is inside a circle
    bool isPointInCircle(const vec2& p, const vec2& circleCenter, float radius)
    {
        auto dist = p - circleCenter;
        float distance = length(dist);
        return distance <= radius;
    }

    // Function to check if a point is inside a rectangle
    bool isPointInRectangle(const vec2& p, const vec2& rectCenter, const vec2& rectSize)
    {
        float halfWidth = rectSize.x * 0.5f;
        float halfHeight = rectSize.y * 0.5f;
        return (p.x >= rectCenter.x - halfWidth && p.x <= rectCenter.x + halfWidth &&
                p.y >= rectCenter.y - halfHeight && p.y <= rectCenter.y + halfHeight);
    }

    // Function to find the closest point on the rectangle to a given point
    vec2 closestPointOnRectangle(const vec2& p, const vec2& rectCenter, const vec2& rectSize)
    {
        float halfWidth = rectSize.x * 0.5f;
        float halfHeight = rectSize.y * 0.5f;
        float closestX = std::max(rectCenter.x - halfWidth, std::min(p.x, rectCenter.x + halfWidth));
        float closestY = std::max(rectCenter.y - halfHeight, std::min(p.y, rectCenter.y + halfHeight));
        return {closestX, closestY};
    }

    //Circle-Rectangle intersection test
    bool testCircleRectangleIntersection(const vec2& rectCenter, const vec2& rectSize, const vec2& circleCenter, float radius)
    {
        bool result = false;
        // Check if the circle's center is inside the rectangle
        if (isPointInRectangle(circleCenter, rectCenter, rectSize)) {
            result = true;
        }
        else
        {
            // Find the closest point on the rectangle to the circle's center
            vec2 closestPoint = closestPointOnRectangle(circleCenter, rectCenter, rectSize);

            // Check if this closest point is inside the circle
            result = isPointInCircle(closestPoint, circleCenter, radius);
        }

        return result;
    }

    bool testCircleContainsRectangle(const vec2& rectCenter, const vec2& rectSize, const vec2& circleCenter, float radius)
    {
        auto p0 = rectCenter + vec2(-rectSize.x, rectSize.y) * 0.5f;
        auto p1 = rectCenter + vec2(rectSize) * 0.5f;
        auto p2 = rectCenter + vec2(rectSize.x, -rectSize.y) * 0.5f;
        auto p3 = rectCenter + vec2(-rectSize) * 0.5f;
        return isPointInCircle(p0, circleCenter, radius) && isPointInCircle(p1, circleCenter, radius)
            && isPointInCircle(p2, circleCenter, radius) && isPointInCircle(p3, circleCenter, radius);
    }
}