#pragma once

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

class Texture;
class Scene;

class RenderPass
{
public:
	RenderPass() = delete;
	RenderPass(VkDevice device, uint32_t colorTargetCount);
	virtual ~RenderPass();

	virtual void setFrameBufferIdx(uint32_t frameBufferIdx) {};

	void begin(VkCommandBuffer commandBuffer, uint32_t bufferIdx = 0);
	virtual void render(Scene* scene, VkCommandBuffer commandBuffer, uint32_t bufferIdx, float dt) = 0;
	void end(VkCommandBuffer commandBuffer);

	static std::vector<char> readFile(std::string const& filename);

protected:
	friend Texture;
	friend Scene;

	VkShaderModule createVkShader(std::vector<char> const& code);

	VkDevice m_vkDevice{ VK_NULL_HANDLE };
	VkPipelineLayout m_pipelineLayout{ VK_NULL_HANDLE };
	VkDescriptorSetLayout m_modelSetLayout{ VK_NULL_HANDLE };
	VkDescriptorSetLayout m_cameraSetLayout{ VK_NULL_HANDLE };
	VkPipeline m_pipeline{ VK_NULL_HANDLE };
	VkRenderPass m_vkRenderPass{ VK_NULL_HANDLE };
	std::vector<VkFramebuffer> m_framebuffers;

	uint32_t m_targetWidth{ 0 };
	uint32_t m_targetHeight{ 0 };
	uint32_t m_colorTargetCount{ 1 };
	bool m_hasDepthAttachment{ false };
};

