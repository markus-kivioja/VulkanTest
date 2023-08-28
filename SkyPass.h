#pragma once

#include "Buffer.h"
#include "Texture.h"
#include "RenderPass.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <memory>

class SkyPass : public RenderPass
{
public:
	SkyPass(VkPhysicalDevice physicalDevice, VkDevice device, std::vector<Texture*>& colorTargets);

	virtual void render(Scene* scene, VkCommandBuffer commandBuffer, uint32_t bufferIdx, float dt) override;
private:
	struct Vertex {
		glm::vec3 position;
	};

	std::vector<VkDescriptorSet> m_descriptorSets;

	std::unique_ptr<Texture> m_cubeMap;
	std::vector<SkyPass::Vertex> m_vertices;
	std::vector<uint16_t> m_indices;
};

