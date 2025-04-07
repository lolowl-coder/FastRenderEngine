#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#ifdef _WIN64
	#define NOMINMAX
	#include <windows.h>
	#include <vulkan/vulkan_win32.h>
#endif /* _WIN64 */

#include "Renderer/VulkanQueueFamily.hpp"
#include "Renderer/VulkanAttachment.hpp"
#include "Log.hpp"

#include "imgui.h"

#include <vector>
#include <fstream>
#include <glm/glm.hpp>

#define SCENE_SCALE 1.0f
constexpr double PI = 3.14159265358979323846;

#define WM_CUSTOM_MESSAGE (WM_USER + 1)

namespace fre
{
	const int MAX_FRAME_DRAWS = 3;
	const int MAX_OBJECTS = 40;
	const EAttachmentKind COLOR_ATTACHMENT = EAttachmentKind::Color;
	const EAttachmentKind POSITION_ATTACHMENT = EAttachmentKind::Color16;
	const EAttachmentKind DEPTH_ATTACHMENT = EAttachmentKind::DepthStencil;

	struct ShaderMetaData;

	struct MainDevice
	{
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice logicalDevice = VK_NULL_HANDLE;
	};

	//Default vertex
	struct Vertex
	{
		glm::vec3 pos = glm::vec3(0.0f);
		glm::vec3 normal = glm::vec3(0.0f);
		glm::vec3 tangent = glm::vec3(0.0f);
		glm::vec2 tex = glm::vec2(0.0f);
	};

	template<class T>
	struct BoundingBox
	{
		BoundingBox()
			: mMin(std::numeric_limits<float>::max())
			, mMax(std::numeric_limits<float>::min())
		{
		}

		BoundingBox(const T& mn, const T& mx)
			: mMin(mn)
			, mMax(mx)
		{}

		T getCenter() const
		{
			return (mMax + mMin) * 0.5f;
		}

		T getSize() const
		{
			return mMax - mMin;
		}

		bool operator == (const BoundingBox<T>& other)
		{
			return isEqual(mMin);
		}

		T mMin;
		T mMax;
	};

	using BoundingBox3D = BoundingBox<glm::vec3>;
	using BoundingBox2D = BoundingBox<glm::vec2>;

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	struct SwapChainDetails
	{
		VkSurfaceCapabilitiesKHR surfaceCapabilities;		//Surface properties, e.g. image size/extent
		std::vector<VkSurfaceFormatKHR> formats;			//Surface image formats, e.g RGBA and size of each color
		std::vector<VkPresentModeKHR> presentationModes;	//How images should be presented to screen
	};

	std::vector<char> readFile(const std::string& fileName);

	uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypes, VkMemoryPropertyFlags properties);

	void createBuffer(const MainDevice& mainDevice, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage,
		VkMemoryPropertyFlags bufferProperties, VkBuffer* buffer, VkDeviceMemory* bufferMemory);

	VkCommandBuffer beginCommandBuffer(VkDevice device, VkCommandPool commandPool);

	void endAndSubmitCommitBuffer(VkDevice device, VkCommandPool commandPool,
		VkQueue queue, VkCommandBuffer commandBuffer);

	void copyBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool,
		VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize);

	std::vector<VulkanQueueFamily> getQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

	template<typename T>
	int getIndexOf(const std::vector<T>& v, const T& value)
	{
		int result = -1;
		const auto& foundIt = std::find(v.begin(), v.end(), value);
		if(foundIt != v.end())
		{
			result = static_cast<int>(std::distance(v.begin(), foundIt));
		}

		return result;
	}

	template<typename T>
	bool areEqual(T a, T b, T epsilon = std::numeric_limits<T>::epsilon())
	{
		return std::abs(a - b) < epsilon;
	}

	uint32_t getNextPowerOf2(uint32_t x);

    glm::vec4 toWorldSpace(const glm::vec2& screenSpace, const glm::vec2& viewSize, const glm::mat4& m, const glm::mat4& v, const glm::mat4& p, float depth = 0.5f);

	glm::vec2 toScreenSpace(const glm::vec3 worldSpace, const glm::vec2& viewSize, const glm::mat4& m, const glm::mat4& v, const glm::mat4& p);

	ImVec2 operator + (const ImVec2& lhs, const ImVec2& rhs);
	ImVec2 operator - (const ImVec2& lhs, const ImVec2& rhs);
	ImVec2 operator * (const ImVec2& lhs, const float rhs);
	ImVec2 operator / (const ImVec2& lhs, const float rhs);

	std::string formatStringCutTrZeroes(const char* format, ...);
	std::string formatString(const char* format, ...);

	void renderLabel(const glm::vec2& screenPos, const char* text, const char* id, const ImVec4& textColor, const ImVec4& outlineColor, const glm::vec2& textAdustment);

	#define MAX(T) std::numeric_limits<T>::max()
	#define MIN(T) std::numeric_limits<T>::min()
	template<typename T>
	T maxValue(const T value)
	{
		return std::numeric_limits<T>::max();
	}

	void onVulkanError(VkResult vulkanResult);

	#define VK_CHECK(c)\
	{\
		VkResult vulkanResult = c;\
		if(vulkanResult != VK_SUCCESS)\
		{\
			LOG_ERROR("Vulkan error {}, {}", vulkanResult, #c);\
			onVulkanError(vulkanResult);\
		}\
	}

	VkExternalSemaphoreHandleTypeFlagBits getDefaultSemaphoreHandleType();

	VkExternalMemoryHandleTypeFlagBits getDefaultMemHandleType();

	#ifdef _WIN64
		class WindowsSecurityAttributes
		{
		protected:
			SECURITY_ATTRIBUTES m_winSecurityAttributes;
			PSECURITY_DESCRIPTOR m_winPSecurityDescriptor;

		public:
			WindowsSecurityAttributes();
			SECURITY_ATTRIBUTES* operator&();
			~WindowsSecurityAttributes();
		};
	#endif /* _WIN64 */

	bool isEqual(const float a, const float b, const float epsilon = 1e-5f);
	bool isEqual(const glm::vec2& a, const glm::vec2& b, const float epsilon = 1e-5f);
	bool isEqual(const BoundingBox2D& a, const BoundingBox2D& b, const float epsilon = 1e-5f);

	std::string getCurrentDateTime();

	bool isBoundingBoxValid(const BoundingBox2D& bb);

	// Function to find the next power of 2
	inline unsigned int nextPowerOf2(unsigned int n) {
		n--;
		n |= n >> 1;
		n |= n >> 2;
		n |= n >> 4;
		n |= n >> 8;
		n |= n >> 16;
		n++;
		return n;
	}

	// Function to find the previous power of 2
	inline unsigned int previousPowerOf2(unsigned int n) {
		if (n == 0) return 0;
		unsigned int p = 1;
		while (p <= n) p <<= 1;
		return p >> 1;
	}

	// Function to find the closest power of 2
	inline unsigned int closestPowerOf2(unsigned int n) {
		unsigned int nextP = nextPowerOf2(n);
		unsigned int prevP = previousPowerOf2(n);

		if (n - prevP < nextP - n) {
			return prevP;
		} else {
			return nextP;
		}
	}

	template<typename T>
	void clampValue(T& value, T mn, T mx)
	{
		value = glm::clamp(value, mn, mx);
	}

	bool endsWith(const std::string& str, const std::string& suffix);
}