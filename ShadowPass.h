#pragma once

#include "RenderPass.h"

#include <memory>

class Texture;

class ShadowPass : public RenderPass
{
public:
	static constexpr uint32_t MAP_WIDTH = 2048;
	static constexpr uint32_t MAP_HEIGHT = 2048;

	ShadowPass(VkPhysicalDevice physicalDevice, VkDevice device, std::vector<Texture*>& depthTargets);

	virtual void render(Scene* scene, VkCommandBuffer commandBuffer, uint32_t bufferIdx, float dt) override;
private:
};

