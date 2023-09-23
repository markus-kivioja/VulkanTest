#include "Camera.h"

#include "Renderer.h"
#include "GBufferPass.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>

Camera::Camera(Type type, VkPhysicalDevice physicalDevice, VkDevice device, VkDescriptorSetAllocateInfo descSetAllocInfo) :
    m_type(type)
{
    m_descriptorSets.resize(Renderer::BUFFER_COUNT);
    VkResult result = vkAllocateDescriptorSets(device, &descSetAllocInfo, m_descriptorSets.data());
    if (result != VK_SUCCESS) {
        std::cout << "Failed to allocate descriptor sets" << std::endl;
        std::terminate();
    }
    
    GBufferPass::CameraTransforms cameraTransforms{};
    if (m_type == NORMAL)
    {
        const float aspectRatio = Renderer::WINDOW_WIDTH / static_cast<float>(Renderer::WINDOW_HEIGHT);
        cameraTransforms.projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
        m_position = glm::vec3(-2.2f, 0.7f, 4.2f);
        m_direction = -glm::vec3(-3.0f, 0.7f, 3.0f);
    }
    else if (m_type == LIGHT)
    {
        constexpr float HALF_WIDTH = 2.2f;
        cameraTransforms.projection = glm::ortho(-HALF_WIDTH, HALF_WIDTH, -HALF_WIDTH, HALF_WIDTH, -9.0f, 20.0f);
        m_position = glm::vec3(1.0f, 5.0f, 8.0f);
        m_direction = glm::vec3(-0.1f, -0.7f, -1.0f);
    }
    cameraTransforms.projection[1][1] *= -1;
    cameraTransforms.view = glm::lookAt(m_position, m_position + m_direction, glm::vec3(0.0f, 1.0f, 0.0f));
    m_view = cameraTransforms.view;
    m_projection = cameraTransforms.projection;

    for (uint32_t i = 0; i < Renderer::BUFFER_COUNT; ++i)
    {
        m_uniformBuffers.emplace_back(std::make_unique<Buffer>(physicalDevice, device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(GBufferPass::CameraTransforms)));
        VkDescriptorBufferInfo uniformBufferInfo{};
        uniformBufferInfo.buffer = m_uniformBuffers[i]->m_vkBuffer;
        uniformBufferInfo.offset = 0;
        uniformBufferInfo.range = sizeof(GBufferPass::CameraTransforms);

        VkWriteDescriptorSet uniformBufferDescriptorWrite{};
        uniformBufferDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        uniformBufferDescriptorWrite.dstSet = m_descriptorSets[i];
        uniformBufferDescriptorWrite.dstBinding = 0;
        uniformBufferDescriptorWrite.dstArrayElement = 0;
        uniformBufferDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBufferDescriptorWrite.descriptorCount = 1;
        uniformBufferDescriptorWrite.pBufferInfo = &uniformBufferInfo;

        std::array<VkWriteDescriptorSet, 1> descriptorWrites{ uniformBufferDescriptorWrite };
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

        m_uniformBuffers[i]->update(&cameraTransforms, sizeof(GBufferPass::CameraTransforms));
    }
}

void Camera::bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t bufferIdx, float dt)
{
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &m_descriptorSets[bufferIdx], 0, nullptr);
}

void Camera::turn(glm::vec2 velocity, uint32_t bufferIdx)
{
    GBufferPass::CameraTransforms cameraTransforms{};
    m_direction.x -= velocity.x * 0.01f;
    m_direction.y += velocity.y * 0.01f;
    m_view = glm::lookAt(m_position, m_position + m_direction, glm::vec3(0.0f, 1.0f, 0.0f));
    cameraTransforms.view = m_view;
    cameraTransforms.projection = m_projection;
    m_uniformBuffers[bufferIdx]->update(&cameraTransforms, sizeof(GBufferPass::CameraTransforms));
}