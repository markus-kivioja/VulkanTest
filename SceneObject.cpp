#include "SceneObject.h"

#include "Renderer.h"
#include "Camera.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <iostream>
#include <array>

SceneObject::SceneObject(uint32_t id, VkPhysicalDevice physicalDevice, VkDevice device, VkCommandBuffer copyCommandBuffer,
    VkPipelineLayout pipelineLayout, VkDescriptorSetAllocateInfo descriptorSetAllocInfo) :
    m_pipelineLayout(pipelineLayout)
    , m_id(id)
{
    m_vertices = {
        {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},

        {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{0.5f, -0.5f, 0.5}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
        {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},

        {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{-0.5f, -0.5f, 0.5}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},

        {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
        {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}
    };
    m_indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        8, 9, 10, 10, 11, 8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20
    };

	m_vertexBuffer = std::make_unique<Buffer>(physicalDevice, device, copyCommandBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        sizeof(GBufferPass::Vertex) * m_vertices.size(), m_vertices.data());

    m_indexBuffer = std::make_unique<Buffer>(physicalDevice, device, copyCommandBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        sizeof(uint16_t) * m_indices.size(), m_indices.data());

    m_albedoMap = std::make_unique<Texture>(physicalDevice, device, copyCommandBuffer, ALBEDO_FILENAME);

    m_descriptorSets.resize(Renderer::BUFFER_COUNT);
    VkResult result = vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, m_descriptorSets.data());
    if (result != VK_SUCCESS) {
        std::cout << "Failed to allocate descriptor sets" << std::endl;
        std::terminate();
    }
    for (uint32_t i = 0; i < Renderer::BUFFER_COUNT; ++i)
    {
        m_uniformBuffers.push_back(std::make_unique<Buffer>(physicalDevice, device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(GBufferPass::Transforms)));

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uniformBuffers[i]->m_vkBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(GBufferPass::Transforms);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_albedoMap->m_imageView;
        imageInfo.sampler = m_albedoMap->m_sampler;

        VkWriteDescriptorSet uniformBufferDescriptorWrite{};
        uniformBufferDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        uniformBufferDescriptorWrite.dstSet = m_descriptorSets[i];
        uniformBufferDescriptorWrite.dstBinding = 0;
        uniformBufferDescriptorWrite.dstArrayElement = 0;
        uniformBufferDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBufferDescriptorWrite.descriptorCount = 1;
        uniformBufferDescriptorWrite.pBufferInfo = &bufferInfo;

        VkWriteDescriptorSet textureDescriptorWrite{};
        textureDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        textureDescriptorWrite.dstSet = m_descriptorSets[i];
        textureDescriptorWrite.dstBinding = 1;
        textureDescriptorWrite.dstArrayElement = 0;
        textureDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        textureDescriptorWrite.descriptorCount = 1;
        textureDescriptorWrite.pImageInfo = &imageInfo;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{ uniformBufferDescriptorWrite, textureDescriptorWrite };
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

void SceneObject::render(VkCommandBuffer commandBuffer, uint32_t buffedIdx, Camera* camera, float dt)
{
    GBufferPass::Transforms transforms{};
    m_orientation += dt * m_rotationSpeed;
    transforms.model = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f + m_id * 1.0f, 0.0f, m_id * 0.2f)), m_orientation, glm::vec3(0.0f, 0.0f, 1.0f));
    transforms.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    transforms.projection = glm::perspective(glm::radians(45.0f), Renderer::WINDOW_WIDTH / static_cast<float>(Renderer::WINDOW_HEIGHT), 0.1f, 10.0f);
    transforms.projection[1][1] *= -1;
    m_uniformBuffers[buffedIdx]->update(&transforms, sizeof(GBufferPass::Transforms));

    VkBuffer vertexBuffers[] = { m_vertexBuffer->m_vkBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->m_vkBuffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[buffedIdx], 0, nullptr);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
}