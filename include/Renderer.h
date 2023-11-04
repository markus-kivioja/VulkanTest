#pragma once

//#include "ResourceManager.h"
#include "Transforms.h"
#include "RenderThreadPool.h"
#include "Texture.h"

#include <cstdint>
#include <vector>
#include <array>
#include <memory>
#include <condition_variable>

struct GLFWwindow;

class Scene;
class RenderPass;
class InputHandler;

class Renderer
{
public:
	static constexpr uint32_t BUFFER_COUNT = 2;

	static constexpr uint32_t WINDOW_WIDTH = 1920;
	static constexpr uint32_t WINDOW_HEIGHT = 1080;

	Renderer();
	~Renderer();

	void loop();

private:
	enum RenderPassId
	{
		SKY = 0,
		GBUFFER,
		SHADOW,
		LIGHTING,
		IMGUI,
		COUNT
	};

	void initVulkan();
	void beginFrame();
	void endFrame();
	void update();
	void render();

	GLFWwindow* m_window;
	std::unique_ptr<InputHandler> m_inputHandler;

	VkInstance m_vkInstance{ VK_NULL_HANDLE };
	VkPhysicalDevice m_vkPhysicalDevice{ VK_NULL_HANDLE };
	VkDevice m_vkDevice{ VK_NULL_HANDLE };
	VkQueue m_presentQueue{ VK_NULL_HANDLE };
	VkSurfaceKHR m_vkSurface{ VK_NULL_HANDLE };
	VkSwapchainKHR m_vkSwapChain{ VK_NULL_HANDLE };

	std::array<VkSemaphore, BUFFER_COUNT> m_frameBufferAvailable{ VK_NULL_HANDLE };
	std::array<VkFence, BUFFER_COUNT> m_vkFences{ VK_NULL_HANDLE };

	std::unique_ptr<Texture> m_gBufferAlbedo;
	std::unique_ptr<Texture> m_gBufferNormal;
	std::unique_ptr<Texture> m_depthBuffer;
	std::unique_ptr<Texture> m_shadowMap;

	//std::unique_ptr<ResourceManager> m_resourceManager;
	std::unique_ptr<Transforms> m_transforms;
	std::vector<std::unique_ptr<RenderPass>> m_renderPasses;
	std::vector<std::unique_ptr<Texture>> m_frameBuffers;
	std::unique_ptr<Scene> m_scene;
	std::unique_ptr<RenderThreadPool> m_renderThreadPool;

	uint32_t m_bufferIdx{ 0 };
	uint32_t m_frameBufferIdx{ 0 };
	uint32_t m_queueFamilyIdx{ 0 };
	uint32_t m_minImageCount{ 0 };
	uint32_t m_threadCount{ 0 };

	float m_dt{ 0 };
};

