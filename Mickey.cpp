#include "Mickey.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Mickey::Mickey(uint32_t id, VkPhysicalDevice physicalDevice, VkDevice device, VkCommandBuffer copyCommandBuffer,
	VkDescriptorSetAllocateInfo descSetAllocInfo) :
	SceneObject::SceneObject(id, physicalDevice, device, copyCommandBuffer, descSetAllocInfo)
{
	loadIndexedMesh(MESH_FILENAME);
    m_albedoMap = std::make_unique<Texture>(physicalDevice, device, copyCommandBuffer, ALBEDO_FILENAME);
    SceneObject::SceneObject::init();
}

void Mickey::update(uint32_t bufferIdx, float dt)
{
    GBufferPass::ModelTransforms modelTransforms{};
    m_orientation += dt * m_rotationSpeed;
    constexpr float scale = 0.1f;
    auto translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f + m_id * 0.0f, 0.0f, -1.0f + m_id * 1.0f));
    auto rotationY = glm::rotate(translation, m_orientation, glm::vec3(0.0f, 1.0f, 0.0f));
    auto rotationZ = glm::rotate(rotationY, static_cast<float>(-M_PI) * 0.5f, glm::vec3(0.0f, 0.0f, 1.0f));
    modelTransforms.model = glm::scale(rotationZ, glm::vec3(scale, scale, scale));
    m_uniformBuffers[bufferIdx]->update(&modelTransforms, sizeof(GBufferPass::ModelTransforms));
}