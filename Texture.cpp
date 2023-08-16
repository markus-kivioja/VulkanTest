#include "Texture.h"

#include "RenderPass.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>

Texture::Texture(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandBuffer copyCommandBuffer, std::string const& filename) :
	m_vkDevice(device)
{
	int width{ 0 };
	int height{ 0 }; 
	int texChannels{ 0 };
	stbi_uc* data = stbi_load(filename.c_str(), &width, &height, &texChannels, STBI_rgb_alpha);
	VkDeviceSize size = static_cast<VkDeviceSize>(width) * height * 4;
	if (!data)
	{
		std::cout << "Failed to load image " << filename << std::endl;
		std::terminate();
	}

	// The staging buffer
	VkBufferCreateInfo stagingBufferInfo{};
	stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingBufferInfo.size = size;
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VkResult result = vkCreateBuffer(m_vkDevice, &stagingBufferInfo, nullptr, &m_stagingBuffer);
	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to create staging buffer" << std::endl;
		std::terminate();
	}
	VkMemoryRequirements stagingMemRequirements;
	vkGetBufferMemoryRequirements(m_vkDevice, m_stagingBuffer, &stagingMemRequirements);
	VkPhysicalDeviceMemoryProperties stagingMemProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &stagingMemProperties);
	VkMemoryPropertyFlags stagingProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	uint32_t stagingMemTypeIdx = 0;
	for (; stagingMemTypeIdx < stagingMemProperties.memoryTypeCount; stagingMemTypeIdx++)
	{
		if ((stagingMemRequirements.memoryTypeBits & (1 << stagingMemTypeIdx)) && (stagingMemProperties.memoryTypes[stagingMemTypeIdx].propertyFlags & stagingProperties) == stagingProperties) {
			break;
		}
	}
	VkMemoryAllocateInfo stagingMemAllocInfo{};
	stagingMemAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	stagingMemAllocInfo.allocationSize = stagingMemRequirements.size;
	stagingMemAllocInfo.memoryTypeIndex = stagingMemTypeIdx;
	result = vkAllocateMemory(m_vkDevice, &stagingMemAllocInfo, nullptr, &m_stagingMemory);
	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to allocate staging buffer memory" << std::endl;
		std::terminate();
	}
	vkBindBufferMemory(m_vkDevice, m_stagingBuffer, m_stagingMemory, 0);
	void* mappedData{ nullptr };
	vkMapMemory(m_vkDevice, m_stagingMemory, 0, stagingBufferInfo.size, 0, &mappedData);
	memcpy(mappedData, data, static_cast<size_t>(size));
	vkUnmapMemory(m_vkDevice, m_stagingMemory);
	stbi_image_free(data);

	// The image
	VkImageCreateInfo imageCreateInfo{};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	result = vkCreateImage(m_vkDevice, &imageCreateInfo, nullptr, &m_image);
	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to create image" << std::endl;
		std::terminate();
	}
	VkMemoryRequirements imageMemRequirements;
	vkGetImageMemoryRequirements(m_vkDevice, m_image, &imageMemRequirements);
	VkPhysicalDeviceMemoryProperties imageMemProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &imageMemProperties);
	VkMemoryPropertyFlags imageProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	uint32_t imageMemTypeIdx = 0;
	for (; imageMemTypeIdx < imageMemProperties.memoryTypeCount; imageMemTypeIdx++)
	{
		if ((imageMemRequirements.memoryTypeBits & (1 << imageMemTypeIdx)) && (imageMemProperties.memoryTypes[imageMemTypeIdx].propertyFlags & imageProperties) == imageProperties) {
			break;
		}
	}
	VkMemoryAllocateInfo imageAllocInfo{};
	imageAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	imageAllocInfo.allocationSize = imageMemRequirements.size;
	imageAllocInfo.memoryTypeIndex = imageProperties;
	result = vkAllocateMemory(m_vkDevice, &imageAllocInfo, nullptr, &m_deviceMemory);
	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to allocate texture memory" << std::endl;
		std::terminate();
	}
	vkBindImageMemory(m_vkDevice, m_image, m_deviceMemory, 0);

	addBarrier(copyCommandBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
	vkCmdCopyBufferToImage(copyCommandBuffer, m_stagingBuffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	addBarrier(copyCommandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	result = vkCreateImageView(m_vkDevice, &viewInfo, nullptr, &m_imageView);
	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to create texture image view" << std::endl;
		std::terminate();
	}

	VkSamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.anisotropyEnable = VK_TRUE;
	VkPhysicalDeviceProperties physicalDeviceProperties{};
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
	samplerCreateInfo.maxAnisotropy = physicalDeviceProperties.limits.maxSamplerAnisotropy;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
	samplerCreateInfo.compareEnable = VK_FALSE;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	result = vkCreateSampler(m_vkDevice, &samplerCreateInfo, nullptr, &m_sampler);
	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to create texture sampler!" << std::endl;
		std::terminate();
	}
}

Texture::Texture(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage) :
	m_width(width)
	, m_height(height)
	, m_vkDevice(device)
	, m_format(format)
{
	VkImageCreateInfo imageCreateInfo{};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.format = format;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = usage;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VkResult result = vkCreateImage(m_vkDevice, &imageCreateInfo, nullptr, &m_image);
	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to create image" << std::endl;
		std::terminate();
	}
	VkMemoryRequirements imageMemRequirements;
	vkGetImageMemoryRequirements(m_vkDevice, m_image, &imageMemRequirements);
	VkPhysicalDeviceMemoryProperties imageMemProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &imageMemProperties);
	VkMemoryPropertyFlags imageProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	uint32_t imageMemTypeIdx = 0;
	for (; imageMemTypeIdx < imageMemProperties.memoryTypeCount; imageMemTypeIdx++)
	{
		if ((imageMemRequirements.memoryTypeBits & (1 << imageMemTypeIdx)) && (imageMemProperties.memoryTypes[imageMemTypeIdx].propertyFlags & imageProperties) == imageProperties) {
			break;
		}
	}
	VkMemoryAllocateInfo imageAllocInfo{};
	imageAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	imageAllocInfo.allocationSize = imageMemRequirements.size;
	imageAllocInfo.memoryTypeIndex = imageProperties;
	result = vkAllocateMemory(m_vkDevice, &imageAllocInfo, nullptr, &m_deviceMemory);
	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to allocate texture memory" << std::endl;
		std::terminate();
	}
	vkBindImageMemory(m_vkDevice, m_image, m_deviceMemory, 0);

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = (format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	result = vkCreateImageView(m_vkDevice, &viewInfo, nullptr, &m_imageView);
	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to create texture image view" << std::endl;
		std::terminate();
	}

	if (usage & VK_IMAGE_USAGE_SAMPLED_BIT)
	{
		VkSamplerCreateInfo samplerCreateInfo{};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.anisotropyEnable = VK_TRUE;
		VkPhysicalDeviceProperties physicalDeviceProperties{};
		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
		samplerCreateInfo.maxAnisotropy = physicalDeviceProperties.limits.maxSamplerAnisotropy;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
		samplerCreateInfo.compareEnable = VK_FALSE;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		result = vkCreateSampler(m_vkDevice, &samplerCreateInfo, nullptr, &m_sampler);
		if (result != VK_SUCCESS)
		{
			std::cout << "Failed to create texture sampler!" << std::endl;
			std::terminate();
		}
	}
}

Texture::Texture(VkDevice device, uint32_t width, uint32_t height, VkFormat format, VkImage image) :
	m_width(width)
	, m_height(height)
	, m_vkDevice(device)
	, m_format(format)
{
	VkImageViewCreateInfo imageViewCreateInfo{};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.image = image;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = format;
	imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;

	VkResult result = vkCreateImageView(m_vkDevice, &imageViewCreateInfo, nullptr, &m_imageView);
	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to create image view" << std::endl;
		std::terminate();
	}
}

Texture::~Texture()
{
	if (m_sampler != VK_NULL_HANDLE)
	{
		vkDestroySampler(m_vkDevice, m_sampler, nullptr);
	}
	if (m_stagingBuffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(m_vkDevice, m_stagingBuffer, nullptr);
		vkFreeMemory(m_vkDevice, m_stagingMemory, nullptr);
	}
	if (m_deviceMemory != VK_NULL_HANDLE)
	{
		vkDestroyImage(m_vkDevice, m_image, nullptr);
		vkFreeMemory(m_vkDevice, m_deviceMemory, nullptr);
	}
	if (m_imageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(m_vkDevice, m_imageView, nullptr);
		m_imageView = VK_NULL_HANDLE;
	}
}

void Texture::addBarrier(VkCommandBuffer commandBuffer, VkImageLayout prevLayout, VkImageLayout nextLayout)
{
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = prevLayout;
	barrier.newLayout = nextLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = m_image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	VkPipelineStageFlags srcStageMask;
	VkPipelineStageFlags dstStageMask;
	if (prevLayout == VK_IMAGE_LAYOUT_UNDEFINED && nextLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (prevLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && nextLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (prevLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && nextLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

		srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	}
	else {
		std::cout << "Failed to add texture barrier" << std::endl;
		std::terminate();
	}
	vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}