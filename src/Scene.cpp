#include "Scene.h"

#include "Renderer.h"
#include "InputHandler.h"
#include "Mickey.h"
#include "Floor.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"

#include <iostream>
#include <array>

Scene::Scene(RenderPass* renderPass, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, uint32_t queueFamilyIdx) :
    m_vkDevice(device)
{
    static constexpr uint32_t MICKEY_COUNT = 4;
    static constexpr uint32_t OBJECT_COUNT = MICKEY_COUNT + 1;

    std::vector<VkDescriptorSetLayout> modelLayouts(Renderer::BUFFER_COUNT, renderPass->m_modelSetLayout);
    VkDescriptorSetAllocateInfo modelDescSetAllocInfo{};
    modelDescSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    modelDescSetAllocInfo.descriptorPool = m_descriptorPool;
    modelDescSetAllocInfo.descriptorSetCount = static_cast<uint32_t>(Renderer::BUFFER_COUNT);
    modelDescSetAllocInfo.pSetLayouts = modelLayouts.data();

    std::vector<VkDescriptorSetLayout> cameraLayouts(Renderer::BUFFER_COUNT, renderPass->m_cameraSetLayout);
    VkDescriptorSetAllocateInfo cameraDescSetAllocInfo{};
    cameraDescSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    cameraDescSetAllocInfo.descriptorPool = m_descriptorPool;
    cameraDescSetAllocInfo.descriptorSetCount = static_cast<uint32_t>(Renderer::BUFFER_COUNT);
    cameraDescSetAllocInfo.pSetLayouts = cameraLayouts.data();

    VkCommandPoolCreateInfo commanPoolCreateInfo{};
    commanPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commanPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commanPoolCreateInfo.queueFamilyIndex = queueFamilyIdx;
    VkCommandPool copyCommandPool{ VK_NULL_HANDLE };
    result = vkCreateCommandPool(device, &commanPoolCreateInfo, nullptr, &copyCommandPool);
    if (result != VK_SUCCESS)
    {
        std::cout << "Failed to create copy command pool" << std::endl;
        std::terminate();
    }

    VkCommandBufferAllocateInfo commandBufferAllocInfo{};
    commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocInfo.commandPool = copyCommandPool;
    commandBufferAllocInfo.commandBufferCount = 1;

    VkCommandBuffer copyCommandBuffer;
    vkAllocateCommandBuffers(device, &commandBufferAllocInfo, &copyCommandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(copyCommandBuffer, &beginInfo);

    m_cameras.resize(Camera::Type::COUNT);
    m_cameras[Camera::Type::NORMAL] = std::make_unique<Camera>(Camera::Type::NORMAL, physicalDevice, device, cameraDescSetAllocInfo);
    m_cameras[Camera::Type::LIGHT] = std::make_unique<Camera>(Camera::Type::LIGHT, physicalDevice, device, cameraDescSetAllocInfo);
    
    for (int i = 0; i < MICKEY_COUNT; ++i)
    {
        m_objects.emplace_back(std::make_unique<Mickey>(i, physicalDevice, device, copyCommandBuffer, modelDescSetAllocInfo));
    }
    m_objects.emplace_back(std::make_unique<Floor>(OBJECT_COUNT, physicalDevice, device, copyCommandBuffer, modelDescSetAllocInfo));

    ImGui_ImplVulkan_CreateFontsTexture(copyCommandBuffer);

    vkEndCommandBuffer(copyCommandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &copyCommandBuffer;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    ImGui_ImplVulkan_DestroyFontUploadObjects();

    vkFreeCommandBuffers(device, copyCommandPool, 1, &copyCommandBuffer);
    vkDestroyCommandPool(device, copyCommandPool, nullptr);
}

void Scene::clean()
{
    m_cameras.clear();
	m_objects.clear();
    vkDestroyDescriptorPool(m_vkDevice, m_descriptorPool, nullptr);
}

void Scene::update(InputHandler* inputHandler, uint32_t bufferIdx)
{
    glm::vec3 camMoveDir{ 0 };
    InputHandler::KeyState keyState = inputHandler->getKeyState();
    if (keyState.forwardPressed)
    {
        camMoveDir += glm::vec3(0, 0, 1.0f);
    }
    if (keyState.backwardPressed)
    {
        camMoveDir += glm::vec3(0, 0, -1.0f);
    }
    if (keyState.leftPressed)
    {
        camMoveDir += glm::vec3(-1.0f, 0, 0);
    }
    if (keyState.rightPressed)
    {
        camMoveDir += glm::vec3(1.0f, 0, 0);
    }
    if (glm::length(camMoveDir))
    {
        camMoveDir = glm::normalize(camMoveDir);
    }

    m_cameras[Camera::Type::NORMAL]->move(camMoveDir, bufferIdx);
    m_cameras[Camera::Type::NORMAL]->turn(inputHandler->getDragVelocity(), bufferIdx);

    m_cameras[Camera::Type::NORMAL]->update(bufferIdx);
}

void Scene::render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, Camera::Type cameraType, uint32_t bufferIdx, float dt)
{
    m_cameras[cameraType]->bind(commandBuffer, pipelineLayout, bufferIdx);

    if (cameraType == Camera::Type::NORMAL)
    {
        for (auto const& obj : m_objects)
        {
            obj->update(bufferIdx, dt);
        }
    }
	for (auto const& obj : m_objects)
	{
		obj->render(commandBuffer, pipelineLayout, bufferIdx, dt);
	}
}