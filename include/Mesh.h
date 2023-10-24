#pragma once

#include <vector>
#include <memory>

class Mesh
{
public:
private:
	struct VertexCacheEntry
	{
		uint32_t index;
		VertexCacheEntry* pNext;
	};

	std::vector<VertexCacheEntry*> m_vertexCache;

	std::unique_ptr<Buffer> m_vertexBuffer;
	std::unique_ptr<Buffer> m_indexBuffer;
};