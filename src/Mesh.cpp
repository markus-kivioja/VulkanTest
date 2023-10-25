#include "Mesh.h"

#include <fstream>

Mesh::Mesh(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandBuffer copyCommandBuffer, const std::string& filename)
{
    m_vertexBuffer = std::make_unique<Buffer>(physicalDevice, device, copyCommandBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        sizeof(GBufferPass::Vertex) * m_vertices.size(), m_vertices.data());

    m_indexBuffer = std::make_unique<Buffer>(physicalDevice, device, copyCommandBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        sizeof(uint16_t) * m_indices.size(), m_indices.data());

	loadIndexedMesh(filename);
}

Mesh::~Mesh()
{
	for (auto cacheEntry : m_vertexCache)
	{
		delete cacheEntry;
	}
}

uint16_t Mesh::addVertex(uint32_t hash, GBufferPass::Vertex* pVertex)
{
	bool bFoundInList = false;
	uint16_t index = 0;

	if ((uint32_t)m_vertexCache.size() > hash)
	{
		VertexCacheEntry* pEntry = m_vertexCache[hash];
		while (pEntry != NULL)
		{
			GBufferPass::Vertex* pCacheVertex = &(m_vertices[pEntry->index]);

			if (0 == memcmp(pVertex, pCacheVertex, sizeof(GBufferPass::Vertex)))
			{
				bFoundInList = true;
				pCacheVertex->normal += pVertex->normal;
				index = pEntry->index;
				break;
			}

			pEntry = pEntry->pNext;
		}
	}

	if (!bFoundInList)
	{
		index = static_cast<uint16_t>(m_vertices.size());
		m_vertices.emplace_back(*pVertex);

		VertexCacheEntry* pNewEntry = new VertexCacheEntry;
		if (pNewEntry == NULL)
			return (uint16_t)-1;

		pNewEntry->index = index;
		pNewEntry->pNext = NULL;

		while ((uint32_t)m_vertexCache.size() <= hash)
		{
			m_vertexCache.emplace_back(nullptr);
		}

		VertexCacheEntry* pCurEntry = m_vertexCache[hash];
		if (pCurEntry == NULL)
		{
			m_vertexCache[hash] = pNewEntry;
		}
		else
		{
			while (pCurEntry->pNext != NULL)
			{
				pCurEntry = pCurEntry->pNext;
			}

			pCurEntry->pNext = pNewEntry;
		}
	}

	return index;
}

void Mesh::loadIndexedMesh(std::string const& filename)
{
	struct Triangle
	{
		uint16_t indices[3];
	};

	enum ReadingType
	{
		POSITION,
		TEXCOORD,
		NORMAL,
		FACE
	};

	std::ifstream fin;
	char input;

	fin.open(filename);

	if (fin.fail())
		return;

	fin.get(input);
	while (input != 'v')
		fin.get(input);

	std::vector<glm::vec3> positions;
	std::vector<glm::vec2> texCoords;
	std::vector<glm::vec3> normals;

	float x, y, z;

	std::vector<Triangle> triangles;
	Triangle triangle;
	uint16_t index;

	ReadingType reading = POSITION;
	constexpr int indicesPerFace = 3;
	while (fin.good())
	{
		switch (reading)
		{
		case POSITION:
			fin >> x >> y >> z;
			positions.emplace_back(glm::vec3(x, y, z));
			break;
		case TEXCOORD:
			fin >> x >> y;
			texCoords.emplace_back(glm::vec2(x, y));
			break;
		case NORMAL:
			fin >> x >> y >> z;
			normals.emplace_back(glm::vec3(x, y, z));
			break;
		case FACE:
			for (int i = 0; i < indicesPerFace; ++i)
			{
				uint16_t posIndex, uvIndex, normalIndex;
				char slash;
				fin >> posIndex >> slash >> uvIndex >> slash >> normalIndex;

				GBufferPass::Vertex vertex;
				vertex.position = positions[posIndex - 1];
				vertex.uvCoord = texCoords[uvIndex - 1];
				vertex.normal = normals[normalIndex - 1];

				index = addVertex(posIndex, &vertex);
				m_indices.emplace_back(index);

				if (i < 3)
					triangle.indices[i] = index;
				else
					triangle.indices[1] = index;
			}
			triangles.emplace_back(triangle);
			break;
		}

		fin.get(input);
		while (input != 'v' && input != 'f' && fin.good())
		{
			fin.get(input);
		}
		if (input == 'f')
		{
			reading = FACE;
		}
		else
		{
			fin.get(input);
			if (input == 't') reading = TEXCOORD;
			else if (input == 'n') reading = NORMAL;
			else reading = POSITION;
		}
	}

	fin.close();
}

void Mesh::bind(VkCommandBuffer commandBuffer)
{
    VkBuffer vertexBuffers[] = { m_vertexBuffer->m_vkBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->m_vkBuffer, 0, VK_INDEX_TYPE_UINT16);
}