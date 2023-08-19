#pragma once

#include "Renderer.h"

#include <vulkan/vulkan.h>

#include <iostream>
#include <vector>
#include <functional>
#include <thread>
#include <queue>
#include <condition_variable>

class RenderThreadPool
{
public:
	struct RenderJob
	{
		std::function<void(VkCommandBuffer)> job;
		std::vector<VkSemaphore> signalSemaphores;
		std::vector<VkSemaphore> waitSemaphores;
		std::vector<Renderer::HostSemaphore*> hostSignals;
		std::vector<Renderer::HostSemaphore*> hostWaits;
		VkFence fence{ VK_NULL_HANDLE };
		uint32_t bufferIdx{ 0 };
	};

	RenderThreadPool(VkDevice device, uint32_t queueFamilyIdx, size_t threadCount);

	void clean();

	void addJob(RenderJob job);
private:
	std::vector<std::thread> m_renderThreads;
	std::queue<RenderJob> m_jobs;

	std::condition_variable m_conditionVariable;
	std::mutex m_mutex;

	bool m_isRunning{ true };
};

