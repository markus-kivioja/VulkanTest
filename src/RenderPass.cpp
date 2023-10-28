#include "RenderPass.h"

#include "Texture.h"

#include <fstream>
#include <iostream>
#include <vector>
#include <array>

std::vector<char> RenderPass::readFile(std::string const& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        std::cout << "Failed to open file " << filename << std::endl;
        std::terminate();
    }
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

RenderPass::RenderPass(VkDevice device, RenderThreadPool* threadPool, uint32_t colorTargetCount) :
	m_vkDevice(device)
    , m_threadPool(threadPool)
    , m_colorTargetCount(colorTargetCount)
{
    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (uint32_t bufferIdx{ 0 }; bufferIdx < Renderer::BUFFER_COUNT; ++bufferIdx)
    {
        VkResult result = vkCreateSemaphore(m_vkDevice, &semaphoreCreateInfo, nullptr, &m_renderJobs[bufferIdx].deviceSignal);
        if (result != VK_SUCCESS)
        {
            std::cout << "Failed to create Vulkan semaphore" << std::endl;
            std::terminate();
        }
    }
}

RenderPass::~RenderPass()
{
    if (m_cameraSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_vkDevice, m_cameraSetLayout, nullptr);
        m_cameraSetLayout = VK_NULL_HANDLE;
    }
    if (m_pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_vkDevice, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }
    if (m_pipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(m_vkDevice, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }
    if (m_vkRenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(m_vkDevice, m_vkRenderPass, nullptr);
        m_vkRenderPass = VK_NULL_HANDLE;
    }
    for (auto& framebuffer : m_framebuffers)
    {
        vkDestroyFramebuffer(m_vkDevice, framebuffer, nullptr);
        framebuffer = VK_NULL_HANDLE;
    }
    for (uint32_t i = 0; i < Renderer::BUFFER_COUNT; ++i)
    {
        vkDestroySemaphore(m_vkDevice, m_renderJobs[i].deviceSignal, nullptr);
    }
}

VkShaderModule RenderPass::createVkShader(std::vector<char> const& code)
{
    VkShaderModuleCreateInfo shaderCreateInfo{};
    shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderCreateInfo.codeSize = code.size();
    shaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule shader;
    VkResult result = vkCreateShaderModule(m_vkDevice, &shaderCreateInfo, nullptr, &shader);
    if (result != VK_SUCCESS)
    {
        std::cout << "Failed to create shader module" << std::endl;
        std::terminate();
    }
    return shader;
}

void RenderPass::begin(VkCommandBuffer commandBuffer, uint32_t frameBufferIdx)
{
    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = m_vkRenderPass;
    renderPassBeginInfo.framebuffer = m_framebuffers[frameBufferIdx];
    renderPassBeginInfo.renderArea.offset = { 0, 0 };
    renderPassBeginInfo.renderArea.extent = { m_targetWidth, m_targetHeight };
    std::vector<VkClearValue> clearValues;
    for (uint32_t i = 0; i < m_colorTargetCount; ++i)
    {
        VkClearValue colorValue;
        colorValue.color = { {0.0f, 0.0f, 0.0f, 1.0f} };
        clearValues.emplace_back(colorValue);
    }
    if (m_hasDepthAttachment)
    {
        VkClearValue depthValue;
        depthValue.depthStencil = { 1.0f, 0 };
        clearValues.emplace_back(depthValue);
    }
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    if (m_pipeline)
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    }

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_targetWidth);
    viewport.height = static_cast<float>(m_targetHeight);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = renderPassBeginInfo.renderArea.extent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void RenderPass::end(VkCommandBuffer commandBuffer)
{
    vkCmdEndRenderPass(commandBuffer);
}

void RenderPass::render(Scene* scene, uint32_t frameBufferIdx, uint32_t bufferIdx, float dt)
{
    m_renderJobs[bufferIdx].job = [this, dt, scene, frameBufferIdx, bufferIdx](VkCommandBuffer commandBuffer)
    {
        m_frameBufferIdx = frameBufferIdx;
        renderImpl(scene, commandBuffer, bufferIdx, dt);
    };

    m_threadPool->addJob(&m_renderJobs[bufferIdx]);
}

void RenderPass::dependsOn(RenderPass* renderPass)
{
    for (uint32_t bufferIdx{ 0 }; bufferIdx < Renderer::BUFFER_COUNT; ++bufferIdx)
    {
        m_renderJobs[bufferIdx].deviceWaits.push_back(renderPass->m_renderJobs[bufferIdx].deviceSignal);
        m_renderJobs[bufferIdx].hostWaits.push_back(&renderPass->m_renderJobs[bufferIdx].hostSignal);
    }
}

void RenderPass::dependsOn(const std::array<VkSemaphore, Renderer::BUFFER_COUNT>& semaphores)
{
    for (uint32_t bufferIdx{ 0 }; bufferIdx < Renderer::BUFFER_COUNT; ++bufferIdx)
    {
        m_renderJobs[bufferIdx].deviceWaits.push_back(semaphores[bufferIdx]);
    }
}

void RenderPass::signalCPU(const std::array<VkFence, Renderer::BUFFER_COUNT>& fences)
{
    for (uint32_t bufferIdx{ 0 }; bufferIdx < Renderer::BUFFER_COUNT; ++bufferIdx)
    {
        m_renderJobs[bufferIdx].fence = fences[bufferIdx];
    }
}
