#pragma once

#include "GBufferPass.h"
#include "Buffer.h"
#include "Material.h"
#include "Mesh.h"
#include "Tranforms.h"

#include <vulkan/vulkan.h>

#include <memory>
#define _USE_MATH_DEFINES
#include <math.h>

class Camera;

class SceneObject
{
public:
	SceneObject(uint32_t id, VkPhysicalDevice physicalDevice, VkDevice device, VkCommandBuffer copyCommandBuffer,
		VkDescriptorSetAllocateInfo descSetAllocInfo);
	~SceneObject();

	void init();
	virtual void update(uint32_t bufferIdx, float dt) = 0;
	void render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t bufferIdx, float dt);
protected:
	uint16_t addVertex(uint32_t hash, GBufferPass::Vertex* pVertex);
	void loadIndexedMesh(std::string const& filename);

	VkPhysicalDevice m_physicalDevice;
	VkDevice m_device;
	VkCommandBuffer m_copyCommandBuffer;

	Transforms* m_transforms{ nullptr };
	
	std::unique_ptr<Mesh> m_mesh{ nullptr };
	std::unique_ptr<Material> m_material{ nullptr };

	std::vector<GBufferPass::Vertex> m_vertices;
	std::vector<uint16_t> m_indices;

	float m_rotationSpeed{ 1.0f };
	float m_orientation = 0;

	uint32_t m_id{ 0 };
};

