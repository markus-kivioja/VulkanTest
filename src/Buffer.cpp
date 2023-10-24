#include "Buffer.h"

#include <iostream>

Buffer::Buffer(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandBuffer copyCommandBuffer, VkBufferUsageFlags usage, VkDeviceSize size, void* initData) :
	m_device(device)
{
	// The staging buffer
	VkBufferCreateInfo stagingBufferInfo{};
	stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingBufferInfo.size = size;
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkResult result = vkCreateBuffer(m_device, &stagingBufferInfo, nullptr, &m_stagingBuffer);
	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to create staging buffer" << std::endl;
		std::terminate();
	}
	VkMemoryRequirements stagingMemRequirements;
	vkGetBufferMemoryRequirements(m_device, m_stagingBuffer, &stagingMemRequirements);
	VkPhysicalDeviceMemoryProperties stagingMemProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &stagingMemProperties);
	VkMemoryPropertyFlags stagingProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	uint32_t stagingMemTypeIdx = 0;
	for (; stagingMemTypeIdx < stagingMemProperties.memoryTypeCount; stagingMemTypeIdx++)
	{
		if ((stagingMemRequirements.memoryTypeBits & (1 << stagingMemTypeIdx)) && (stagingMemProperties.memoryTypes[stagingMemTypeIdx].propertyFlags & stagingProperties) == stagingProperties)
		{
			break;
		}
	}
	VkMemoryAllocateInfo stagingMemAllocInfo{};
	stagingMemAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	stagingMemAllocInfo.allocationSize = stagingMemRequirements.size;
	stagingMemAllocInfo.memoryTypeIndex = stagingMemTypeIdx;
	result = vkAllocateMemory(m_device, &stagingMemAllocInfo, nullptr, &m_stagingMemory);
	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to allocate staging buffer memory" << std::endl;
		std::terminate();
	}
	vkBindBufferMemory(m_device, m_stagingBuffer, m_stagingMemory, 0);
	void* mappedData{ nullptr };
	vkMapMemory(m_device, m_stagingMemory, 0, stagingBufferInfo.size, 0, &mappedData);
	memcpy(mappedData, initData, static_cast<size_t>(size));
	vkUnmapMemory(m_device, m_stagingMemory);

	
	// Only GPU buffer
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	result = vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_vkBuffer);
	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to create buffer" << std::endl;
		std::terminate();
	}
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(m_device, m_vkBuffer, &memRequirements);
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
	VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	uint32_t memTypeIdx = 0;
	for (; memTypeIdx < memProperties.memoryTypeCount; memTypeIdx++)
	{
		if ((memRequirements.memoryTypeBits & (1 << memTypeIdx)) && (memProperties.memoryTypes[memTypeIdx].propertyFlags & properties) == properties)
		{
			break;
		}
	}
	VkMemoryAllocateInfo memAllocInfo{};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize = memRequirements.size;
	memAllocInfo.memoryTypeIndex = memTypeIdx;
	result = vkAllocateMemory(m_device, &memAllocInfo, nullptr, &m_deviceMemory);
	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to allocate buffer memory" << std::endl;
		std::terminate();
	}
	vkBindBufferMemory(m_device, m_vkBuffer, m_deviceMemory, 0);

	VkBufferCopy region{};
	region.size = size;
	vkCmdCopyBuffer(copyCommandBuffer, m_stagingBuffer, m_vkBuffer, 1, &region);
}

Buffer::Buffer(VkPhysicalDevice physicalDevice, VkDevice device, VkBufferUsageFlags usage, VkDeviceSize size) :
	m_device(device)
{
	// Persistent mapping buffer
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VkResult result = vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_vkBuffer);
	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to create persistent mapping buffer" << std::endl;
		std::terminate();
	}
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(m_device, m_vkBuffer, &memRequirements);
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
	VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	uint32_t memTypeIdx = 0;
	for (; memTypeIdx < memProperties.memoryTypeCount; memTypeIdx++) {
		if ((memRequirements.memoryTypeBits & (1 << memTypeIdx)) && (memProperties.memoryTypes[memTypeIdx].propertyFlags & properties) == properties) {
			break;
		}
	}
	VkMemoryAllocateInfo memAllocInfo{};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize = memRequirements.size;
	memAllocInfo.memoryTypeIndex = memTypeIdx;
	result = vkAllocateMemory(m_device, &memAllocInfo, nullptr, &m_deviceMemory);
	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to allocate persistent mapping buffer memory" << std::endl;
		std::terminate();
	}
	vkBindBufferMemory(m_device, m_vkBuffer, m_deviceMemory, 0);
	vkMapMemory(device, m_deviceMemory, 0, size, 0, &m_hostData);
}

Buffer::~Buffer()
{
	if (m_stagingBuffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(m_device, m_stagingBuffer, nullptr);
		vkFreeMemory(m_device, m_stagingMemory, nullptr);
	}

	vkDestroyBuffer(m_device, m_vkBuffer, nullptr);
	vkFreeMemory(m_device, m_deviceMemory, nullptr);
}

void Buffer::update(void* data, size_t offset, size_t size)
{
	if (m_hostData)
	{
		memcpy(m_hostData + offset, data, size);
	}
	else
	{
		std::cout << "Trying to update buffer that is not persistent mapped" << std::endl;
		std::terminate();
	}
}