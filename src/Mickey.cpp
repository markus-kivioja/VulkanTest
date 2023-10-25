#include "Mickey.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define _USE_MATH_DEFINES
#include <math.h>

Mickey::Mickey(uint32_t id, Transforms* transforms, Mesh* mesh, Material* material) :
	SceneObject::SceneObject(id, transforms, mesh, material)
{
}

void Mickey::update(float dt, uint32_t bufferIdx)
{
    m_orientation += dt * m_rotationSpeed;
    constexpr float scale = 0.1f;
    auto translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f + m_id * 0.0f, 0.0f, -1.0f + m_id * 1.0f));
    auto rotationY = glm::rotate(translation, m_orientation, glm::vec3(0.0f, 1.0f, 0.0f));
    auto rotationZ = glm::rotate(rotationY, static_cast<float>(-M_PI) * 0.5f, glm::vec3(0.0f, 0.0f, 1.0f));
    auto transform = glm::scale(rotationZ, glm::vec3(scale, scale, scale));

    m_transforms->update(m_id, transform, bufferIdx);
}