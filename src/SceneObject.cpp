#include "SceneObject.h"

#include "Renderer.h"
#include "Camera.h"

#include <chrono>
#include <iostream>
#include <array>

SceneObject::SceneObject(uint32_t id, Transforms* transforms, Mesh* mesh, Material* material) :
    m_id(id)
    , m_transforms(transforms)
    , m_mesh(mesh)
    , m_material(material)
{
}

void SceneObject::render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t bufferIdx, float dt)
{
	m_transforms->bind(commandBuffer, pipelineLayout, bufferIdx, m_id);
	m_material->bind(commandBuffer, pipelineLayout);
	m_mesh->bind(commandBuffer);

    vkCmdDrawIndexed(commandBuffer, m_mesh->getVertexCount(), 1, 0, 0, 0);
}