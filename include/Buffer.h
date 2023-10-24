#pragma once

#include <vulkan/vulkan.h>

class SceneObject;
class Camera;
class LightingPass;

class Buffer
{
public:
	Buffer(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandBuffer copyCommandBuffer, VkBufferUsageFlags usage, VkDeviceSize size, void* initData);
	Buffer(VkPhysicalDevice physicalDevice, VkDevice device, VkBufferUsageFlags usage, VkDeviceSize size);
	~Buffer();

	void update(void* data, size_t offset, size_t size);
private:
	friend SceneObject;
	friend Camera;
	friend LightingPass;

	VkDevice m_device{ VK_NULL_HANDLE };

	VkBuffer m_stagingBuffer{ VK_NULL_HANDLE };
	VkDeviceMemory m_stagingMemory{ VK_NULL_HANDLE };

	VkBuffer m_vkBuffer{ VK_NULL_HANDLE };
	VkDeviceMemory m_deviceMemory{ VK_NULL_HANDLE };

	uint8_t* m_hostData{ nullptr };
};