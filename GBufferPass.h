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
	struct Transforms {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 projection;
	};

	GBufferPass(VkDevice device, std::vector<Texture*> colorTargets, Texture* depthTarget);
	virtual ~GBufferPass();
private:
};

