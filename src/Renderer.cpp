#include "Renderer.h"

#include "Scene.h"
#include "SkyPass.h"
#include "GBufferPass.h"
#include "ShadowPass.h"
#include "LightingPass.h"
#include "ImguiPass.h"
#include "RenderThreadPool.h"
#include "InputHandler.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <iostream>
#include <array>

Renderer::Renderer()
{
    initVulkan();

    m_inputHandler = std::make_unique<InputHandler>(m_window);

    m_depthBuffer = std::make_unique<Texture>(m_vkPhysicalDevice, m_vkDevice, WINDOW_WIDTH, WINDOW_HEIGHT, 
        VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    m_gBufferAlbedo = std::make_unique<Texture>(m_vkPhysicalDevice, m_vkDevice, WINDOW_WIDTH, WINDOW_HEIGHT,
        VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    m_gBufferNormal = std::make_unique<Texture>(m_vkPhysicalDevice, m_vkDevice, WINDOW_WIDTH, WINDOW_HEIGHT,
        VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    m_shadowMap = std::make_unique<Texture>(m_vkPhysicalDevice, m_vkDevice, ShadowPass::MAP_WIDTH, ShadowPass::MAP_HEIGHT,
        VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    uint32_t imageCount = 0;
    std::vector<VkImage> swapChainImages;
    vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapChain, &imageCount, swapChainImages.data());
    for (auto const& image : swapChainImages)
    {
        m_frameBuffers.emplace_back(std::make_unique<Texture>(m_vkDevice, WINDOW_WIDTH, WINDOW_HEIGHT, VK_FORMAT_B8G8R8A8_SRGB, image));
    }

    m_renderThreadPool = std::make_unique<RenderThreadPool>(m_vkDevice, m_queueFamilyIdx, m_threadCount);

    m_renderPasses.resize(RenderPassId::COUNT);

    std::vector<Texture*> skyTargets{ m_gBufferAlbedo.get() };
    m_renderPasses[RenderPassId::SKY] = std::make_unique<SkyPass>(m_vkPhysicalDevice, m_vkDevice, m_renderThreadPool.get(), skyTargets, m_presentQueue, m_queueFamilyIdx);

    std::vector<Texture*> gBufferColorTargets{m_gBufferAlbedo.get(), m_gBufferNormal.get()};
    m_renderPasses[RenderPassId::GBUFFER] = std::make_unique<GBufferPass>(m_vkDevice, m_renderThreadPool.get(), gBufferColorTargets, m_depthBuffer.get());

    std::vector<Texture*> shadowPassDepthTargets{ m_shadowMap.get() };
    m_renderPasses[RenderPassId::SHADOW] = std::make_unique<ShadowPass>(m_vkPhysicalDevice, m_vkDevice, m_renderThreadPool.get(), shadowPassDepthTargets);

    std::vector<Texture*> onScreenColorTargets;
    for (auto& framebuffer : m_frameBuffers)
    {
        onScreenColorTargets.emplace_back(framebuffer.get());
    }
    std::vector<Texture*> lightingSrcTextures{ m_gBufferAlbedo.get(), m_gBufferNormal.get(), m_depthBuffer.get(), m_shadowMap.get() };
    m_renderPasses[RenderPassId::LIGHTING] = std::make_unique<LightingPass>(m_vkPhysicalDevice, m_vkDevice, m_renderThreadPool.get(), onScreenColorTargets, lightingSrcTextures);

    ImguiPass::InitInfo imguiInitInfo{};
    imguiInitInfo.instance = m_vkInstance;
    imguiInitInfo.physicalDevice = m_vkPhysicalDevice;
    imguiInitInfo.queueFamilyIdx = m_queueFamilyIdx;
    imguiInitInfo.minImageCount = m_minImageCount;
    imguiInitInfo.imageCount = imageCount;
    imguiInitInfo.queue = m_presentQueue;
    m_renderPasses[RenderPassId::IMGUI] = std::make_unique<ImguiPass>(imguiInitInfo, m_vkDevice, m_renderThreadPool.get(), onScreenColorTargets);

    m_scene = std::make_unique<Scene>(m_renderPasses[RenderPassId::GBUFFER].get(), m_vkPhysicalDevice, m_vkDevice, m_presentQueue, m_queueFamilyIdx);

    // Set the render job dependencies

    // Need to wait because G-buffer is not double buffered so it might still be in use by the previus thread.
    // Otherwise only lighting pass would depend on the framebuffer availability.
    m_renderPasses[SKY]->dependsOn(m_frameBufferAvailable);

    m_renderPasses[GBUFFER]->dependsOn(m_renderPasses[SKY].get());
    
    m_renderPasses[LIGHTING]->dependsOn(m_renderPasses[GBUFFER].get());
    m_renderPasses[LIGHTING]->dependsOn(m_renderPasses[SHADOW].get());

    m_renderPasses[IMGUI]->dependsOn(m_renderPasses[LIGHTING].get());
    m_renderPasses[IMGUI]->signalCPU(m_vkFences);
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
    //instanceExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    //instanceCreateInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
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
    m_minImageCount = surfaceCapabilities.minImageCount;
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
    swapChainCreateInfo.minImageCount = m_minImageCount;
    swapChainCreateInfo.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
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

    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (auto& semaphore : m_frameBufferAvailable)
    {
        result = vkCreateSemaphore(m_vkDevice, &semaphoreCreateInfo, nullptr, &semaphore);
        if (result != VK_SUCCESS)
        {
            std::cout << "Failed to create \"frame buffer available\" semaphore" << std::endl;
            std::terminate();
        }
    }

    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (auto& fence : m_vkFences)
    {
        result = vkCreateFence(m_vkDevice, &fenceCreateInfo, nullptr, &fence);
        if (result != VK_SUCCESS)
        {
            std::cout << "Failed to create fence" << std::endl;
            std::terminate();
        }
    }

    // Initialize imgui
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForVulkan(m_window, true);
}

Renderer::~Renderer()
{
    m_renderThreadPool->clean();

    for (uint32_t i = 0; i < BUFFER_COUNT; ++i)
    {
        vkDestroySemaphore(m_vkDevice, m_frameBufferAvailable[i], nullptr);
        vkDestroyFence(m_vkDevice, m_vkFences[i], nullptr);
    }

    m_renderPasses.clear();
    m_gBufferAlbedo.reset();
    m_gBufferNormal.reset();
    m_depthBuffer.reset();
    m_shadowMap.reset();
    m_frameBuffers.clear();
    m_scene->clean();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    vkDestroySwapchainKHR(m_vkDevice, m_vkSwapChain, nullptr);
    vkDestroyDevice(m_vkDevice, nullptr);
    vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);
    vkDestroyInstance(m_vkInstance, nullptr);
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Renderer::beginFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow(nullptr);
    ImGui::Render();

    vkWaitForFences(m_vkDevice, 1, &m_vkFences[m_bufferIdx], VK_TRUE, UINT64_MAX);
    vkResetFences(m_vkDevice, 1, &m_vkFences[m_bufferIdx]);

    vkAcquireNextImageKHR(m_vkDevice, m_vkSwapChain, UINT64_MAX, m_frameBufferAvailable[m_bufferIdx], VK_NULL_HANDLE, &m_frameBufferIdx);

    static auto prevTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    m_dt = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - prevTime).count();
    prevTime = currentTime;
}

void Renderer::endFrame()
{
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    VkSemaphore waitSemaphores[] = { m_renderPasses[IMGUI]->m_renderJobs[m_bufferIdx].deviceSignal };
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = waitSemaphores;

    VkSwapchainKHR swapChains[] = { m_vkSwapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &m_frameBufferIdx;

    {
        std::unique_lock lock(m_renderPasses[IMGUI]->m_renderJobs[m_bufferIdx].hostSignal.mutex);
        m_renderPasses[IMGUI]->m_renderJobs[m_bufferIdx].hostSignal.cv.wait(lock, [this]()
            {
                return m_renderPasses[IMGUI]->m_renderJobs[m_bufferIdx].hostSignal.signaled;
            });
        m_renderPasses[IMGUI]->m_renderJobs[m_bufferIdx].hostSignal.signaled = false;
    }

    vkQueuePresentKHR(m_presentQueue, &presentInfo);

    m_bufferIdx = (m_bufferIdx + 1) % BUFFER_COUNT;
}

void Renderer::update()
{
    m_inputHandler->update();

    m_scene->update(m_inputHandler.get(), m_bufferIdx);
}

void Renderer::render()
{
    for (auto& pass : m_renderPasses)
    {
        pass->render(m_scene.get(), m_frameBufferIdx, m_bufferIdx, m_dt);
    }
}

void Renderer::loop()
{
    while (!glfwWindowShouldClose(m_window))
    {
        beginFrame();

        update();
        render();

        endFrame();
    }

    vkDeviceWaitIdle(m_vkDevice);
}
