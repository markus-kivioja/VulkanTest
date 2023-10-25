#include "GBufferPass.h"

#include "Texture.h"
#include "Scene.h"

#include <iostream>
#include <array>

GBufferPass::GBufferPass(VkDevice device, RenderThreadPool* threadPool, std::vector<Texture*>& colorTargets, Texture* depthTarget,
    VkDescriptorSetLayout transformDescSetLayout, VkDescriptorSetLayout materialDescSetLayout) :
	RenderPass::RenderPass(device, threadPool, 2)
{
    m_hasDepthAttachment = true;

    std::array<VkAttachmentDescription, 3> attachmentDescriptions;
    attachmentDescriptions[0].flags = 0;
    attachmentDescriptions[0].format = colorTargets[0]->m_format;
    attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    attachmentDescriptions[1].flags = 0;
    attachmentDescriptions[1].format = colorTargets[1]->m_format;
    attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    attachmentDescriptions[2].flags = 0;
    attachmentDescriptions[2].format = depthTarget->m_format;
    attachmentDescriptions[2].samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescriptions[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescriptions[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescriptions[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescriptions[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescriptions[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescriptions[2].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    std::array<VkAttachmentReference, 2> colorAttachmentRefs;
    colorAttachmentRefs[0].attachment = 0;
    colorAttachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachmentRefs[1].attachment = 1;
    colorAttachmentRefs[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 2;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDesc{};
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.colorAttachmentCount = 2;
    subpassDesc.pColorAttachments = colorAttachmentRefs.data();
    subpassDesc.pDepthStencilAttachment = &depthAttachmentRef;

    VkRenderPassCreateInfo renderPassCreateInfo{};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
    renderPassCreateInfo.pAttachments = attachmentDescriptions.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDesc;
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcAccessMask = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    if (depthTarget)
    {
        dependency.srcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &dependency;

    VkResult result = vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &m_vkRenderPass);
    if (result != VK_SUCCESS)
    {
        std::cout << "Failed to create render pass" << std::endl;
        std::terminate();
    }

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = m_vkRenderPass;
    std::array<VkImageView, 3> attachmentViews{ colorTargets[0]->m_imageView, colorTargets[1]->m_imageView, depthTarget->m_imageView };
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
    framebufferInfo.pAttachments = attachmentViews.data();
    framebufferInfo.width = colorTargets[0]->m_width;
    framebufferInfo.height = colorTargets[0]->m_height;
    framebufferInfo.layers = 1;

    VkFramebuffer framebuffer;
    result = vkCreateFramebuffer(m_vkDevice, &framebufferInfo, nullptr, &framebuffer);
    m_framebuffers.emplace_back(framebuffer);
    if (result != VK_SUCCESS)
    {
        std::cout << "Failed to create framebuffer" << std::endl;
        std::terminate();
    }

    m_targetWidth = colorTargets[0]->m_width;
    m_targetHeight = colorTargets[0]->m_height;

    auto vertexShaderSrc = readFile("shaders/gbuffer_vert.spv");
    auto fragmentShaderSrc = readFile("shaders/gbuffer_frag.spv");

    VkShaderModule vertexShader = createVkShader(vertexShaderSrc);
    VkShaderModule fragmentShader = createVkShader(fragmentShaderSrc);

    VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
    vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderStageInfo.module = vertexShader;
    vertexShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
    fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderStageInfo.module = fragmentShader;
    fragmentShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderStageInfo, fragmentShaderStageInfo };

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, position);
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, normal);
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, uvCoord);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment1{};
    colorBlendAttachment1.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment1.blendEnable = VK_FALSE;
    VkPipelineColorBlendAttachmentState colorBlendAttachment2{};
    colorBlendAttachment2.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment2.blendEnable = VK_FALSE;

    std::array<VkPipelineColorBlendAttachmentState, 2> colorBlendAttachments{ colorBlendAttachment1 , colorBlendAttachment2 };

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
    colorBlending.pAttachments = colorBlendAttachments.data();
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    std::array<VkDescriptorSetLayout, 3> descSetLayouts{ transformDescSetLayout, materialDescSetLayout, m_cameraSetLayout };
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descSetLayouts.data();

    result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
    if (result != VK_SUCCESS)
    {
        std::cout << "Failed to create pipeline layout" << std::endl;
        std::terminate();
    }

    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{};
    depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
    depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
    depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencilStateCreateInfo;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_vkRenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);
    if (result != VK_SUCCESS)
    {
        std::cout << ("Failed to create graphics pipeline") << std::endl;
        std::terminate();
    }

    vkDestroyShaderModule(device, fragmentShader, nullptr);
    vkDestroyShaderModule(device, vertexShader, nullptr);
}

GBufferPass::~GBufferPass()
{

}

void GBufferPass::renderImpl(Scene* scene, VkCommandBuffer commandBuffer, uint32_t bufferIdx, float dt)
{
    begin(commandBuffer);
    scene->render(commandBuffer, m_pipelineLayout, Camera::Type::NORMAL, bufferIdx, dt);
    end(commandBuffer);
}