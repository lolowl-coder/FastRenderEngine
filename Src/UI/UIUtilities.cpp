#include "UI/UIUtilities.hpp"

#include "imgui_internal.h"

using namespace fre;
using namespace glm;

namespace app
{
    bool inputInt(const int mn, const int mx, const char* id, const char* text, int& value, const int inc, const int incFast)
    {
        ImGui::PushID(id);
	    bool result = inputInt(mn, mx, text, value, inc, incFast);
	    ImGui::PopID();

        return result;
    }

    bool inputInt(const int mn, const int mx, const char* text, int& value, const int inc, const int incFast)
    {
        bool result = false;
	    if(ImGui::InputInt(text, &value, inc, incFast))
        {
            clampValue(value, mn, mx);
            result = true;
        }

        return result;
    }

    bool inputUInt(const uint32_t mn, const uint32_t mx, const char* id, const char* text, uint32_t& value, const int inc, const int incFast)
    {
        int tmp = value;
	    bool result = inputInt(mn, mx, id, text, tmp, inc, incFast);
        value = tmp;

        return result;
    }

    bool inputUInt(const uint32_t mn, const uint32_t mx, const char* text, uint32_t& value, const int inc, const int incFast)
    {
        int tmp = value;
        bool result = inputInt(mn, mx, text, tmp, inc, incFast);
        value = tmp;

        return result;
    }

    bool inputFloat(const float mn, const float mx, const char* id, const char* text, float& value, const char* format, const float inc, const float incFast)
    {
        ImGui::PushID(id);
	    bool result = inputFloat(mn, mx, text, value, format, inc, incFast);
	    ImGui::PopID();

        return result;
    }

    bool inputFloat(const float mn, const float mx, const char* text, float& value, const char* format, const float inc, const float incFast)
    {
        bool result = false;
	    if(ImGui::InputFloat(text, &value, inc, incFast, format))
        {
            clampValue(value, mn, mx);
            result = true;
        }

        return result;
    }

    bool sliderInt(const int mn, const int mx, const char* text, int& value)
    {
        return ImGui::SliderInt(text, &value, mn, mx, "%d", ImGuiSliderFlags_AlwaysClamp);
    }

    bool sliderUInt(const uint32_t mn, const uint32_t mx, const char* text, uint32_t& value)
    {
        int tmp = value;
        bool result = sliderInt(mn, mx, text, tmp);
        value = tmp;

        return result;
    }

    bool sliderFloat(const float mn, const float mx, const char* text, float& value, const char* format)
    {
        return ImGui::SliderFloat(text, &value, mn, mx, format, ImGuiSliderFlags_AlwaysClamp);
    }

    void constrainedWindow(const char* text, vec2& winPos, vec2& winSize, const BoundingBox2D& constraints)
    {
        ImVec2& wp = (ImVec2&)winPos;
		ImVec2& ws = (ImVec2&)winSize;
		static bool dragging = false;
		static bool resizing = false;
		static ImVec2 dragOffset(0.0f, 0.0f);
		static ImVec2 resizeOffset(0.0f, 0.0f);

		ImGui::SetNextWindowPos(wp);
		ImGui::SetNextWindowSize(ws);

		ImGui::Begin("Wafer", nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoFocusOnAppearing |
			ImGuiWindowFlags_NoNavInputs |
			ImGuiWindowFlags_NoNavFocus
		);

        //ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
			
		// Resizing logic from the bottom-right corner
		ImVec2 cornerPos = wp + ws;
		ImGui::SetCursorScreenPos(cornerPos - ImVec2(20, 20));  // Offset for visible resize corner
		if(ImGui::InvisibleButton("resizeCorner", ImVec2(40, 40)) || ImGui::IsItemActivated())
		{
			resizing = true;
			resizeOffset = ImGui::GetMousePos() - cornerPos;
		}

		// Start dragging if the mouse is clicked inside the window
		if(ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && !resizing)
		{
			dragging = true;
			dragOffset = ImGui::GetMousePos() - wp;
		}

		if(dragging)
		{
			if (ImGui::IsMouseReleased(0))
			{
				dragging = false;
			}
			else
			{
				// Update window position manually
				wp = ImGui::GetMousePos() - dragOffset;
				ImGui::SetWindowPos(wp);  // Force update window position
			}
		}

		const float minSize = 150.0f;
		//wp = *(vec2*)(&ImGui::GetWindowPos());
		if(wp.x < constraints.mMin.x)
		{
			wp.x = constraints.mMin.x;
			ImGui::SetWindowPos(*(ImVec2*)(&wp));
		}
		if(wp.y < constraints.mMin.y)
		{
			wp.y = constraints.mMin.y;
			ImGui::SetWindowPos(*(ImVec2*)(&wp));
		}
		if(wp.x > constraints.mMax.x - minSize)
		{
			wp.x = constraints.mMax.x - minSize;
			ImGui::SetWindowPos(*(ImVec2*)(&wp));
		}
		if(wp.y > constraints.mMax.y - minSize)
		{
			wp.y = constraints.mMax.y - minSize;
			ImGui::SetWindowPos(*(ImVec2*)(&wp));
		}
		if(wp.x > constraints.mMax.x - ws.x)
		{
			wp.x = constraints.mMax.x - ws.x;
			ImGui::SetWindowPos(*(ImVec2*)(&wp));
		}
		if(wp.y > constraints.mMax.y - ws.y)
		{
			wp.y = constraints.mMax.y - ws.y;
			ImGui::SetWindowPos(*(ImVec2*)(&wp));
		}

		if (resizing)
		{
			if (ImGui::IsMouseReleased(0))
			{
				resizing = false;
			}
			else
			{
				// Compute new size
				ImVec2 newSize = ImGui::GetMousePos() - ImVec2(wp.x, wp.y) - resizeOffset;
				const vec2 tmp = constraints.mMax - winPos;
				const ImVec2& maxSize = (ImVec2&)(tmp);

                newSize = ImClamp(newSize, ImVec2(minSize, minSize), maxSize);

				// Apply the constrained size
				ws = newSize;
				ImGui::SetWindowSize(ws);
			}
		}
    }

    ImVec2 toImVec2(const glm::vec2& v)
    {
        return ImVec2(v.x, v.y);
    }

    glm::vec2 toVec2(const ImVec2& v)
    {
        return glm::vec2(v.x, v.y);
    }
}