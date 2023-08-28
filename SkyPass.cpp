#include "SkyPass.h"

SkyPass::SkyPass(VkPhysicalDevice physicalDevice, VkDevice device, std::vector<Texture*>& colorTargets) :
	RenderPass::RenderPass(device, 1)
{
    m_vertices = {
        {{-1.0f, -1.0f, 1.0f}},
        {{0.5f, -0.5f, 1.0f}},
        {{0.5f, 0.5f, 1.0f}},
        {{-0.5f, 0.5f, 1.0f}},

        {{1.0f, -1.0f, -1.0f}},
        {{1.0f, 1.0f, -1.0f}},
        {{1.0f, 1.0f, 1.0f}},
        {{1.0f, -1.0f, 1.0}},

        {{-1.0f, -1.0f, -1.0f}},
        {{1.0f, -1.0f, -1.0f}},
        {{1.0f, -1.0f, 1.0f}},
        {{-1.0f, -1.0f, 1.0f}},

        {{-1.0f, -1.0f, -1.0f}},
        {{-1.0f, 1.0f, -1.0f}},
        {{1.0f, 1.0f, -1.0f}},
        {{1.0f, -1.0f, -1.0f}},

        {{-1.0f, -1.0f, -1.0f}},
        {{-1.0f, -1.0f, 1.0f}},
        {{-1.0f, 1.0f, 1.0f}},
        {{-1.0f, 1.0f, -1.0f}},

        {{-1.0f, 1.0f, -1.0f}},
        {{-1.0f, 1.0f, 1.0f}},
        {{1.0f, 1.0f, 1.0f}},
        {{1.0f, 1.0f, -1.0f}}
    };
    m_indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        8, 9, 10, 10, 11, 8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20
    };
}

void SkyPass::render(Scene* scene, VkCommandBuffer commandBuffer, uint32_t bufferIdx, float dt)
{
    begin(commandBuffer);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[bufferIdx], 0, nullptr);
    vkCmdDraw(commandBuffer, 4, 1, 0, 0);

    end(commandBuffer);
}