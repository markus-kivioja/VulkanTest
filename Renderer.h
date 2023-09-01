#pragma once

#include "Texture.h"
#include "RenderThreadPool.h"

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <cstdint>
#include <vector>
#include <array>
#include <memory>
#include <condition_variable>

struct GLFWwindow;

class Scene;
class RenderPass;

class Renderer
{
public:
	static constexpr uint32_t BUFFER_COUNT = 2;

	static constexpr uint32_t WINDOW_WIDTH = 1920;
	static constexpr uint32_t WINDOW_HEIGHT = 1080;

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
	VkSurfaceKHR m_vkSurface{ VK_NULL_HANDLE };
	VkSwapchainKHR m_vkSwapChain{ VK_NULL_HANDLE };

	std::array<RenderThreadPool::RenderJob, BUFFER_COUNT> m_skyJobs;
	std::array<RenderThreadPool::RenderJob, BUFFER_COUNT> m_gBufferJobs;
	std::array<RenderThreadPool::RenderJob, BUFFER_COUNT> m_shadowMapJobs;
	std::array<RenderThreadPool::RenderJob, BUFFER_COUNT> m_lightingJobs;
	std::array<RenderThreadPool::RenderJob, BUFFER_COUNT> m_imguiJobs;

	std::array<VkSemaphore, BUFFER_COUNT> m_frameBufferAvailable{ VK_NULL_HANDLE };
	std::array<VkSemaphore, BUFFER_COUNT> m_gBufferPassFinished{ VK_NULL_HANDLE };
	std::array<VkSemaphore, BUFFER_COUNT> m_shadowPassFinished{ VK_NULL_HANDLE };
	std::array<VkSemaphore, BUFFER_COUNT> m_lightingPassFinished{ VK_NULL_HANDLE };
	std::array<VkSemaphore, BUFFER_COUNT> m_imguiPassFinished{ VK_NULL_HANDLE };
	std::array<VkFence, BUFFER_COUNT> m_vkFences{ VK_NULL_HANDLE };

	RenderThreadPool::HostSemaphore m_gBufferPassSubmitted;
	RenderThreadPool::HostSemaphore m_shadowPassSubmitted;
	RenderThreadPool::HostSemaphore m_lightingPassSubmitted;
	RenderThreadPool::HostSemaphore m_imguiPassSubmitted;

	std::unique_ptr<Texture> m_gBufferAlbedo;
	std::unique_ptr<Texture> m_gBufferNormal;
	std::unique_ptr<Texture> m_depthBuffer;
	std::unique_ptr<Texture> m_shadowMap;

	std::vector<std::unique_ptr<Texture>> m_frameBuffers;

	enum RenderPassId
	{
		SKY = 0,
		GBUFFER,
		SHADOW,
		LIGHTING,
		IMGUI,
		COUNT
	};
	std::vector< std::unique_ptr<RenderPass>> m_renderPasses;

	uint32_t m_bufferIdx{ 0 };
	uint32_t m_frameBufferIdx{ 0 };
	uint32_t m_queueFamilyIdx{ 0 };
	uint32_t m_minImageCount{ 0 };

	std::unique_ptr<Scene> m_scene;

	uint32_t m_threadCount{ 0 };
	std::unique_ptr<RenderThreadPool> m_renderThreadPool;
};

