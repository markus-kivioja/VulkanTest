#pragma once
#include "SceneObject.h"

class Mickey : public SceneObject
{
public:
	Mickey(uint32_t id, VkPhysicalDevice physicalDevice, VkDevice device, VkCommandBuffer copyCommandBuffer,
		VkDescriptorSetAllocateInfo descSetAllocInfo);

	virtual void update(uint32_t bufferIdx, float dt) override;
private:
	inline static const std::string MESH_FILENAME = "assets/mickey.obj";
	inline static const std::string ALBEDO_FILENAME = "assets/mickey.png";
};

