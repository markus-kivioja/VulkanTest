#pragma once

#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <vector>
#include <memory>

#include "SceneObject.h"
#include "Camera.h"

struct GLFWwindow;

class SkyPass;
class LightingPass;
class InputHandler;

class Scene
{
public:
	Scene(RenderPass* renderPass, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, uint32_t queueFamilyIdx);

	void clean();

	void update(InputHandler* inputHandler, uint32_t bufferIdx);

	void render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, Camera::Type cameraType, uint32_t bufferIdx, float dt);
private:
	friend SkyPass;
	friend LightingPass;

	VkDescriptorPool m_descriptorPool{ VK_NULL_HANDLE };
	VkDevice m_vkDevice{ VK_NULL_HANDLE };

	std::vector<std::unique_ptr<Camera>> m_cameras;
	std::vector<std::unique_ptr<SceneObject>> m_objects;

	glm::vec3 m_lightDirection;
};

