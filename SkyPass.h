#pragma once

#include "RenderPass.h"
#include "EnvironmentCube.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <memory>

class SkyPass : public RenderPass
{
public:
	SkyPass(VkPhysicalDevice physicalDevice, VkDevice device, std::vector<Texture*>& colorTargets, VkQueue queue, uint32_t queueFamilyIdx);
	virtual ~SkyPass() override;

	virtual void render(Scene* scene, VkCommandBuffer commandBuffer, uint32_t bufferIdx, float dt) override;
private:
	struct Vertex {
		glm::vec3 position;
	};

	VkDescriptorPool m_descriptorPool{ VK_NULL_HANDLE };

	std::unique_ptr<EnvironmentCube> m_environmentCube{ nullptr };
};

