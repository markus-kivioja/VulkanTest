#pragma once

#include "RenderPass.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"

class ImguiPass : public RenderPass
{
public:
	struct InitInfo
	{
		VkInstance instance{ VK_NULL_HANDLE };
		VkPhysicalDevice physicalDevice{ VK_NULL_HANDLE };
		uint32_t queueFamilyIdx{ 0 };
		uint32_t minImageCount{ 0 };
		uint32_t imageCount{ 0 };
		VkQueue queue{ VK_NULL_HANDLE };
	};
	ImguiPass(InitInfo initInfo, VkDevice device, std::vector<Texture*>& colorTargets);
	virtual ~ImguiPass();
	virtual void render(Scene* scene, VkCommandBuffer commandBuffer, uint32_t bufferIdx, float dt) override;
private:
	VkDescriptorPool m_descPool{ VK_NULL_HANDLE };
};

