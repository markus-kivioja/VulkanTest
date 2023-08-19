#include "Renderer.h"

#include "Scene.h"
#include "GBufferPass.h"
#include "ShadowPass.h"
#include "LightingPass.h"
#include "RenderThreadPool.h"

#include <iostream>

Renderer::Renderer()
{
    initVulkan();

    m_depthBuffer = std::make_unique<Texture>(m_vkPhysicalDevice, m_vkDevice, WINDOW_WIDTH, WINDOW_HEIGHT, 
        VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    m_gBufferAlbedo = std::make_unique<Texture>(m_vkPhysicalDevice, m_vkDevice, WINDOW_WIDTH, WINDOW_HEIGHT,
        VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    m_gBufferNormal = std::make_unique<Texture>(m_vkPhysicalDevice, m_vkDevice, WINDOW_WIDTH, WINDOW_HEIGHT,
        VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    uint32_t imageCount = 0;
    std::vector<VkImage> swapChainImages;
    vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapChain, &imageCount, swapChainImages.data());
    for (auto const& image : swapChainImages)
    {
        m_frameBuffers.push_back(std::make_unique<Texture>(m_vkDevice, WINDOW_WIDTH, WINDOW_HEIGHT, VK_FORMAT_B8G8R8A8_UNORM, image));
    }

    m_renderPasses.resize(RenderPassId::COUNT);

    std::vector<Texture*> gBufferColorTargets{m_gBufferAlbedo.get(), m_gBufferNormal.get()};
    m_renderPasses[RenderPassId::GBUFFER] = std::make_unique<GBufferPass>(m_vkDevice, gBufferColorTargets, m_depthBuffer.get());

    m_renderPasses[RenderPassId::SHADOW] = std::make_unique<ShadowPass>(m_vkPhysicalDevice, m_vkDevice);

    std::vector<Texture*> lightingColorTargets;
    for (auto& framebuffer : m_frameBuffers)
    {
        lightingColorTargets.push_back(framebuffer.get());
    }
    std::vector<Texture*> lightingSrcTextures{ m_gBufferAlbedo.get(), m_gBufferNormal.get(), m_depthBuffer.get() };
    m_renderPasses[RenderPassId::LIGHTING] = std::make_unique<LightingPass>(m_vkPhysicalDevice, m_vkDevice, lightingColorTargets, lightingSrcTextures);

    m_scene = std::make_unique<Scene>(m_renderPasses[RenderPassId::GBUFFER].get(), m_vkPhysicalDevice, m_vkDevice, m_presentQueue, m_queueFamilyIdx);

    m_renderThreadPool = std::make_unique<RenderThreadPool>(m_vkDevice, m_queueFamilyIdx, m_threadCount);
}

void Renderer::initVulkan()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "VulkanTest", nullptr, nullptr);

    uint32_t extensionCount{ 0 };
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "VulkaTest";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
#if _DEBUG
    instanceCreateInfo.enabledLayerCount = 1;
    const char* validLayer[] = { "VK_LAYER_KHRONOS_validation" };
    instanceCreateInfo.ppEnabledLayerNames = validLayer;
#else
    instanceCreateInfo.enabledLayerCount = 0;
#endif
    std::vector<const char*> instanceExtensions;
    for (uint32_t i = 0; i < glfwExtensionCount; i++)
    {
        instanceExtensions.emplace_back(glfwExtensions[i]);
    }
    instanceExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    instanceCreateInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
    VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &m_vkInstance);
    if (result != VK_SUCCESS)
    {
        std::cout << "Failed to create Vulkan instance" << std::endl;
        std::terminate();
    }

    result = glfwCreateWindowSurface(m_vkInstance, m_window, nullptr, &m_vkSurface);
    if (result != VK_SUCCESS)
    {
        std::cout << "Failed to create window surface!" << std::endl;
        std::terminate();
    }

    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(m_vkInstance, &physicalDeviceCount, nullptr);
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(m_vkInstance, &physicalDeviceCount, physicalDevices.data());
    m_vkPhysicalDevice = physicalDevices[0];

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &queueFamilyCount, queueFamilies.data());
    m_queueFamilyIdx = 0;
    for (auto const& queueFamily : queueFamilies)
    {
        VkBool32 surfaceSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_vkPhysicalDevice, m_queueFamilyIdx, m_vkSurface, &surfaceSupport);
        if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && surfaceSupport)
        {
            m_threadCount = queueFamily.queueCount - 1;
            break;
        }
        m_queueFamilyIdx++;
    }
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = m_queueFamilyIdx;
    queueCreateInfo.queueCount = m_threadCount + 1;
    std::vector<float> queuePriorities(m_threadCount + 1, 1.0f);
    queueCreateInfo.pQueuePriorities = queuePriorities.data();
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    const std::vector<const char*> deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
#if _DEBUG
    deviceCreateInfo.enabledLayerCount = 1;
    deviceCreateInfo.ppEnabledLayerNames = validLayer;
#else
    deviceCreateInfo.enabledLayerCount = 0;
#endif
    result = vkCreateDevice(m_vkPhysicalDevice, &deviceCreateInfo, nullptr, &m_vkDevice);
    if (result != VK_SUCCESS)
    {
        std::cout << "Failed to create logical device" << std::endl;
        std::terminate();
    }
    vkGetDeviceQueue(m_vkDevice, m_queueFamilyIdx, 0, &m_presentQueue);

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_vkPhysicalDevice, m_vkSurface, &surfaceCapabilities);
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_vkPhysicalDevice, m_vkSurface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_vkPhysicalDevice, m_vkSurface, &presentModeCount, presentModes.data());
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysicalDevice, m_vkSurface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysicalDevice, m_vkSurface, &formatCount, formats.data());
    VkSwapchainCreateInfoKHR swapChainCreateInfo{};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = m_vkSurface;
    swapChainCreateInfo.minImageCount = surfaceCapabilities.minImageCount;
    swapChainCreateInfo.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    swapChainCreateInfo.imageColorSpace = formats[0].colorSpace;
    swapChainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.presentMode = presentModes[0];
    swapChainCreateInfo.clipped = VK_TRUE;
    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
    swapChainCreateInfo.queueFamilyIndexCount = 1;
    uint32_t queueFamilyIndices[] = { static_cast<uint32_t>(m_queueFamilyIdx) };
    swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    result = vkCreateSwapchainKHR(m_vkDevice, &swapChainCreateInfo, nullptr, &m_vkSwapChain);
    if (result != VK_SUCCESS)
    {
        std::cout << "Failed to create swap chain" << std::endl;
        std::terminate();
    }

    VkCommandPoolCreateInfo commanPoolCreateInfo{};
    commanPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commanPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commanPoolCreateInfo.queueFamilyIndex = m_queueFamilyIdx;
    result = vkCreateCommandPool(m_vkDevice, &commanPoolCreateInfo, nullptr, &m_vkCommandPool);
    if (result != VK_SUCCESS)
    {
        std::cout << "Failed to create command pool" << std::endl;
        std::terminate();
    }

    m_vkCommandBuffers.resize(BUFFER_COUNT);
    VkCommandBufferAllocateInfo commandBufferAllocInfo{};
    commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocInfo.commandPool = m_vkCommandPool;
    commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocInfo.commandBufferCount = static_cast<uint32_t>(m_vkCommandBuffers.size());
    result = vkAllocateCommandBuffers(m_vkDevice, &commandBufferAllocInfo, m_vkCommandBuffers.data());
    if (result != VK_SUCCESS)
    {
        std::cout << "Failed to allocate command buffers" << std::endl;
        std::terminate();
    }

    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    m_vkFrameBufferAvailableSemaphores.resize(BUFFER_COUNT);
    for (auto& semaphore : m_vkFrameBufferAvailableSemaphores)
    {
        result = vkCreateSemaphore(m_vkDevice, &semaphoreCreateInfo, nullptr, &semaphore);
        if (result != VK_SUCCESS)
        {
            std::cout << "Failed to create \"frame buffer available\" semaphore" << std::endl;
            std::terminate();
        }
    }
    m_vkRenderFinishedSemaphores.resize(BUFFER_COUNT);
    for (auto& semaphore : m_vkRenderFinishedSemaphores)
    {
        result = vkCreateSemaphore(m_vkDevice, &semaphoreCreateInfo, nullptr, &semaphore);
        if (result != VK_SUCCESS)
        {
            std::cout << "Failed to create \"render finished\" semaphore" << std::endl;
            std::terminate();
        }
    }

    m_gBufferPassFinished.resize(BUFFER_COUNT);
    for (auto& semaphore : m_gBufferPassFinished)
    {
        result = vkCreateSemaphore(m_vkDevice, &semaphoreCreateInfo, nullptr, &semaphore);
        if (result != VK_SUCCESS)
        {
            std::cout << "Failed to create \"G-buffer finished\" semaphore" << std::endl;
            std::terminate();
        }
    }

    m_shadowPassFinished.resize(BUFFER_COUNT);
    for (auto& semaphore : m_shadowPassFinished)
    {
        result = vkCreateSemaphore(m_vkDevice, &semaphoreCreateInfo, nullptr, &semaphore);
        if (result != VK_SUCCESS)
        {
            std::cout << "Failed to create \"shadow pass finished\" semaphore" << std::endl;
            std::terminate();
        }
    }

    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    m_vkFences.resize(BUFFER_COUNT);
    for (auto& fence : m_vkFences)
    {
        result = vkCreateFence(m_vkDevice, &fenceCreateInfo, nullptr, &fence);
        if (result != VK_SUCCESS)
        {
            std::cout << "Failed to create fence" << std::endl;
            std::terminate();
        }
    }
}

Renderer::~Renderer()
{
    m_renderThreadPool->clean();

    for (uint32_t i = 0; i < BUFFER_COUNT; ++i)
    {
        vkDestroySemaphore(m_vkDevice, m_vkRenderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(m_vkDevice, m_vkFrameBufferAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(m_vkDevice, m_gBufferPassFinished[i], nullptr);
        vkDestroySemaphore(m_vkDevice, m_shadowPassFinished[i], nullptr);
        vkDestroyFence(m_vkDevice, m_vkFences[i], nullptr);
    }
    vkDestroyCommandPool(m_vkDevice, m_vkCommandPool, nullptr);

    m_renderPasses.clear();
    m_gBufferAlbedo.reset();
    m_gBufferNormal.reset();
    m_depthBuffer.reset();
    m_frameBuffers.clear();
    m_scene->clean();

    vkDestroySwapchainKHR(m_vkDevice, m_vkSwapChain, nullptr);
    vkDestroyDevice(m_vkDevice, nullptr);
    vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);
    vkDestroyInstance(m_vkInstance, nullptr);
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Renderer::beginFrame()
{
    vkWaitForFences(m_vkDevice, 1, &m_vkFences[m_bufferIdx], VK_TRUE, UINT64_MAX);
    vkResetFences(m_vkDevice, 1, &m_vkFences[m_bufferIdx]);

    vkAcquireNextImageKHR(m_vkDevice, m_vkSwapChain, UINT64_MAX, m_vkFrameBufferAvailableSemaphores[m_bufferIdx], VK_NULL_HANDLE, &m_frameBufferIdx);
}

void Renderer::endFrame()
{
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    VkSemaphore waitSemaphores[] = { m_vkRenderFinishedSemaphores[m_bufferIdx] };
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = waitSemaphores;

    VkSwapchainKHR swapChains[] = { m_vkSwapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &m_frameBufferIdx;

    {
        std::unique_lock lock(m_lightinPassSubmitted.mutex);
        m_lightinPassSubmitted.cv.wait(lock, [this]()
            {
                return m_lightinPassSubmitted.signaled;
            });
        m_lightinPassSubmitted.signaled = false;
    }

    vkQueuePresentKHR(m_presentQueue, &presentInfo);

    m_bufferIdx = (m_bufferIdx + 1) % BUFFER_COUNT;
}

void Renderer::render()
{
    auto keyCallback = [](GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        auto scene = (Scene*)glfwGetWindowUserPointer(window);
        scene->keyPressed(window, key, scancode, action, mods);
    };
    glfwSetWindowUserPointer(m_window, m_scene.get());
    glfwSetKeyCallback(m_window, keyCallback);

    while (!glfwWindowShouldClose(m_window))
    {
        beginFrame();

        // Create the G-buffer render job
        RenderThreadPool::RenderJob gBufferJob{};
        gBufferJob.bufferIdx = m_bufferIdx;
        gBufferJob.signalSemaphores = std::vector<VkSemaphore>{ m_gBufferPassFinished[m_bufferIdx] };
        gBufferJob.hostSignals = std::vector<HostSemaphore*>{ &m_gBufferPassSubmitted };
        gBufferJob.job = [this](VkCommandBuffer commandBuffer)
        {
            m_renderPasses[GBUFFER]->begin(commandBuffer);
            m_scene->render(commandBuffer, m_bufferIdx);
            m_renderPasses[GBUFFER]->end(commandBuffer);
        };

        // Create the lighting render job
        RenderThreadPool::RenderJob lightingJob{};
        lightingJob.bufferIdx = m_bufferIdx;
        lightingJob.fence = m_vkFences[m_bufferIdx];
        lightingJob.waitSemaphores = std::vector<VkSemaphore>{
            m_gBufferPassFinished[m_bufferIdx],
            m_vkFrameBufferAvailableSemaphores[m_bufferIdx]
        };
        lightingJob.hostWaits = std::vector<HostSemaphore*>{ &m_gBufferPassSubmitted };
        lightingJob.signalSemaphores = std::vector<VkSemaphore>{ m_vkRenderFinishedSemaphores[m_bufferIdx] };
        lightingJob.hostSignals = std::vector<HostSemaphore*>{ &m_lightinPassSubmitted };
        lightingJob.job = [this](VkCommandBuffer commandBuffer)
        {
            m_renderPasses[LIGHTING]->begin(commandBuffer, m_frameBufferIdx);
            m_renderPasses[LIGHTING]->render(commandBuffer, m_bufferIdx);
            m_renderPasses[LIGHTING]->end(commandBuffer);
        };
        
        // Give the rendering jobs to the thread pool
        m_renderThreadPool->addJob(gBufferJob);
        m_renderThreadPool->addJob(lightingJob);

        endFrame();

        glfwPollEvents();
    }

    vkDeviceWaitIdle(m_vkDevice);
}
