#pragma once

#include "RenderPass.h"
#include "Buffer.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>

class LightingPass : public RenderPass
{
public:
	LightingPass(VkPhysicalDevice physicalDevice, VkDevice device, std::vector<Texture*> colorTargets, std::vector<Texture*> srcTextures);
	virtual ~LightingPass();

	virtual void render(VkCommandBuffer commandBuffer, uint32_t buffedIdx);
private:
	struct Transforms {
		glm::mat4 projInverse;
	};

	VkDescriptorPool m_descriptorPool{ VK_NULL_HANDLE };
	std::vector<VkDescriptorSet> m_descriptorSets;
	std::vector<std::unique_ptr<Buffer>> m_uniformBuffers;
};

