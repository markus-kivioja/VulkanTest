#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <memory>

#include "SceneObject.h"
#include "Camera.h"

struct GLFWwindow;

class Scene
{
public:
	Scene(RenderPass* renderPass, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, uint32_t queueFamilyIdx);

	void clean();

	void render(VkCommandBuffer commandBuffer, uint32_t buffedIdx);

	void keyPressed(GLFWwindow* window, int key, int scancode, int action, int mods);
private:
	VkDescriptorPool m_descriptorPool{ VK_NULL_HANDLE };
	VkDevice m_vkDevice{ VK_NULL_HANDLE };

	std::unique_ptr<Camera> m_camera;
	std::vector<std::unique_ptr<SceneObject>> m_objects;
};

