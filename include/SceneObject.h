#pragma once

#include "GBufferPass.h"
#include "Buffer.h"
#include "Material.h"
#include "Mesh.h"
#include "Tranforms.h"

#include <vulkan/vulkan.h>

#include <memory>

class Camera;

class SceneObject
{
public:
	SceneObject(uint32_t id, Transforms* transforms, Mesh* mesh, Material* material);

	virtual void update(float dt, uint32_t bufferIdx) = 0;

	void render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t bufferIdx, float dt);

protected:
	uint32_t m_id{ 0 };

	Transforms* m_transforms{ nullptr };
	Mesh* m_mesh{ nullptr };
	Material* m_material{ nullptr };

	float m_rotationSpeed{ 1.0f };
	float m_orientation = 0;
};

