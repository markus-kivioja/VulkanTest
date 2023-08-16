#pragma once

#include <vulkan/vulkan.h>

#include <string>

class GBufferPass;
class LightingPass;
class SceneObject;
class Renderer;

class Texture
{
public:
	Texture(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandBuffer copyCommandBuffer, std::string const& filename);
	Texture(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage);
	Texture(VkDevice device, uint32_t width, uint32_t height, VkFormat format, VkImage image);
	~Texture();

private:
	friend GBufferPass;
	friend LightingPass;
	friend SceneObject;
	friend Renderer;

	void addBarrier(VkCommandBuffer commandBuffer, VkImageLayout prevLayout, VkImageLayout nextLayout);

	uint32_t m_width{ 0 };
	uint32_t m_height{ 0 };

	VkDevice m_vkDevice{ VK_NULL_HANDLE };
	VkImage m_image{ VK_NULL_HANDLE };
	VkDeviceMemory m_deviceMemory{ VK_NULL_HANDLE };
	VkImageView m_imageView{ VK_NULL_HANDLE };
	VkFormat m_format{ VK_FORMAT_UNDEFINED };
	VkSampler m_sampler{ VK_NULL_HANDLE };

	VkBuffer m_stagingBuffer{ VK_NULL_HANDLE };
	VkDeviceMemory m_stagingMemory{ VK_NULL_HANDLE };
};

