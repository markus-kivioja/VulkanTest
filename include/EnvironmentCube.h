#pragma once
#include "SceneObject.h"

class EnvironmentCube : public SceneObject
{
public:
	EnvironmentCube(uint32_t id, VkPhysicalDevice physicalDevice, VkDevice device, VkCommandBuffer copyCommandBuffer,
		VkDescriptorSetAllocateInfo descSetAllocInfo);

	virtual void update(uint32_t bufferIdx, float dt) override;
private:
	inline static const std::vector<std::string> ALBEDO_FILENAMES =
	{
		"assets/posx.jpg",
		"assets/negx.jpg",
		"assets/posy.jpg",
		"assets/negy.jpg",
		"assets/posz.jpg",
		"assets/negz.jpg",
	};
};

