#pragma once

#include "Renderer.h"
#include "RenderThreadPool.h"

#include <vulkan/vulkan.h>

#include <string>
#include <vector>
#include <array>

class Texture;
class Scene;
class Renderer;

class RenderPass
{
public:
	RenderPass() = delete;
	RenderPass(VkDevice device, RenderThreadPool* threadPool, uint32_t colorTargetCount);
	virtual ~RenderPass();

	void render(Scene* scene, uint32_t frameBufferIdx, uint32_t bufferIdx, float dt);

	void begin(VkCommandBuffer commandBuffer, uint32_t frameBufferIdx = 0);
	virtual void renderImpl(Scene* scene, VkCommandBuffer commandBuffer, uint32_t bufferIdx, float dt) = 0;
	void end(VkCommandBuffer commandBuffer);

	void dependsOn(RenderPass* renderPass);
	void dependsOn(const std::array<VkSemaphore, Renderer::BUFFER_COUNT>& semaphores);
	void signalCPU(const std::array<VkFence, Renderer::BUFFER_COUNT>& fences);

	static std::vector<char> readFile(std::string const& filename);

protected:
	friend Texture;
	friend Scene;
	friend Renderer;

	VkShaderModule createVkShader(std::vector<char> const& code);

	VkDevice m_vkDevice{ VK_NULL_HANDLE };
	VkPipelineLayout m_pipelineLayout{ VK_NULL_HANDLE };
	VkDescriptorSetLayout m_cameraSetLayout{ VK_NULL_HANDLE };
	VkPipeline m_pipeline{ VK_NULL_HANDLE };
	VkRenderPass m_vkRenderPass{ VK_NULL_HANDLE };
	std::vector<VkFramebuffer> m_framebuffers;

	RenderThreadPool* m_threadPool{ nullptr };
	std::array<RenderThreadPool::RenderJob, Renderer::BUFFER_COUNT> m_renderJobs;

	uint32_t m_targetWidth{ 0 };
	uint32_t m_targetHeight{ 0 };
	uint32_t m_colorTargetCount{ 1 };
	bool m_hasDepthAttachment{ false };

	uint32_t m_frameBufferIdx{ 0 };
};

