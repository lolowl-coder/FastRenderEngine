#include "Engine.hpp"
#include "Shader.hpp"
#include "Utilities.hpp"

#include <sstream>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>

#ifdef _WIN64
	#include <VersionHelpers.h>
	#include <dxgi1_2.h>
	#include <aclapi.h>
	#include <windows.h>
#endif /* _WIN64 */

using namespace glm;

namespace fre
{
    std::vector<char> readFile(const std::string& fileName)
	{
		//Open stream from given file
		std::ifstream file(fileName, std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open file! " + fileName);
		}

		const auto fileSize = (size_t)file.tellg();
		file.seekg(0);
		std::vector<char> fileBuffer(fileSize);
		file.read(fileBuffer.data(), fileSize);

		file.close();

		return fileBuffer;
	}

    uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypes, VkMemoryPropertyFlags properties)
	{
		//Get properties of physical device memory
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
		{
			if ((allowedTypes & (i << 1))	//Index of memory type must match corresponding bit in allowedTypes
				&& (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)	//Desired property bit flags are part of memory type's property flags
			{
				return i;
			}
		}

		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
		{
			if ((allowedTypes & 1) == 1)
			{
				if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
				{
					return i;
				}
			}
			allowedTypes >>= 1;
		}

		LOG_ERROR("Could not find a suitable memory type!");

		return std::numeric_limits<uint32_t>::max();
	}

    void createBuffer(const MainDevice& mainDevice, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage,
		VkMemoryPropertyFlags bufferProperties, VkMemoryAllocateFlags allocFlags,
		VkBuffer* buffer, uint64_t* deviceAddress, VkDeviceMemory* bufferMemory)
	{
		//Information to create a buffer (doesn't include assigning memory)
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = bufferSize;
		//Multiple types of buffers possible
		bufferInfo.usage = bufferUsage;
		//Similar to swap chain images can share vertex buffers
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK(vkCreateBuffer(mainDevice.logicalDevice, &bufferInfo, nullptr, buffer));

		//Get buffer memory requirements
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(mainDevice.logicalDevice, *buffer, &memRequirements);

		VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo = {};
		memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
		memoryAllocateFlagsInfo.flags = allocFlags;

		//ALLOCATE MEMORY TO BUFFER
		VkMemoryAllocateInfo memoryAllocInfo = {};
		memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocInfo.pNext = &memoryAllocateFlagsInfo;
		memoryAllocInfo.allocationSize = memRequirements.size;
		//index of memory type on Physical Device that has required bit flags
		memoryAllocInfo.memoryTypeIndex = findMemoryTypeIndex(mainDevice.physicalDevice, memRequirements.memoryTypeBits,
			bufferProperties);
		//VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT - CPU can interact with memory
		//VK_MEMORY_PROPERTY_HOST_COHERENT_BIT - Allows placement of data straight into buffer after mapping (otherwise would have to specify manually)

		//Allocate memory to VkDeviceMemory
		VK_CHECK(vkAllocateMemory(mainDevice.logicalDevice, &memoryAllocInfo, nullptr, bufferMemory));

		//Allocate memory for given buffer
		VK_CHECK(vkBindBufferMemory(mainDevice.logicalDevice, *buffer, *bufferMemory, 0));

		if(deviceAddress != nullptr)
		{
			if((bufferUsage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) != 0)
			{
				*deviceAddress = getBufferDeviceAddress(mainDevice.logicalDevice, *buffer);
			}
			else
			{
				LOG_ERROR("createBuffer: failed to get device address. Buffer usage should contain VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT flag.");
			}
		}
		LOG_TRACE("Vulkan buffer created: {}", (uint64_t)*buffer);
	}

    VkCommandBuffer beginCommandBuffer(VkDevice device, VkCommandPool commandPool)
	{
		//Command buffer to hold transfer command
		VkCommandBuffer commandBuffer;

		//Command Buffer details
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		//Allocate command buffer from pool
		VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer));

		//Information to begin the command buffer record
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;	//We're only using the command buffer once, so set up for one time submit

		//Begin recording transfer commands
		VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

		return commandBuffer;
	}

    void endAndSubmitCommitBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer)
	{
		VK_CHECK(vkEndCommandBuffer(commandBuffer));

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		//Submit transfer commands to transfer queue and wait until it finished
		VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
		VK_CHECK(vkQueueWaitIdle(queue));

		//Free temporary command buffer back to pool
		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	void copyBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool,
		VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize)
	{
		VkCommandBuffer transferCommandBuffer = beginCommandBuffer(device, transferCommandPool);

		//Region of data to copy from and to
		VkBufferCopy bufferCopyRegion = {};
		bufferCopyRegion.srcOffset = 0;
		bufferCopyRegion.dstOffset = 0;
		bufferCopyRegion.size = bufferSize;

		//Command to copy srcBuffer to dstBuffer
		vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);

		endAndSubmitCommitBuffer(device, transferCommandPool, transferQueue, transferCommandBuffer);
	}

	std::vector<VulkanQueueFamily> getQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		//Get all Queue Family properties info for the given device
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());
		
		std::vector<VulkanQueueFamily> result(queueFamilyCount);
		for(uint8_t i = 0; i < queueFamilyList.size(); i++)
		{
			result[i].init(device, surface, queueFamilyList[i], i);
		}

		return result;
	}

	uint32_t getNextPowerOf2(uint32_t x)
	{
		return pow(2, ceil(log(x)/log(2)));
	}

	vec4 toWorldSpace(const vec2& screenSpace, const vec2& viewSize, const mat4& m, const mat4& v, const mat4& p, float depth)
	{
        vec2 ndc = vec2(
            screenSpace.x / viewSize.x * 2.0f - 1.0f,
            (screenSpace.y / viewSize.y * 2.0f - 1.0f));
        vec4 clip = vec4(ndc, 0.0f, 1.0f);
        mat4 invProjection = inverse(p);
        mat4 invView = inverse(v);
        mat4 invModel = inverse(m);

        vec4 result = invProjection * clip;
        result = invView * result;
        result = invModel * result;

        return result;
	}

	vec2 toScreenSpace(const vec3 worldSpace, const vec2& viewSize, const mat4& m, const mat4& v, const mat4& p)
	{
		//clip space
		vec4 result = p * v * m * vec4(worldSpace, 1.0f);
		//ndc
		result = result / result.w;
		//screen space
		result.x = (result.x + 1.0f) * 0.5f * viewSize.x;
		result.y = (result.y + 1.0f) * 0.5f * viewSize.y;

		return vec2(result);
	}

	ImVec2 operator + (const ImVec2& lhs, const ImVec2& rhs)
	{
		return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
	}

	ImVec2 operator - (const ImVec2& lhs, const ImVec2& rhs)
	{
		return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y);
	}

	ImVec2 operator * (const ImVec2& lhs, const float rhs)
	{
		return ImVec2(lhs.x * rhs, lhs.y * rhs);
	}

	ImVec2 operator / (const ImVec2& lhs, const float rhs)
	{
		return ImVec2(lhs.x / rhs, lhs.y / rhs);
	}

	std::string formatStringCutTrZeroes(const char* format, ...)
	{
		// Initialize the va_list
		va_list args;
		va_start(args, format);

		// Determine the length of the formatted string
		int length = std::vsnprintf(nullptr, 0, format, args);
		va_end(args);

		// Allocate a buffer to hold the formatted string
		std::string buffer(length, '\0'); // No need to +1 for null terminator in std::string

		// Format the string into the buffer
		va_start(args, format);
		std::vsnprintf(&buffer[0], buffer.size() + 1, format, args);
		va_end(args);

		// Remove trailing zeroes and the trailing decimal point if necessary
		if(strcmp(format, "%.f") != 0 && std::string(format).find(" ") == std::string::npos)
		{
			size_t len = buffer.size();
			while(len > 1 && buffer[len - 1] == '0')
			{
				len--;
			}
			if(len > 1 && buffer[len - 1] == '.')
			{
				len--; // Remove trailing decimal point as well
			}
			buffer.resize(len); // Properly resize the buffer
		}

		return buffer;
	}

	std::string formatString(const char* format, ...)
	{
		// Initialize the va_list
		va_list args;
		va_start(args, format);

		// Determine the length of the formatted string
		int length = std::vsnprintf(nullptr, 0, format, args);
		va_end(args);

		// Allocate a buffer to hold the formatted string
		std::string buffer(length + 1, '\0'); // +1 for null terminator

		// Format the string into the buffer
		va_start(args, format);
		std::vsnprintf(&buffer[0], buffer.size() + 1, format, args);
		va_end(args);

		return buffer;
	}

	void renderLabel(const vec2& screenPos, const char* text, const char* id, const ImVec4& textColor, const ImVec4& outlineColor, const vec2& textAdustment)
	{
		auto textSize = ImGui::CalcTextSize(text);
		ImGui::PushStyleColor(ImGuiCol_Text, outlineColor);
		const auto cursorPos = ImVec2(screenPos.x - textSize.x * textAdustment.x, screenPos.y - textSize.y * textAdustment.y);
        ImGui::SetCursorPos(cursorPos + ImVec2(1, 0));
        ImGui::Text(text);
        ImGui::SetCursorPos(cursorPos + ImVec2(0, 1));
        ImGui::Text(text);
        ImGui::SetCursorPos(cursorPos + ImVec2(-1, 0));
        ImGui::Text(text);
        ImGui::SetCursorPos(cursorPos + ImVec2(0, -1));
        ImGui::Text(text);
        ImGui::PopStyleColor();
		ImGui::PushStyleColor(ImGuiCol_Text, textColor);
        ImGui::SetCursorPos(cursorPos);
		ImGui::Text(text);
		ImGui::PopStyleColor();
	}

	void onVulkanError(VkResult vulkanResult)
	{
		throw std::runtime_error(formatString("Vulkan error %i", vulkanResult));
	}

	VkExternalSemaphoreHandleTypeFlagBits getDefaultSemaphoreHandleType()
	{
	#ifdef _WIN64
		return IsWindows8OrGreater()
			? VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT
			: VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT;
	#else
		return VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
	#endif /* _WIN64 */
	}

	VkExternalMemoryHandleTypeFlagBits getDefaultMemHandleType()
	{
	#ifdef _WIN64
		return IsWindows8Point1OrGreater()
		? VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT
		: VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT;
	#else
		return VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
	#endif /* _WIN64 */
	}

	#ifdef _WIN64
		WindowsSecurityAttributes::WindowsSecurityAttributes()
		{
			m_winPSecurityDescriptor = (PSECURITY_DESCRIPTOR)calloc(
				1, SECURITY_DESCRIPTOR_MIN_LENGTH + 2 * sizeof(void**));
			if(!m_winPSecurityDescriptor)
			{
				throw std::runtime_error(
					"Failed to allocate memory for security descriptor");
			}

			PSID* ppSID = (PSID*)((PBYTE)m_winPSecurityDescriptor +
				SECURITY_DESCRIPTOR_MIN_LENGTH);
			PACL* ppACL = (PACL*)((PBYTE)ppSID + sizeof(PSID*));

			InitializeSecurityDescriptor(m_winPSecurityDescriptor,
				SECURITY_DESCRIPTOR_REVISION);

			SID_IDENTIFIER_AUTHORITY sidIdentifierAuthority =
				SECURITY_WORLD_SID_AUTHORITY;
			AllocateAndInitializeSid(&sidIdentifierAuthority, 1, SECURITY_WORLD_RID, 0, 0,
				0, 0, 0, 0, 0, ppSID);

			EXPLICIT_ACCESS explicitAccess;
			ZeroMemory(&explicitAccess, sizeof(EXPLICIT_ACCESS));
			explicitAccess.grfAccessPermissions =
				STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL;
			explicitAccess.grfAccessMode = SET_ACCESS;
			explicitAccess.grfInheritance = INHERIT_ONLY;
			explicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_SID;
			explicitAccess.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
			explicitAccess.Trustee.ptstrName = (LPTSTR)*ppSID;

			SetEntriesInAcl(1, &explicitAccess, NULL, ppACL);

			SetSecurityDescriptorDacl(m_winPSecurityDescriptor, TRUE, *ppACL, FALSE);

			m_winSecurityAttributes.nLength = sizeof(m_winSecurityAttributes);
			m_winSecurityAttributes.lpSecurityDescriptor = m_winPSecurityDescriptor;
			m_winSecurityAttributes.bInheritHandle = TRUE;
		}

		SECURITY_ATTRIBUTES* WindowsSecurityAttributes::operator&()
		{
			return &m_winSecurityAttributes;
		}

		WindowsSecurityAttributes::~WindowsSecurityAttributes()
		{
			PSID* ppSID = (PSID*)((PBYTE)m_winPSecurityDescriptor +
				SECURITY_DESCRIPTOR_MIN_LENGTH);
			PACL* ppACL = (PACL*)((PBYTE)ppSID + sizeof(PSID*));

			if(*ppSID)
			{
				FreeSid(*ppSID);
			}
			if(*ppACL)
			{
				LocalFree(*ppACL);
			}
			free(m_winPSecurityDescriptor);
		}
	#endif /* _WIN64 */

	bool isEqual(const float a, const float b, const float epsilon)
	{
		return std::abs(a - b) < epsilon;
	}

	bool isEqual(const vec2& a, const vec2& b, const float epsilon)
	{
		return std::abs(a.x - b.x) < epsilon && std::abs(a.y - b.y) < epsilon;
	}

	bool isEqual(const BoundingBox2D& a, const BoundingBox2D& b, const float epsilon)
	{
		return
			std::abs(a.mMin.x - b.mMin.x) < epsilon && std::abs(a.mMin.y - b.mMin.y) < epsilon &&
			std::abs(a.mMax.x - b.mMax.x) < epsilon && std::abs(a.mMax.y - b.mMax.y) < epsilon;
	}

	std::string getCurrentDateTime()
	{
		// Get current time
		auto now = std::chrono::system_clock::now();

		// Convert to time_t
		std::time_t time = std::chrono::system_clock::to_time_t(now);

		// Convert to local time
		std::tm* tm = std::localtime(&time);
		
		auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

		// Format the date and time
		std::ostringstream oss;
		oss << std::put_time(tm, "%Y-%m-%d-%H-%M-%S") << '-' << std::setw(3) << std::setfill('0') << millis.count();

		return oss.str();
	}

	bool isBoundingBoxValid(const BoundingBox2D& bb)
	{
		return
			bb.mMin.x < bb.mMax.x &&
			bb.mMin.y < bb.mMax.y
			/*!isEqual(bb.mMin.x, bb.mMax.x) ||
            !isEqual(bb.mMin.y, bb.mMax.y);*/
			;
	}

	bool endsWith(const std::string& str, const std::string& suffix)
	{
		return str.size() >= suffix.size() &&
			   str.rfind(suffix) == (str.size() - suffix.size());
	}
}