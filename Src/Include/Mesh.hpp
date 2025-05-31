#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include "Renderer/Callbacks.hpp"
#include "Member.hpp"
#include "Pointers.hpp"
#include "Utilities.hpp"

#include <limits>
#include <vector>

namespace fre
{
	class VulkanRenderer;

	class Mesh
	{
	public:
		using Ptr = std::shared_ptr<Mesh>;
		using RecordCallback = std::function<void(VulkanRenderer* renderer, uint32_t subPass, VkPipelineBindPoint pipelineBindPoint)>;

		//Vertices are raw data
		using Vertices = std::vector<uint8_t>;
		using Indices = std::vector<uint32_t>;
		Mesh();
		Mesh(uint32_t materialId);
		~Mesh();

		uint32_t getId() const;

		//Vertex array represented by raw bytes for flexibility
		void setVertices(const Vertices& vertices, uint32_t vertexSize);
		void setIndices(const Indices& indices);

		GETTER_SETTER(uint32_t, MaterialId);

		//Returns size of vertex in bytes
		uint32_t getVertexSize() const;
		uint32_t getVertexCount() const;
		//Vertices to generate instead of passing vertex buffer
		FIELD_NS(uint32_t, GeneratedVerticesCount, private, public, public);
		uint32_t getIndexCount() const;

		//Vertex array raw data
		const void* getVertexData() const;
		//Index array raw data
		const void* getIndexData() const;

		GETTER_SETTER(BoundingBox3D, BoundingBox);
		GETTER_SETTER(uint32_t, ComputeShaderId);
		GETTER_SETTER(glm::vec3, ComputeSpace);

		//Visit callbacks
		GETTER_SETTER(RecordCallback, BeforeVisitCallback);
		GETTER_SETTER(RecordCallback, AfterVisitCallback);

		//Record callbacks
		GETTER_SETTER(RecordCallback, BeforeRecordCallback); 
		GETTER_SETTER(RecordCallback, AfterRecordCallback); 

		GETTER_SETTER(bool, Visible);

		GETTER_SETTER(uint32_t, InstanceCount);

		FIELD_NS(std::vector<uint32_t>, DescriptorSets, private, public, public);

        FIELD_NS(std::vector<std::vector<VulkanDescriptorPtr>>, Descriptors, private, public, public);

	public:
		//Callback to pass variables to shader
		PushConstantCallback mPushConstantsCallback = nullptr;
		//Callback to pass data like textures or buffers to shader
		BindDescriptorSetsCallback mBindDescriptorSetsCallback = nullptr;

	private:
		uint32_t mId = std::numeric_limits<uint32_t>::max();
		uint32_t mMaterialId = std::numeric_limits<uint32_t>::max();
		uint32_t mComputeShaderId = std::numeric_limits<uint32_t>::max();

		uint32_t mVertexSize = 0;
		Vertices mVertices;
		Indices mIndices;

		BoundingBox3D mBoundingBox = BoundingBox3D(glm::vec3(0.0f), glm::vec3(0.0f));

		std::string mComputeShaderName;

		glm::ivec3 mComputeSpace;

		//Visit callbacks
		RecordCallback mBeforeVisitCallback = nullptr;
		RecordCallback mAfterVisitCallback = nullptr;

		//Record callbacks
		RecordCallback mBeforeRecordCallback = nullptr;
		RecordCallback mAfterRecordCallback = nullptr;

		bool mVisible = true;
		uint32_t mInstanceCount = 1;
	};
}