#pragma once

#include "GBufferPass.h"

#include <vulkan/vulkan.h>

#include <memory>

#include "Buffer.h"
#include "Texture.h"

class Camera;

class SceneObject
{
public:
	SceneObject(uint32_t id, VkPhysicalDevice physicalDevice, VkDevice device, VkCommandBuffer copyCommandBuffer,
		VkPipelineLayout pipelineLayout, VkDescriptorSetAllocateInfo descriptorSetAllocInfo);

	void render(VkCommandBuffer commandBuffer, uint32_t buffedIdx, Camera* camera, float dt);
private:
	inline static const std::string ALBEDO_FILENAME = "assets/crate.jpg" ;

	std::vector<VkDescriptorSet> m_descriptorSets;
	VkPipelineLayout m_pipelineLayout{ VK_NULL_HANDLE };

	std::unique_ptr<Buffer> m_vertexBuffer;
	std::unique_ptr<Buffer> m_indexBuffer;
	std::vector<std::unique_ptr<Buffer>> m_uniformBuffers;

	std::unique_ptr<Texture> m_albedoMap;
	std::unique_ptr<Texture> m_normalMap;
	std::unique_ptr<Texture> m_displacementMap;
	std::unique_ptr<Texture> m_specularMap;

	std::vector<GBufferPass::Vertex> m_vertices;
	std::vector<uint16_t> m_indices;

	float m_rotationSpeed{ 1.0f };
	float m_orientation = 0;

	uint32_t m_id{ 0 };
};

