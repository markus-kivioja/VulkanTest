#include "ShadowPass.h"

#include "Texture.h"

ShadowPass::ShadowPass(VkPhysicalDevice physicalDevice, VkDevice device) :
	RenderPass::RenderPass(device, 0)
{
	m_hasDepthAttachment = true;

	m_depthMap = std::make_unique<Texture>(physicalDevice, m_vkDevice, MAP_WIDTH, MAP_HEIGHT,
		VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
}