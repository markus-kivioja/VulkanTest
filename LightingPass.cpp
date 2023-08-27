#include "LightingPass.h"

#include "Texture.h"
#include "Renderer.h"
#include "Scene.h"

#include <array>
#include <iostream>

LightingPass::LightingPass(VkPhysicalDevice physicalDevice, VkDevice device, std::vector<Texture*> colorTargets, std::vector<Texture*> srcTextures) :
	RenderPass::RenderPass(device, 1)
{
    m_hasDepthAttachment = false;

    VkAttachmentDescription colorAttachmentDescription{};
    colorAttachmentDescription.format = colorTargets[0]->m_format;
    colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDesc{};
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.pColorAttachments = &colorAttachmentRef;

    std::vector<VkAttachmentDescription> attachmentDescriptions = { colorAttachmentDescription };

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
        m_framebuffers.push_back(framebuffer);
        if (result != VK_SUCCESS)
        {
            std::cout << "Failed to create framebuffer" << std::endl;
            std::terminate();
        }
    }

    m_targetWidth = colorTargets[0]->m_width;
    m_targetHeight = colorTargets[0]->m_height;

    auto vertexShaderSrc = readFile("shaders/lighting_vert.spv");
    auto fragmentShaderSrc = readFile("shaders/lighting_frag.spv");

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

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
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
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
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

    VkDescriptorSetLayoutBinding uniformBufferLayoutBinding{};
    uniformBufferLayoutBinding.binding = 0;
    uniformBufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformBufferLayoutBinding.descriptorCount = 1;
    uniformBufferLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding albedoLayoutBinding{};
    albedoLayoutBinding.binding = 1;
    albedoLayoutBinding.descriptorCount = 1;
    albedoLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    albedoLayoutBinding.pImmutableSamplers = nullptr;
    albedoLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding normalLayoutBinding{};
    normalLayoutBinding.binding = 2;
    normalLayoutBinding.descriptorCount = 1;
    normalLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    normalLayoutBinding.pImmutableSamplers = nullptr;
    normalLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding depthLayoutBinding{};
    depthLayoutBinding.binding = 3;
    depthLayoutBinding.descriptorCount = 1;
    depthLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    depthLayoutBinding.pImmutableSamplers = nullptr;
    depthLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding shadowLayoutBinding{};
    shadowLayoutBinding.binding = 4;
    shadowLayoutBinding.descriptorCount = 1;
    shadowLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    shadowLayoutBinding.pImmutableSamplers = nullptr;
    shadowLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
    descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    std::array<VkDescriptorSetLayoutBinding, 5> bindings = { uniformBufferLayoutBinding, albedoLayoutBinding, normalLayoutBinding, depthLayoutBinding, shadowLayoutBinding };
    descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    descriptorSetLayoutInfo.pBindings = bindings.data();

    result = vkCreateDescriptorSetLayout(device, &descriptorSetLayoutInfo, nullptr, &m_modelSetLayout);
    if (result != VK_SUCCESS)
    {
        std::cout << "Failed to create descriptor set layout!" << std::endl;
        std::terminate();
    }

    VkDescriptorSetLayoutBinding cameraTransformLayoutBinding{};
    cameraTransformLayoutBinding.binding = 0;
    cameraTransformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cameraTransformLayoutBinding.descriptorCount = 1;
    cameraTransformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo cameraDescSetLayoutInfo{};
    cameraDescSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    std::array<VkDescriptorSetLayoutBinding, 1> cameraBindings = { cameraTransformLayoutBinding };
    cameraDescSetLayoutInfo.bindingCount = static_cast<uint32_t>(cameraBindings.size());
    cameraDescSetLayoutInfo.pBindings = cameraBindings.data();

    result = vkCreateDescriptorSetLayout(device, &cameraDescSetLayoutInfo, nullptr, &m_cameraSetLayout);
    if (result != VK_SUCCESS)
    {
        std::cout << "Failed to create camera descriptor set layout!" << std::endl;
        std::terminate();
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    std::array<VkDescriptorSetLayout, 2> descSetLayouts{ m_modelSetLayout, m_cameraSetLayout };
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descSetLayouts.data();

    result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
    if (result != VK_SUCCESS)
    {
        std::cout << "Failed to create pipeline layout" << std::endl;
        std::terminate();
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
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

    VkDescriptorPoolSize uniformBufferPoolSize{};
    uniformBufferPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformBufferPoolSize.descriptorCount = static_cast<uint32_t>(Renderer::BUFFER_COUNT);

    VkDescriptorPoolSize texturePoolSize{};
    texturePoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texturePoolSize.descriptorCount = 4 * static_cast<uint32_t>(Renderer::BUFFER_COUNT);

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    std::array<VkDescriptorPoolSize, 2> poolSizes{ uniformBufferPoolSize, texturePoolSize };
    descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
    descriptorPoolCreateInfo.maxSets = static_cast<uint32_t>(Renderer::BUFFER_COUNT);

    result = vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &m_descriptorPool);
    if (result != VK_SUCCESS)
    {
        std::cout << "Failed to create descriptor pool" << std::endl;
        std::terminate();
    }

    std::vector<VkDescriptorSetLayout> layouts(Renderer::BUFFER_COUNT, m_modelSetLayout);
    VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
    descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocInfo.descriptorPool = m_descriptorPool;
    descriptorSetAllocInfo.descriptorSetCount = static_cast<uint32_t>(Renderer::BUFFER_COUNT);
    descriptorSetAllocInfo.pSetLayouts = layouts.data();

    m_descriptorSets.resize(Renderer::BUFFER_COUNT);
    result = vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, m_descriptorSets.data());
    if (result != VK_SUCCESS) {
        std::cout << "Failed to allocate descriptor sets" << std::endl;
        std::terminate();
    }
    for (uint32_t i = 0; i < Renderer::BUFFER_COUNT; ++i)
    {
        m_uniformBuffers.push_back(std::make_unique<Buffer>(physicalDevice, device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(LightingPass::Transforms)));

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uniformBuffers[i]->m_vkBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(LightingPass::Transforms);

        VkDescriptorImageInfo albedoInfo{};
        albedoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        albedoInfo.imageView = srcTextures[0]->m_imageView;
        albedoInfo.sampler = srcTextures[0]->m_sampler;

        VkDescriptorImageInfo normalInfo{};
        normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        normalInfo.imageView = srcTextures[1]->m_imageView;
        normalInfo.sampler = srcTextures[1]->m_sampler;

        VkDescriptorImageInfo depthInfo{};
        depthInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        depthInfo.imageView = srcTextures[2]->m_imageView;
        depthInfo.sampler = srcTextures[2]->m_sampler;

        VkDescriptorImageInfo shadowInfo{};
        shadowInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        shadowInfo.imageView = srcTextures[3]->m_imageView;
        shadowInfo.sampler = srcTextures[3]->m_sampler;

        VkWriteDescriptorSet uniformBufferDescriptorWrite{};
        uniformBufferDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        uniformBufferDescriptorWrite.dstSet = m_descriptorSets[i];
        uniformBufferDescriptorWrite.dstBinding = 0;
        uniformBufferDescriptorWrite.dstArrayElement = 0;
        uniformBufferDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBufferDescriptorWrite.descriptorCount = 1;
        uniformBufferDescriptorWrite.pBufferInfo = &bufferInfo;

        VkWriteDescriptorSet albedoDescriptorWrite{};
        albedoDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        albedoDescriptorWrite.dstSet = m_descriptorSets[i];
        albedoDescriptorWrite.dstBinding = 1;
        albedoDescriptorWrite.dstArrayElement = 0;
        albedoDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        albedoDescriptorWrite.descriptorCount = 1;
        albedoDescriptorWrite.pImageInfo = &albedoInfo;

        VkWriteDescriptorSet normalDescriptorWrite{};
        normalDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        normalDescriptorWrite.dstSet = m_descriptorSets[i];
        normalDescriptorWrite.dstBinding = 2;
        normalDescriptorWrite.dstArrayElement = 0;
        normalDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        normalDescriptorWrite.descriptorCount = 1;
        normalDescriptorWrite.pImageInfo = &normalInfo;

        VkWriteDescriptorSet depthDescriptorWrite{};
        depthDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        depthDescriptorWrite.dstSet = m_descriptorSets[i];
        depthDescriptorWrite.dstBinding = 3;
        depthDescriptorWrite.dstArrayElement = 0;
        depthDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        depthDescriptorWrite.descriptorCount = 1;
        depthDescriptorWrite.pImageInfo = &depthInfo;

        VkWriteDescriptorSet shadowDescriptorWrite{};
        shadowDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        shadowDescriptorWrite.dstSet = m_descriptorSets[i];
        shadowDescriptorWrite.dstBinding = 4;
        shadowDescriptorWrite.dstArrayElement = 0;
        shadowDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        shadowDescriptorWrite.descriptorCount = 1;
        shadowDescriptorWrite.pImageInfo = &shadowInfo;

        std::array<VkWriteDescriptorSet, 5> descriptorWrites{ 
            uniformBufferDescriptorWrite,
            albedoDescriptorWrite,
            normalDescriptorWrite,
            depthDescriptorWrite,
            shadowDescriptorWrite
        };
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

LightingPass::~LightingPass()
{
    vkDestroyDescriptorPool(m_vkDevice, m_descriptorPool, nullptr);
}

void LightingPass::render(Scene* scene, VkCommandBuffer commandBuffer, uint32_t bufferIdx, float dt)
{
    begin(commandBuffer, m_frameBufferIdx);

    scene->m_cameras[Camera::Type::LIGHT]->bind(commandBuffer, m_pipelineLayout, bufferIdx, dt);

    LightingPass::Transforms transforms{};
    transforms.projInverse = glm::inverse(scene->m_cameras[Camera::Type::NORMAL]->m_projection);
    transforms.viewInverse = glm::inverse(scene->m_cameras[Camera::Type::NORMAL]->m_view);
    transforms.lightDir = scene->m_cameras[Camera::Type::LIGHT]->m_direction;
    
    m_uniformBuffers[bufferIdx]->update(&transforms, sizeof(LightingPass::Transforms));

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[bufferIdx], 0, nullptr);
    vkCmdDraw(commandBuffer, 4, 1, 0, 0);

    end(commandBuffer);
}