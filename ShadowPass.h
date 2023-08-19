#pragma once

#include "RenderPass.h"

#include <memory>

class Texture;

class ShadowPass : public RenderPass
{
public:
	static constexpr uint32_t MAP_WIDTH = 2048;
	static constexpr uint32_t MAP_HEIGHT = 2048;

	ShadowPass(VkPhysicalDevice physicalDevice, VkDevice device);
private:
	std::unique_ptr<Texture> m_depthMap;
};

