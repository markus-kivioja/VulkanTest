#include "Scene.h"

#include "Renderer.h"

#include <iostream>
#include <array>

Scene::Scene(RenderPass* renderPass, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, uint32_t queueFamilyIdx) :
    m_vkDevice(device)
{
    m_camera = std::make_unique<Camera>();

    static constexpr uint32_t OBJECT_COUNT = 4;

    VkDescriptorPoolSize uniformBufferPoolSize{};
    uniformBufferPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformBufferPoolSize.descriptorCount = OBJECT_COUNT * static_cast<uint32_t>(Renderer::BUFFER_COUNT);
    VkDescriptorPoolSize texturePoolSize{};
    texturePoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texturePoolSize.descriptorCount = OBJECT_COUNT * static_cast<uint32_t>(Renderer::BUFFER_COUNT);

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    std::array<VkDescriptorPoolSize, 2> poolSizes{ uniformBufferPoolSize, texturePoolSize };
    descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
    descriptorPoolCreateInfo.maxSets = OBJECT_COUNT * static_cast<uint32_t>(Renderer::BUFFER_COUNT);

    VkResult result = vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &m_descriptorPool);
    if (result != VK_SUCCESS)
    {
        std::cout << "Failed to create descriptor pool" << std::endl;
        std::terminate();
    }

    std::vector<VkDescriptorSetLayout> layouts(Renderer::BUFFER_COUNT, renderPass->m_descriptorSetLayout);
    VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
    descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocInfo.descriptorPool = m_descriptorPool;
    descriptorSetAllocInfo.descriptorSetCount = static_cast<uint32_t>(Renderer::BUFFER_COUNT);
    descriptorSetAllocInfo.pSetLayouts = layouts.data();

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

    for (int i = 0; i < OBJECT_COUNT; ++i)
    {
        m_objects.push_back(std::make_unique<SceneObject>(i, physicalDevice, device, copyCommandBuffer, renderPass->m_pipelineLayout, descriptorSetAllocInfo));
    }

    vkEndCommandBuffer(copyCommandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &copyCommandBuffer;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, copyCommandPool, 1, &copyCommandBuffer);
    vkDestroyCommandPool(device, copyCommandPool, nullptr);
}

void Scene::clean()
{
	m_objects.clear();
    vkDestroyDescriptorPool(m_vkDevice, m_descriptorPool, nullptr);
}

void Scene::render(VkCommandBuffer commandBuffer, uint32_t buffedIdx)
{
	for (auto const& obj : m_objects)
	{
		obj->render(commandBuffer, buffedIdx, m_camera.get());
	}
}

void Scene::keyPressed(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
        case GLFW_KEY_W:
            m_camera->moveForward();
            break;
        case GLFW_KEY_S:
            m_camera->moveBackward();
            break;
        case GLFW_KEY_A:
            m_camera->moveLeft();
            break;
        case GLFW_KEY_D:
            m_camera->moveRight();
            break;
        default:
            break;
        }
    }
}