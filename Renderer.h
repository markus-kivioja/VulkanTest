#pragma once

#include "Texture.h"

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <cstdint>
#include <vector>
#include <memory>

struct GLFWwindow;

class Scene;
class RenderPass;

class Renderer
{
public:
	static constexpr uint32_t BUFFER_COUNT = 2;

	static constexpr uint32_t WINDOW_WIDTH = 1024;
	static constexpr uint32_t WINDOW_HEIGHT = 768;

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
	VkQueue m_vkQueue{ VK_NULL_HANDLE };
	VkCommandPool m_vkCommandPool{ VK_NULL_HANDLE };
	std::vector<VkCommandBuffer> m_vkCommandBuffers;
	VkSurfaceKHR m_vkSurface{ VK_NULL_HANDLE };
	VkSwapchainKHR m_vkSwapChain{ VK_NULL_HANDLE };
	std::vector<VkSemaphore> m_vkFrameBufferAvailableSemaphores{ VK_NULL_HANDLE };
	std::vector<VkSemaphore> m_vkRenderFinishedSemaphores{ VK_NULL_HANDLE };
	std::vector<VkFence> m_vkFences{ VK_NULL_HANDLE };

	std::unique_ptr<Texture> m_gBufferAlbedo;
	std::unique_ptr<Texture> m_gBufferNormal;
	std::unique_ptr<Texture> m_depthBuffer;

	std::vector<std::unique_ptr<Texture>> m_frameBuffers;

	enum RenderPassId
	{
		GBUFFER = 0,
		LIGHTING,
		COUNT
	};
	std::vector< std::unique_ptr<RenderPass>> m_renderPasses;

	uint32_t m_bufferIdx{ 0 };
	uint32_t m_frameBufferIdx{ 0 };
	uint32_t m_queueFamilyIdx{ 0 };

	std::unique_ptr<Scene> m_scene;
};

