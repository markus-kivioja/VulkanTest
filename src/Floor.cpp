#include "Floor.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Floor::Floor(uint32_t id, VkPhysicalDevice physicalDevice, VkDevice device, VkCommandBuffer copyCommandBuffer,
	VkDescriptorSetAllocateInfo descSetAllocInfo) :
	SceneObject::SceneObject(id, physicalDevice, device, copyCommandBuffer, descSetAllocInfo)
{
    m_vertices = {
        {{-0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{-0.5f, 0.0f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 10.0f}},
        {{0.5f, 0.0f, 0.5f}, {0.0f, 1.0f, 0.0f}, {10.0f, 10.0f}},
        {{0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {10.0f, 0.0f}}
    };
    m_indices = {
        0, 1, 2, 2, 3, 0
    };
    m_albedoMap = std::make_unique<Texture>(physicalDevice, device, copyCommandBuffer, std::vector{ ALBEDO_FILENAME });
    SceneObject::SceneObject::init();
}

void Floor::update(uint32_t bufferIdx, float dt)
{
    GBufferPass::ModelTransforms modelTransforms{};
    m_orientation += dt * m_rotationSpeed;
    constexpr float scale = 10.0f;
    auto translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.45f, .0f));
    modelTransforms.model = glm::scale(translation, glm::vec3(scale, scale, scale));
    m_uniformBuffers[bufferIdx]->update(&modelTransforms, sizeof(GBufferPass::ModelTransforms));
}