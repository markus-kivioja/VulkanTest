#include "Scene.h"

#include "Renderer.h"

#include <iostream>
#include <array>

Scene::Scene(RenderPass* renderPass, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, uint32_t queueFamilyIdx) :
    m_vkDevice(device)
{
    static constexpr uint32_t OBJECT_COUNT = 4;

    VkDescriptorPoolSize uniformBufferPoolSize{};
    uniformBufferPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformBufferPoolSize.descriptorCount = OBJECT_COUNT * 2 * static_cast<uint32_t>(Renderer::BUFFER_COUNT);
    VkDescriptorPoolSize texturePoolSize{};
    texturePoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texturePoolSize.descriptorCount = OBJECT_COUNT * static_cast<uint32_t>(Renderer::BUFFER_COUNT);

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    std::array<VkDescriptorPoolSize, 2> poolSizes{ uniformBufferPoolSize, texturePoolSize };
    descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
    descriptorPoolCreateInfo.maxSets = OBJECT_COUNT * 2 * static_cast<uint32_t>(Renderer::BUFFER_COUNT);

    VkResult result = vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &m_descriptorPool);
    if (result != VK_SUCCESS)
    {
        std::cout << "Failed to create descriptor pool" << std::endl;
        std::terminate();
    }

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
    for (int i = 0; i < OBJECT_COUNT; ++i)
    {
        m_objects.push_back(std::make_unique<SceneObject>(i, physicalDevice, device, copyCommandBuffer, modelDescSetAllocInfo));
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
    m_cameras.clear();
	m_objects.clear();
    vkDestroyDescriptorPool(m_vkDevice, m_descriptorPool, nullptr);
}

void Scene::render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, Camera::Type cameraType, uint32_t bufferIdx, float dt)
{
    m_cameras[cameraType]->bind(commandBuffer, pipelineLayout, bufferIdx, dt);

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

void Scene::keyPressed(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
        case GLFW_KEY_W:
            m_cameras[Camera::Type::NORMAL]->moveForward();
            break;
        case GLFW_KEY_S:
            m_cameras[Camera::Type::NORMAL]->moveBackward();
            break;
        case GLFW_KEY_A:
            m_cameras[Camera::Type::NORMAL]->moveLeft();
            break;
        case GLFW_KEY_D:
            m_cameras[Camera::Type::NORMAL]->moveRight();
            break;
        default:
            break;
        }
    }
}