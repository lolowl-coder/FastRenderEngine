#pragma once

#include "Utilities.hpp"

#include "imgui.h"

#include <glm/glm.hpp>
#include <vector>

namespace app
{
	bool inputInt(const int mn, const int mx, const char* id, const char* text, int& value, const int inc = 1, const int incFast = 2);
	bool inputInt(const int mn, const int mx, const char* text, int& value, const int inc = 1, const int incFast = 2);

	bool inputUInt(const uint32_t mn, const uint32_t mx, const char* id, const char* text, uint32_t& value, const int inc = 1, const int incFast = 2);
    bool inputUInt(const uint32_t mn, const uint32_t mx, const char* text, uint32_t& value, const int inc = 1, const int incFast = 2);

	bool inputFloat(const float mn, const float mx, const char* id, const char* text, float& value, const char* format = "%.3f", const float inc = 1.0f, const float incFast = 2.0f);
	bool inputFloat(const float mn, const float mx, const char* text, float& value, const char* format = "%.3f", const float inc = 1.0f, const float incFast = 2.0f);

	bool sliderInt(const int mn, const int mx, const char* text, int& value);
	bool sliderUInt(const uint32_t mn, const uint32_t mx, const char* text, uint32_t& value);

	bool sliderFloat(const float mn, const float mx, const char* text, float& value, const char* format = "%.3f");

	void constrainedWindow(const char* text, glm::vec2& winPos, glm::vec2& winSize, const fre::BoundingBox2D& constraints);
}