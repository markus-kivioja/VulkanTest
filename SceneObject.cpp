#include "SceneObject.h"

#include "Renderer.h"
#include "Camera.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <iostream>
#include <array>
#include <fstream>

SceneObject::SceneObject(uint32_t id, VkPhysicalDevice physicalDevice, VkDevice device, VkCommandBuffer copyCommandBuffer,
    VkPipelineLayout pipelineLayout, VkDescriptorSetAllocateInfo descriptorSetAllocInfo) :
    m_pipelineLayout(pipelineLayout)
    , m_id(id)
{
    loadIndexedMesh(MESH_FILENAME);

	m_vertexBuffer = std::make_unique<Buffer>(physicalDevice, device, copyCommandBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        sizeof(GBufferPass::Vertex) * m_vertices.size(), m_vertices.data());

    m_indexBuffer = std::make_unique<Buffer>(physicalDevice, device, copyCommandBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        sizeof(uint16_t) * m_indices.size(), m_indices.data());

    m_albedoMap = std::make_unique<Texture>(physicalDevice, device, copyCommandBuffer, ALBEDO_FILENAME);

    m_descriptorSets.resize(Renderer::BUFFER_COUNT);
    VkResult result = vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, m_descriptorSets.data());
    if (result != VK_SUCCESS) {
        std::cout << "Failed to allocate descriptor sets" << std::endl;
        std::terminate();
    }
    for (uint32_t i = 0; i < Renderer::BUFFER_COUNT; ++i)
    {
        m_uniformBuffers.push_back(std::make_unique<Buffer>(physicalDevice, device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(GBufferPass::Transforms)));

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uniformBuffers[i]->m_vkBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(GBufferPass::Transforms);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_albedoMap->m_imageView;
        imageInfo.sampler = m_albedoMap->m_sampler;

        VkWriteDescriptorSet uniformBufferDescriptorWrite{};
        uniformBufferDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        uniformBufferDescriptorWrite.dstSet = m_descriptorSets[i];
        uniformBufferDescriptorWrite.dstBinding = 0;
        uniformBufferDescriptorWrite.dstArrayElement = 0;
        uniformBufferDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBufferDescriptorWrite.descriptorCount = 1;
        uniformBufferDescriptorWrite.pBufferInfo = &bufferInfo;

        VkWriteDescriptorSet textureDescriptorWrite{};
        textureDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        textureDescriptorWrite.dstSet = m_descriptorSets[i];
        textureDescriptorWrite.dstBinding = 1;
        textureDescriptorWrite.dstArrayElement = 0;
        textureDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        textureDescriptorWrite.descriptorCount = 1;
        textureDescriptorWrite.pImageInfo = &imageInfo;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{ uniformBufferDescriptorWrite, textureDescriptorWrite };
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

SceneObject::~SceneObject()
{
    for (auto cacheEntry : m_vertexCache)
    {
        delete cacheEntry;
    }
}

uint16_t SceneObject::addVertex(uint32_t hash, GBufferPass::Vertex* pVertex)
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
        m_vertices.push_back(*pVertex);

        VertexCacheEntry* pNewEntry = new VertexCacheEntry;
        if (pNewEntry == NULL)
            return (uint16_t)-1;

        pNewEntry->index = index;
        pNewEntry->pNext = NULL;

        while ((uint32_t)m_vertexCache.size() <= hash)
        {
            m_vertexCache.push_back(nullptr);
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

void SceneObject::loadIndexedMesh(std::string const& filename)
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
			positions.push_back(glm::vec3(x, y, z));
			break;
		case TEXCOORD:
			fin >> x >> y;
			texCoords.push_back(glm::vec2(x, y));
			break;
		case NORMAL:
			fin >> x >> y >> z;
			normals.push_back(glm::vec3(x, y, z));
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
				m_indices.push_back(index);

				if (i < 3)
					triangle.indices[i] = index;
				else
					triangle.indices[1] = index;
			}
			triangles.push_back(triangle);
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

void SceneObject::render(VkCommandBuffer commandBuffer, uint32_t buffedIdx, Camera* camera, float dt)
{
    GBufferPass::Transforms transforms{};
    m_orientation += dt * m_rotationSpeed;
    constexpr float scale = 0.1f;
    auto translation = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f + m_id * 1.0f, 0.0f, m_id * 0.2f));
    auto rotationY = glm::rotate(translation, m_orientation, glm::vec3(0.0f, 1.0f, 0.0f));
    auto rotationZ = glm::rotate(rotationY, static_cast<float>(-M_PI) * 0.5f, glm::vec3(0.0f, 0.0f, 1.0f));
    transforms.model = glm::scale(rotationZ, glm::vec3(scale, scale, scale));
    transforms.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    transforms.projection = glm::perspective(glm::radians(45.0f), Renderer::WINDOW_WIDTH / static_cast<float>(Renderer::WINDOW_HEIGHT), 0.1f, 10.0f);
    transforms.projection[1][1] *= -1;
    m_uniformBuffers[buffedIdx]->update(&transforms, sizeof(GBufferPass::Transforms));

    VkBuffer vertexBuffers[] = { m_vertexBuffer->m_vkBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->m_vkBuffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[buffedIdx], 0, nullptr);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
}