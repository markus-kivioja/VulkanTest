#pragma once

#include "Buffer.h"
#include "GBufferPass.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <memory>
#include <string>

class Mesh
{
public:
	Mesh(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandBuffer copyCommandBuffer, const std::string& filename);
	~Mesh();

	void bind(VkCommandBuffer commandBuffer);

	uint32_t getVertexCount() const { return static_cast<uint32_t>(m_indices.size()); };
private:
	uint16_t addVertex(uint32_t hash, GBufferPass::Vertex* pVertex);
	void loadIndexedMesh(std::string const& filename);

	struct VertexCacheEntry
	{
		uint32_t index;
		VertexCacheEntry* pNext;
	};

	std::vector<VertexCacheEntry*> m_vertexCache;

	std::unique_ptr<Buffer> m_vertexBuffer;
	std::unique_ptr<Buffer> m_indexBuffer;

	std::vector<GBufferPass::Vertex> m_vertices;
	std::vector<uint16_t> m_indices;
};