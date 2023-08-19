#pragma once

#include "Texture.h"

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec3.hpp>

#include <cstdint>
#include <vector>
#include <memory>
#include <condition_variable>

struct GLFWwindow;

class Scene;
class RenderPass;
class RenderThreadPool;

class Renderer
{
public:
	static constexpr uint32_t BUFFER_COUNT = 2;

	static constexpr uint32_t WINDOW_WIDTH = 1024;
	static constexpr uint32_t WINDOW_HEIGHT = 768;

	struct SpotLight
	{
		glm::vec3 position;
		glm::vec3 direction;
	};

	struct HostSemaphore
	{
		bool signaled{ false };
		std::condition_variable cv;
		std::mutex mutex;
	};

	Renderer();
	~Renderer();
	void render();

private:
	void initVulkan();
	void beginFrame();
	void endFrame();

	GLFWwindow* m_window;

	VkInstance m_vkInstance{ VK_NULL_HANDLE };
	VkPhysicalDevice m_vkPhysicalDevice{ VK_NULL_HANDLE };
	VkDevice m_vkDevice{ VK_NULL_HANDLE };
	VkQueue m_presentQueue{ VK_NULL_HANDLE };
	VkCommandPool m_vkCommandPool{ VK_NULL_HANDLE };
	std::vector<VkCommandBuffer> m_vkCommandBuffers;
	VkSurfaceKHR m_vkSurface{ VK_NULL_HANDLE };
	VkSwapchainKHR m_vkSwapChain{ VK_NULL_HANDLE };
	std::vector<VkSemaphore> m_vkFrameBufferAvailableSemaphores{ VK_NULL_HANDLE };
	std::vector<VkSemaphore> m_vkRenderFinishedSemaphores{ VK_NULL_HANDLE };
	std::vector<VkSemaphore> m_gBufferPassFinished{ VK_NULL_HANDLE };
	std::vector<VkSemaphore> m_shadowPassFinished{ VK_NULL_HANDLE };
	std::vector<VkFence> m_vkFences{ VK_NULL_HANDLE };

	HostSemaphore m_gBufferPassSubmitted;
	HostSemaphore m_shadowPassSubmitted;
	HostSemaphore m_lightinPassSubmitted;

	std::unique_ptr<Texture> m_gBufferAlbedo;
	std::unique_ptr<Texture> m_gBufferNormal;
	std::unique_ptr<Texture> m_depthBuffer;

	std::vector<std::unique_ptr<Texture>> m_frameBuffers;

	enum RenderPassId
	{
		GBUFFER = 0,
		SHADOW,
		LIGHTING,
		COUNT
	};
	std::vector< std::unique_ptr<RenderPass>> m_renderPasses;

	uint32_t m_bufferIdx{ 0 };
	uint32_t m_frameBufferIdx{ 0 };
	uint32_t m_queueFamilyIdx{ 0 };

	std::unique_ptr<Scene> m_scene;
	SpotLight m_spotLight{};

	uint32_t m_threadCount{ 0 };
	std::unique_ptr<RenderThreadPool> m_renderThreadPool;
};

