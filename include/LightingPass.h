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
	LightingPass(VkPhysicalDevice physicalDevice, VkDevice device, RenderThreadPool* threadPool, std::vector<Texture*>& colorTargets, std::vector<Texture*>& srcTextures);
	virtual ~LightingPass();

	virtual void renderImpl(Scene* scene, VkCommandBuffer commandBuffer, uint32_t bufferIdx, float dt) override;
private:
	struct Transforms {
		glm::mat4 projInverse;
		glm::mat4 viewInverse;
		glm::vec3 lightDir;
	};

	VkDescriptorPool m_descriptorPool{ VK_NULL_HANDLE };
	std::vector<VkDescriptorSet> m_descriptorSets;
	std::vector<std::unique_ptr<Buffer>> m_uniformBuffers;
};

