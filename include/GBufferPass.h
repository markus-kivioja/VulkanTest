#pragma once

#include "RenderPass.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

class GBufferPass : public RenderPass
{
public:
	struct Vertex {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 uvCoord;
	};
	struct CameraTransforms {
		glm::mat4 view;
		glm::mat4 projection;
	};

	GBufferPass(VkDevice device, RenderThreadPool* threadPool, std::vector<Texture*>& colorTargets, Texture* depthTarget);
	virtual ~GBufferPass();

	virtual void renderImpl(Scene* scene, VkCommandBuffer commandBuffer, uint32_t bufferIdx, float dt) override;
private:
};

