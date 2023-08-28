#pragma once

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
	struct HostSemaphore
	{
		bool signaled{ false };
		std::condition_variable cv;
		std::mutex mutex;
	};
	struct RenderJob
	{
		std::function<void(VkCommandBuffer)> job;
		std::vector<VkSemaphore> signalSemaphores;
		std::vector<VkSemaphore> waitSemaphores;
		std::vector<HostSemaphore*> hostSignals;
		std::vector<HostSemaphore*> hostWaits;
		VkFence fence{ VK_NULL_HANDLE };
	};

	RenderThreadPool(VkDevice device, uint32_t queueFamilyIdx, size_t threadCount);

	void clean();

	void addJob(RenderJob job);
private:
	static constexpr uint32_t COMMAND_BUFFER_COUNT = 5;

	std::vector<std::thread> m_renderThreads;
	std::queue<RenderJob> m_renderJobs;

	std::condition_variable m_conditionVariable;
	std::mutex m_mutex;

	bool m_isRunning{ true };
};

