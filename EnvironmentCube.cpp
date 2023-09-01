#include "EnvironmentCube.h"

#include "Mickey.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

EnvironmentCube::EnvironmentCube(uint32_t id, VkPhysicalDevice physicalDevice, VkDevice device, VkCommandBuffer copyCommandBuffer,
    VkDescriptorSetAllocateInfo descSetAllocInfo) :
    SceneObject::SceneObject(id, physicalDevice, device, copyCommandBuffer, descSetAllocInfo)
{
    m_vertices = {
        {{-1.0f, -1.0f, 1.0f}},
        {{0.5f, -0.5f, 1.0f}},
        {{0.5f, 0.5f, 1.0f}},
        {{-0.5f, 0.5f, 1.0f}},

        {{1.0f, -1.0f, -1.0f}},
        {{1.0f, 1.0f, -1.0f}},
        {{1.0f, 1.0f, 1.0f}},
        {{1.0f, -1.0f, 1.0}},

        {{-1.0f, -1.0f, -1.0f}},
        {{1.0f, -1.0f, -1.0f}},
        {{1.0f, -1.0f, 1.0f}},
        {{-1.0f, -1.0f, 1.0f}},

        {{-1.0f, -1.0f, -1.0f}},
        {{-1.0f, 1.0f, -1.0f}},
        {{1.0f, 1.0f, -1.0f}},
        {{1.0f, -1.0f, -1.0f}},

        {{-1.0f, -1.0f, -1.0f}},
        {{-1.0f, -1.0f, 1.0f}},
        {{-1.0f, 1.0f, 1.0f}},
        {{-1.0f, 1.0f, -1.0f}},

        {{-1.0f, 1.0f, -1.0f}},
        {{-1.0f, 1.0f, 1.0f}},
        {{1.0f, 1.0f, 1.0f}},
        {{1.0f, 1.0f, -1.0f}}
    };
    m_indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        8, 9, 10, 10, 11, 8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20
    };

    m_albedoMap = std::make_unique<Texture>(physicalDevice, device, copyCommandBuffer, ALBEDO_FILENAMES);

    SceneObject::SceneObject::init();
}

void EnvironmentCube::update(uint32_t bufferIdx, float dt)
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