#pragma once
#include "SceneObject.h"

class Floor : public SceneObject
{
public:
	Floor(uint32_t id, VkPhysicalDevice physicalDevice, VkDevice device, VkCommandBuffer copyCommandBuffer,
		VkDescriptorSetAllocateInfo descSetAllocInfo);

	virtual void update(uint32_t bufferIdx, float dt) override;
private:
	inline static const std::string ALBEDO_FILENAME = "assets/crate.jpg";
};

