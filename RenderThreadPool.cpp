#include "RenderThreadPool.h"

#include "Renderer.h"

#include <array>

RenderThreadPool::RenderThreadPool(VkDevice device, uint32_t queueFamilyIdx, size_t threadCount)
{
    for (size_t i = 0; i < threadCount; ++i)
    {
        m_renderThreads.emplace_back([this, device, queueFamilyIdx, i]()
        {
            // Initialize every thread with its own command pool, command buffer, and queue
            VkCommandPoolCreateInfo commanPoolCreateInfo{};
            commanPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            commanPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            commanPoolCreateInfo.queueFamilyIndex = queueFamilyIdx;
            VkCommandPool commandPool{};
            VkResult result = vkCreateCommandPool(device, &commanPoolCreateInfo, nullptr, &commandPool);
            if (result != VK_SUCCESS)
            {
                std::cout << "Failed to create command pool" << std::endl;
                std::terminate();
            }

            std::array<VkCommandBuffer, Renderer::BUFFER_COUNT> commandBuffers;
            VkCommandBufferAllocateInfo commandBufferAllocInfo{};
            commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            commandBufferAllocInfo.commandPool = commandPool;
            commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            commandBufferAllocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
            result = vkAllocateCommandBuffers(device, &commandBufferAllocInfo, commandBuffers.data());
            if (result != VK_SUCCESS)
            {
                std::cout << "Failed to allocate command buffers" << std::endl;
                std::terminate();
            }

            VkQueue queue{};
            vkGetDeviceQueue(device, queueFamilyIdx, static_cast<uint32_t>(i + 1), &queue);

            // Take render jobs from the job queue and execute them
            while (true)
            {
                RenderJob renderJob;
                {
                    std::unique_lock lock(m_mutex);
                    m_conditionVariable.wait(lock, [this]()
                    {
                        return !m_renderJobs.empty() || !m_isRunning;
                    });
                    if (!m_isRunning)
                    {
                        break;
                    }
                    renderJob = std::move(m_renderJobs.front());
                    m_renderJobs.pop();
                }

                vkResetCommandBuffer(commandBuffers[renderJob.bufferIdx], 0);

                VkCommandBufferBeginInfo beginInfo{};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                beginInfo.flags = 0;
                beginInfo.pInheritanceInfo = nullptr;
                VkResult result = vkBeginCommandBuffer(commandBuffers[renderJob.bufferIdx], &beginInfo);
                if (result != VK_SUCCESS)
                {
                    std::cout << "Failed to begin command buffer" << std::endl;
                    std::terminate();
                }

                renderJob.job(commandBuffers[renderJob.bufferIdx]);

                result = vkEndCommandBuffer(commandBuffers[renderJob.bufferIdx]);
                if (result != VK_SUCCESS) {
                    std::cout << "Failed to end command buffer" << std::endl;
                    std::terminate();
                }
                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.waitSemaphoreCount = static_cast<uint32_t>(renderJob.waitSemaphores.size());
                submitInfo.pWaitSemaphores = renderJob.waitSemaphores.data();
                std::vector<VkPipelineStageFlags> waitStages(renderJob.waitSemaphores.size(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
                submitInfo.pWaitDstStageMask = waitStages.data();
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &commandBuffers[renderJob.bufferIdx];
                submitInfo.signalSemaphoreCount = static_cast<uint32_t>(renderJob.signalSemaphores.size());
                submitInfo.pSignalSemaphores = renderJob.signalSemaphores.data();

                // Must wait for the command buffers that signal Vulkan semaphores to be submitted before submitting a waiting command buffer
                for (auto& hostWait : renderJob.hostWaits)
                {
                    std::unique_lock lock(hostWait->mutex);
                    hostWait->cv.wait(lock, [hostWait]()
                    {
                        return hostWait->signaled;
                    });
                    hostWait->signaled = false;
                }

                result = vkQueueSubmit(queue, 1, &submitInfo, renderJob.fence);
                if (result != VK_SUCCESS)
                {
                    std::cout << "Failed to submit draw command buffer" << std::endl;
                    std::terminate();
                }

                // Tell other threads that the command buffer which signals the Vulkan sempahore has been submitted
                for (auto& hostSignal : renderJob.hostSignals)
                {
                    {
                        std::unique_lock lock(m_mutex);
                        hostSignal->signaled = true;
                    }
                    hostSignal->cv.notify_one();
                }
            }
            vkDestroyCommandPool(device, commandPool, nullptr);
        });
    }
}

void RenderThreadPool::addJob(RenderJob job)
{
    {
        std::unique_lock lock(m_mutex);
        m_renderJobs.emplace(job);
    }
    m_conditionVariable.notify_one();
}

void RenderThreadPool::clean()
{
    {
        std::unique_lock lock(m_mutex);
        m_isRunning = false;
    }
    m_conditionVariable.notify_all();
    for (auto& renderThread : m_renderThreads)
    {
        renderThread.join();
    }
}