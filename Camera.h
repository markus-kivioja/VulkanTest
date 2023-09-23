#pragma once

#include "Buffer.h"

#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <vector>
#include <memory>

class LightingPass;

class Camera
{
public:
	enum Type
	{
		NORMAL = 0,
		LIGHT,
		COUNT
	};

	Camera(Type type, VkPhysicalDevice physicalDevice, VkDevice device, VkDescriptorSetAllocateInfo descSetAllocInfo);

	void bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t bufferIdx, float dt);

	void turn(glm::vec2 velocity, uint32_t bufferIdx);
	void moveForward() { std::cout << "Forward!" << std::endl; };
	void moveBackward() { std::cout << "Backward!" << std::endl; };
	void moveLeft() { std::cout << "Left!" << std::endl; };
	void moveRight() { std::cout << "Right!" << std::endl; };
private:
	friend LightingPass;

	std::vector<std::unique_ptr<Buffer>> m_uniformBuffers;
	std::vector<VkDescriptorSet> m_descriptorSets;
	
	Type m_type{ NORMAL };

	glm::vec3 m_position;
	glm::vec3 m_direction;
	glm::mat4 m_view;
	glm::mat4 m_projection;
};

