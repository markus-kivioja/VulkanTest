#include "ImguiPass.h"

#include "Texture.h"
#include "Scene.h"

#include <iostream>
#include <array>

ImguiPass::ImguiPass(InitInfo initInfo, VkDevice device, RenderThreadPool* threadPool, std::vector<Texture*>& colorTargets) :
    ImguiPass::RenderPass(device, threadPool, 1)
{
    m_hasDepthAttachment = false;

    VkAttachmentDescription attachment = {};
    attachment.format = colorTargets[0]->m_format;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference color_attachment = {};
    color_attachment.attachment = 0;
    color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment;
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &attachment;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &dependency;
    VkResult result = vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &m_vkRenderPass);
    if (result != VK_SUCCESS)
    {
        std::cout << "Failed to create render pass" << std::endl;
        std::terminate();
    }

    for (auto& target : colorTargets)
    {
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_vkRenderPass;
        std::vector<VkImageView> attachmentViews{ target->m_imageView };
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
        framebufferInfo.pAttachments = attachmentViews.data();
        framebufferInfo.width = target->m_width;
        framebufferInfo.height = target->m_height;
        framebufferInfo.layers = 1;

        VkFramebuffer framebuffer;
        result = vkCreateFramebuffer(m_vkDevice, &framebufferInfo, nullptr, &framebuffer);
        m_framebuffers.emplace_back(framebuffer);
        if (result != VK_SUCCESS)
        {
            std::cout << "Failed to create framebuffer" << std::endl;
            std::terminate();
        }
    }

    m_targetWidth = colorTargets[0]->m_width;
    m_targetHeight = colorTargets[0]->m_height;

    // Initialize imgui
    {
        std::array<VkDescriptorPoolSize, 1> poolSizes =
        {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
        };
        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 1;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        vkCreateDescriptorPool(m_vkDevice, &poolInfo, nullptr, &m_descPool);
    }
    ImGui_ImplVulkan_InitInfo imguiInitInfo = {};
    imguiInitInfo.Instance = initInfo.instance;
    imguiInitInfo.PhysicalDevice = initInfo.physicalDevice;
    imguiInitInfo.Device = m_vkDevice;
    imguiInitInfo.QueueFamily = initInfo.queueFamilyIdx;
    imguiInitInfo.Queue = initInfo.queue;
    imguiInitInfo.PipelineCache = VK_NULL_HANDLE;
    imguiInitInfo.DescriptorPool = m_descPool;
    imguiInitInfo.Subpass = 0;
    imguiInitInfo.MinImageCount = initInfo.minImageCount;
    imguiInitInfo.ImageCount = initInfo.imageCount;
    imguiInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    imguiInitInfo.Allocator = nullptr;
    imguiInitInfo.CheckVkResultFn = [](VkResult result)
    {
        if (result != VK_SUCCESS)
        {
            std::cout << "Vulkan error in imgui: " << result << std::endl;
            std::terminate();
        }
    };
    ImGui_ImplVulkan_Init(&imguiInitInfo, m_vkRenderPass);
}

ImguiPass::~ImguiPass()
{
    vkDestroyDescriptorPool(m_vkDevice, m_descPool, nullptr);
}

void ImguiPass::renderImpl(Scene* scene, VkCommandBuffer commandBuffer, uint32_t bufferIdx, float dt)
{
    begin(commandBuffer, m_frameBufferIdx);

    ImDrawData* imguiDrawData = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(imguiDrawData, commandBuffer);

    end(commandBuffer);
}