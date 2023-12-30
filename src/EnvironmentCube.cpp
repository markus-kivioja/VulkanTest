#include "EnvironmentCube.h"

#include "Mickey.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

EnvironmentCube::EnvironmentCube(uint32_t id, Transforms* transforms, Mesh* mesh, Material* material) :
    SceneObject::SceneObject(id, transforms, mesh, material)
{
}

void EnvironmentCube::update(float dt, uint32_t bufferIdx)
{
    constexpr float scale = 50.0f;
    auto transform = glm::scale(glm::mat4(1.0f), glm::vec3(scale, scale, scale));

    m_transforms->update(m_id, transform, bufferIdx);
}