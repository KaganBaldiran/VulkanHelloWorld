#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <set>
#include <optional>
#include <limits>
#include <fstream>

#ifdef NDEBUG
    const bool EnableValidationLayers = false;
#else 
    const bool EnableValidationLayers = true;
#endif // !NDEBUG

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto Func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (Func != nullptr)
    {
        return Func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator)
{
    auto Func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (Func != nullptr)
    {
        Func(instance, debugMessenger, pAllocator);
    }
}

std::vector<char> ReadFile(const char* FileName)
{
    std::ifstream File(FileName, std::ios::ate | std::ios::binary);

    if (!File.is_open()) {
        throw std::runtime_error("Failed to open file!");
    }

    size_t FileSize = static_cast<size_t>(File.tellg());
    std::vector<char> Buffer(FileSize);

    File.seekg(0);
    File.read(Buffer.data(), FileSize);
    File.close();
    return Buffer;
}

struct QueueFamilyIndices {
    std::optional<uint32_t> GraphicsFamily;
    std::optional<uint32_t> PresentFamily;

    bool isComplete() {
        return GraphicsFamily.has_value() && PresentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR Capabilities;
    std::vector<VkSurfaceFormatKHR> Formats;
    std::vector<VkPresentModeKHR> PresentModes;
};

class HelloWorldTriangle
{
public:
    void Run()
    {
        InitWindow();
        InitVulkan();
        MainLoop();
        CleanUp();
    }

private:
    GLFWwindow* window;
    unsigned int WindowInitialWidth = 800;
    unsigned int WindowInitialHeight = 600;

    VkInstance Instance;
    VkDebugUtilsMessengerEXT DebugMessenger;
    VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;

    VkDevice LogicalDevice;
    VkQueue GraphicsQueue;
    VkQueue PresentQueue;
    VkSurfaceKHR Surface;


    VkSwapchainKHR SwapChain;
    std::vector<VkImage> SwapChainImages;
    std::vector<VkImageView> SwapChainImagesViews;
    VkSurfaceFormatKHR SurfaceFormat;
    VkPresentModeKHR PresentMode;
    VkExtent2D Extent;

    VkRenderPass RenderPass;
    VkPipelineLayout PipelineLayout;
    VkPipeline GraphicsPipeline;

    VkCommandPool CommandPool;
    VkCommandBuffer CommandBuffer;

    VkSemaphore ImageAvailableSemophore;
    VkSemaphore RenderFinishedSemophore;
    VkFence InFlightFence;

    std::vector<VkFramebuffer> SwapChainFramebuffers;

    const std::vector<const char*> ValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> DeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    void InitWindow()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WindowInitialWidth, WindowInitialHeight, "Vulkan", nullptr, nullptr);

    }

    void InitVulkan()
    {
        CreateInstance();
        SetupDebugMessenger();
        CreateSurface();
        PickPhysicalDevice();
        CreateLogicalDevice();
        CreateSwapChain();
        CreateImageViews();
        CreateRenderPass();
        CreateGraphicsPipeline();
        CreateFramebuffers();
        CreateCommandPool();
        CreateCommandBuffer();
        CreateSyncObjects();
    }

    void MainLoop()
    {
        while (!glfwWindowShouldClose(window))
        {
            DrawFrame();
            glfwPollEvents();
        }

        vkDeviceWaitIdle(LogicalDevice);
    }

    void CleanUp()
    {
        vkDestroySemaphore(LogicalDevice, ImageAvailableSemophore, nullptr);
        vkDestroySemaphore(LogicalDevice, RenderFinishedSemophore, nullptr);
        vkDestroyFence(LogicalDevice, InFlightFence, nullptr);

        vkDestroyCommandPool(LogicalDevice, CommandPool, nullptr);

        for (auto Framebuffer : SwapChainFramebuffers)
        {
            vkDestroyFramebuffer(LogicalDevice, Framebuffer, nullptr);
        }
        vkDestroyPipeline(LogicalDevice, GraphicsPipeline, nullptr);
        vkDestroyPipelineLayout(LogicalDevice, PipelineLayout, nullptr);
        vkDestroyRenderPass(LogicalDevice, RenderPass, nullptr);

        for (auto ImageView : SwapChainImagesViews)
        {
            vkDestroyImageView(LogicalDevice, ImageView, nullptr);
        }

        vkDestroySwapchainKHR(LogicalDevice, SwapChain, nullptr);
        vkDestroyDevice(LogicalDevice, nullptr);
        if (EnableValidationLayers)
        {
            DestroyDebugUtilsMessengerEXT(Instance, DebugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(Instance, Surface, nullptr);
        vkDestroyInstance(Instance, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void CreateInstance()
    {
        if (EnableValidationLayers && !CheckValidationLayerSupport())
        {
            throw std::runtime_error("Requested validation layers aren't available!");
        }

        VkApplicationInfo AppInfo{};
        AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        AppInfo.pApplicationName = "Hello Triangle";
        AppInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
        AppInfo.pEngineName = "No Engine";
        AppInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
        AppInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        CreateInfo.pApplicationInfo = &AppInfo;

        uint32_t GLFWextensionsCount = 0;
        const char** GLFWextensionsString;

        GLFWextensionsString = glfwGetRequiredInstanceExtensions(&GLFWextensionsCount);

        std::vector<const char*> GLFWRequiredExtensions;
        GLFWRequiredExtensions.resize(GLFWextensionsCount);

        for (size_t i = 0; i < GLFWextensionsCount; i++)
        {
            GLFWRequiredExtensions[i] = *(GLFWextensionsString + i);
            std::cout << *(GLFWextensionsString + i) << std::endl;
        }

        if (EnableValidationLayers) {
            GLFWRequiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            GLFWextensionsCount++;
        }

        CreateInfo.enabledExtensionCount = GLFWextensionsCount;
        CreateInfo.ppEnabledExtensionNames = GLFWRequiredExtensions.data();

        VkDebugUtilsMessengerCreateInfoEXT DebugCreateInfo{};
        if (EnableValidationLayers)
        {
            CreateInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
            CreateInfo.ppEnabledLayerNames = ValidationLayers.data();

            PopulateMessagerCreateInfo(DebugCreateInfo);
            CreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&DebugCreateInfo;
        }
        else
        {
            CreateInfo.enabledLayerCount = 0;
            CreateInfo.pNext = nullptr;
        }

        uint32_t ExtensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, nullptr);

        std::vector<VkExtensionProperties> Extensions(ExtensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, Extensions.data());

        std::cout << "Available extensions: " << std::endl;

        unsigned int FoundExtensionsCount = 0;
        for (const auto& extension : Extensions)
        {
            for (const auto& GLFWextention : GLFWRequiredExtensions)
            {
                if (strcmp(GLFWextention, extension.extensionName) == 0)
                {
                    FoundExtensionsCount++;
                }
            }
            std::cout << '\t' << extension.extensionName << std::endl;
        }

        if (FoundExtensionsCount != GLFWextensionsCount)
        {
            std::cout << "Some of the required extensions seems to be missing!" << std::endl;
        }

        if (vkCreateInstance(&CreateInfo, nullptr, &Instance) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create an instance!");
        }
    }

    bool CheckValidationLayerSupport()
    {
        uint32_t LayerCount;
        vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);
        std::vector<VkLayerProperties> AvailableLayers(LayerCount);
        vkEnumerateInstanceLayerProperties(&LayerCount, AvailableLayers.data());

        for (const char* LayerName : ValidationLayers)
        {
            bool layerFound = false;
            for (const auto& AvailableLayer : AvailableLayers)
            {
                if (strcmp(AvailableLayer.layerName, LayerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) return false;
        }
        return true;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        std::cerr << "Validation Layer: " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }

    void PopulateMessagerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& CreateInfo)
    {
        CreateInfo = {};
        CreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        CreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        CreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        CreateInfo.pfnUserCallback = debugCallback;
    }

    void SetupDebugMessenger()
    {
        if (!EnableValidationLayers) return;
        VkDebugUtilsMessengerCreateInfoEXT CreateInfo{};
        PopulateMessagerCreateInfo(CreateInfo);

        if (CreateDebugUtilsMessengerEXT(Instance, &CreateInfo, nullptr, &DebugMessenger) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to setup debug messenger!");
        }
    }

    void PickPhysicalDevice()
    {
        uint32_t DeviceCount = 0;
        vkEnumeratePhysicalDevices(Instance, &DeviceCount, nullptr);

        if (DeviceCount == 0)
        {
            throw std::runtime_error("Failed to detect GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> PhyDevices(DeviceCount);
        vkEnumeratePhysicalDevices(Instance, &DeviceCount, PhyDevices.data());

        std::multimap<int, VkPhysicalDevice>  Candidates;

        for (const auto& Device : PhyDevices)
        {
            int Score = CheckDeviceSuitability(Device);
            Candidates.insert({ Score,Device });
        }

        if (Candidates.rbegin()->first > 0)
        {
            this->PhysicalDevice = Candidates.rbegin()->second;
        }
        else throw std::runtime_error("Failed to detect a suitable physical device!");

        VkPhysicalDeviceProperties DeviceProperties;
        vkGetPhysicalDeviceProperties(PhysicalDevice, &DeviceProperties);
        std::cout << "Physical Device: " << DeviceProperties.deviceName << std::endl;
    }

    int CheckDeviceSuitability(VkPhysicalDevice Device)
    {
        VkPhysicalDeviceProperties DeviceProperties;
        vkGetPhysicalDeviceProperties(Device, &DeviceProperties);

        VkPhysicalDeviceFeatures DeviceFeatures;
        vkGetPhysicalDeviceFeatures(Device, &DeviceFeatures);
        if (!DeviceFeatures.geometryShader) return 0;

        int Score = 0;
        if (DeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            Score += 1000;
        }

        Score += DeviceProperties.limits.maxImageDimension2D;
        QueueFamilyIndices indices = FindQueueFamilies(Device);

        bool IsExtensionsSupported = CheckDeviceExtensionSupport(Device);

        Score *= static_cast<int>(indices.GraphicsFamily.has_value());
        Score *= static_cast<int>(IsExtensionsSupported);

        if (IsExtensionsSupported)
        {
            SwapChainSupportDetails SwapChainSupport = QuerySwapChainSupport(Device);
            Score *= static_cast<int>(!SwapChainSupport.Formats.empty() && !SwapChainSupport.PresentModes.empty());
        }

        return Score;
    }

    bool CheckDeviceExtensionSupport(VkPhysicalDevice Device)
    {
        uint32_t ExtensionCount;
        vkEnumerateDeviceExtensionProperties(Device, nullptr, &ExtensionCount, nullptr);

        std::vector<VkExtensionProperties> AvailableExtensions(ExtensionCount);
        vkEnumerateDeviceExtensionProperties(Device, nullptr, &ExtensionCount, AvailableExtensions.data());

        std::set<std::string> RequiredExtensions(DeviceExtensions.begin(), DeviceExtensions.end());

        for (const auto& Extension : AvailableExtensions)
        {
            RequiredExtensions.erase(Extension.extensionName);
        }

        return RequiredExtensions.empty();
    }

    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice Device)
    {
        QueueFamilyIndices Indices;

        uint32_t QueueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(Device, &QueueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(Device, &QueueFamilyCount, QueueFamilies.data());

        int i = 0;
        for (const auto& QueueFamily : QueueFamilies)
        {
            VkBool32 DoesSupportPresent = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(Device, i, Surface, &DoesSupportPresent);
            if (QueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                Indices.GraphicsFamily = i;
            }
            if (DoesSupportPresent)
            {
                Indices.PresentFamily = i;
            }

            if (Indices.isComplete())
            {
                break;
            }
            i++;
        }
        return Indices;
    }

    void CreateLogicalDevice()
    {
        QueueFamilyIndices indices = FindQueueFamilies(PhysicalDevice);

        std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;
        std::set<uint32_t> UniqueQueueFamilies = { indices.GraphicsFamily.value(),indices.PresentFamily.value() };

        QueueCreateInfos.reserve(UniqueQueueFamilies.size());

        float QueuePriority = 1.0f;
        for (uint32_t QueueFamily : UniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo QueueCreateInfo{};
            QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            QueueCreateInfo.queueFamilyIndex = QueueFamily;
            QueueCreateInfo.queueCount = 1;
            QueueCreateInfo.pQueuePriorities = &QueuePriority;
            QueueCreateInfos.push_back(QueueCreateInfo);
        }

        //TODO Soon to return
        VkPhysicalDeviceFeatures DeviceFeatures{};

        VkDeviceCreateInfo DeviceCreateInfo{};
        DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        DeviceCreateInfo.pQueueCreateInfos = QueueCreateInfos.data();
        DeviceCreateInfo.queueCreateInfoCount = QueueCreateInfos.size();
        DeviceCreateInfo.pEnabledFeatures = &DeviceFeatures;

        DeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(DeviceExtensions.size());
        DeviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();

        if (vkCreateDevice(PhysicalDevice, &DeviceCreateInfo, nullptr, &LogicalDevice) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create the logical device!");
        }

        vkGetDeviceQueue(LogicalDevice, indices.GraphicsFamily.value(), 0, &GraphicsQueue);
        vkGetDeviceQueue(LogicalDevice, indices.PresentFamily.value(), 0, &PresentQueue);
    }

    void CreateSurface()
    {
        if (glfwCreateWindowSurface(Instance, window, nullptr, &Surface) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create a window surface!");
        }
    }

    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice Device)
    {
        SwapChainSupportDetails Details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Device, Surface, &Details.Capabilities);

        uint32_t FormatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(Device, Surface, &FormatCount, nullptr);

        if (FormatCount != 0)
        {
            Details.Formats.resize(FormatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(Device, Surface, &FormatCount, Details.Formats.data());
        }


        uint32_t PresentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(Device, Surface, &PresentModeCount, nullptr);

        if (PresentModeCount != 0)
        {
            Details.PresentModes.resize(PresentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(Device, Surface, &PresentModeCount, Details.PresentModes.data());
        }

        return Details;
    }

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& AvailableFormats)
    {
        for (const auto& AvailableFormat : AvailableFormats)
        {
            if (AvailableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && AvailableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return AvailableFormat;
            }
        }

        return AvailableFormats[0];
    }

    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& AvailablePresentModes)
    {
        for (const auto& AvailablePresentMode : AvailablePresentModes)
        {
            if (AvailablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return AvailablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& Capabilities)
    {
        if (Capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return Capabilities.currentExtent;
        }
        else
        {
            int Width, Height;
            glfwGetFramebufferSize(window, &Width, &Height);

            VkExtent2D ActualExtent = {
                static_cast<uint32_t>(Width),
                static_cast<uint32_t>(Height)
            };

            ActualExtent.width = std::clamp(ActualExtent.width, Capabilities.minImageExtent.width, Capabilities.maxImageExtent.width);
            ActualExtent.height = std::clamp(ActualExtent.height, Capabilities.minImageExtent.height, Capabilities.maxImageExtent.height);

            return ActualExtent;
        }
    }

    void CreateSwapChain()
    {
        SwapChainSupportDetails SwapChainSupport = QuerySwapChainSupport(this->PhysicalDevice);

        SurfaceFormat = ChooseSwapSurfaceFormat(SwapChainSupport.Formats);
        PresentMode = ChooseSwapPresentMode(SwapChainSupport.PresentModes);
        Extent = ChooseSwapExtent(SwapChainSupport.Capabilities);

        uint32_t SwapChainImageCount = SwapChainSupport.Capabilities.minImageCount + 1;

        if (SwapChainSupport.Capabilities.maxImageCount > 0 && SwapChainImageCount > SwapChainSupport.Capabilities.maxImageCount)
        {
            SwapChainImageCount = SwapChainSupport.Capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR SwapChainCreateInfo{};
        SwapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        SwapChainCreateInfo.surface = Surface;
        SwapChainCreateInfo.minImageCount = SwapChainImageCount;
        SwapChainCreateInfo.imageFormat = SurfaceFormat.format;
        SwapChainCreateInfo.imageColorSpace = SurfaceFormat.colorSpace;
        SwapChainCreateInfo.imageExtent = Extent;
        SwapChainCreateInfo.imageArrayLayers = 1;
        SwapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices Indices = FindQueueFamilies(PhysicalDevice);
        uint32_t QueueFamilyIndices[] = { Indices.GraphicsFamily.value(),Indices.PresentFamily.value() };

        if (Indices.GraphicsFamily != Indices.PresentFamily)
        {
            SwapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            SwapChainCreateInfo.queueFamilyIndexCount = 2;
            SwapChainCreateInfo.pQueueFamilyIndices = QueueFamilyIndices;
        }
        else
        {
            SwapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            SwapChainCreateInfo.queueFamilyIndexCount = 0;
            SwapChainCreateInfo.pQueueFamilyIndices = nullptr;
        }

        SwapChainCreateInfo.preTransform = SwapChainSupport.Capabilities.currentTransform;
        SwapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        SwapChainCreateInfo.presentMode = PresentMode;
        SwapChainCreateInfo.clipped = VK_TRUE;
        SwapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(LogicalDevice, &SwapChainCreateInfo, nullptr, &SwapChain) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(LogicalDevice, SwapChain, &SwapChainImageCount, nullptr);
        this->SwapChainImages.resize(SwapChainImageCount);
        vkGetSwapchainImagesKHR(LogicalDevice, SwapChain, &SwapChainImageCount, SwapChainImages.data());
    }

    void CreateImageViews()
    {
        SwapChainImagesViews.resize(SwapChainImages.size());
        for (size_t i = 0; i < SwapChainImages.size(); i++)
        {
            VkImageViewCreateInfo ImageViewCreateInfo{};
            ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            ImageViewCreateInfo.image = SwapChainImages[i];
            ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            ImageViewCreateInfo.format = SurfaceFormat.format;

            ImageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            ImageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            ImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            ImageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            ImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
            ImageViewCreateInfo.subresourceRange.levelCount = 1;
            ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            ImageViewCreateInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(LogicalDevice, &ImageViewCreateInfo, nullptr, &SwapChainImagesViews[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create an image view! id: [" + std::to_string(i) + "]");
            }
        }
    }

    void CreateGraphicsPipeline()
    {
        auto VertexShaderCode = ReadFile("shaders/vert.spv");
        auto FragmentShaderCode = ReadFile("shaders/frag.spv");

        VkShaderModule VertexShaderModule = CreateShaderModule(VertexShaderCode);
        VkShaderModule FragmentShaderModule = CreateShaderModule(FragmentShaderCode);

        VkPipelineShaderStageCreateInfo VertexShaderStageCreateInfo{};
        VertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        VertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        VertexShaderStageCreateInfo.module = VertexShaderModule;
        VertexShaderStageCreateInfo.pName = "main";

        VkPipelineShaderStageCreateInfo FragmentShaderStageCreateInfo{};
        FragmentShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        FragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        FragmentShaderStageCreateInfo.module = FragmentShaderModule;
        FragmentShaderStageCreateInfo.pName = "main";

        VkPipelineShaderStageCreateInfo ShaderStages[] = { VertexShaderStageCreateInfo,FragmentShaderStageCreateInfo };

        std::vector<VkDynamicState> DynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo DynamicStateCreateInfo{};
        DynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        DynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(DynamicStates.size());
        DynamicStateCreateInfo.pDynamicStates = DynamicStates.data();

        VkPipelineVertexInputStateCreateInfo VertexInputCreateInputInfo{};
        VertexInputCreateInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        VertexInputCreateInputInfo.vertexBindingDescriptionCount = 0;
        VertexInputCreateInputInfo.pVertexBindingDescriptions = nullptr;
        VertexInputCreateInputInfo.vertexAttributeDescriptionCount = 0;
        VertexInputCreateInputInfo.pVertexAttributeDescriptions = nullptr;

        VkPipelineInputAssemblyStateCreateInfo InputAssemblyCreateInfo{};
        InputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        InputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        InputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

        VkViewport Viewport{};
        Viewport.x = 0.0f;
        Viewport.y = 0.0f;
        Viewport.width = static_cast<float>(Extent.width);
        Viewport.height = static_cast<float>(Extent.height);
        Viewport.minDepth = 0.0f;
        Viewport.maxDepth = 1.0f;

        VkRect2D Scissor{};
        Scissor.offset = { 0,0 };
        Scissor.extent = Extent;

        VkPipelineViewportStateCreateInfo ViewportStateCreateInfo{};
        ViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        ViewportStateCreateInfo.viewportCount = 1;
        ViewportStateCreateInfo.pViewports = &Viewport;
        ViewportStateCreateInfo.scissorCount = 1;
        ViewportStateCreateInfo.pScissors = &Scissor;

        VkPipelineRasterizationStateCreateInfo RasterizerStateCreateInfo{};
        RasterizerStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        RasterizerStateCreateInfo.depthClampEnable = VK_FALSE;
        RasterizerStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
        RasterizerStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
        RasterizerStateCreateInfo.lineWidth = 1.0f;
        RasterizerStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        RasterizerStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
        RasterizerStateCreateInfo.depthBiasEnable = VK_FALSE;
        RasterizerStateCreateInfo.depthBiasConstantFactor = 0.0f;
        RasterizerStateCreateInfo.depthBiasClamp = 0.0f;
        RasterizerStateCreateInfo.depthBiasSlopeFactor = 0.0f;

        VkPipelineMultisampleStateCreateInfo MultiSamplingCreateInfo{};
        MultiSamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        MultiSamplingCreateInfo.sampleShadingEnable = VK_FALSE;
        MultiSamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        MultiSamplingCreateInfo.minSampleShading = 1.0f;
        MultiSamplingCreateInfo.pSampleMask = nullptr;
        MultiSamplingCreateInfo.alphaToCoverageEnable = VK_FALSE;
        MultiSamplingCreateInfo.alphaToOneEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState ColorBlendAttachment{};
        ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        ColorBlendAttachment.blendEnable = VK_TRUE;
        ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo ColorBlendStateCreateInfo{};
        ColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        ColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
        ColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
        ColorBlendStateCreateInfo.attachmentCount = 1;
        ColorBlendStateCreateInfo.pAttachments = &ColorBlendAttachment;
        ColorBlendStateCreateInfo.blendConstants[0] = 0.0f;
        ColorBlendStateCreateInfo.blendConstants[1] = 0.0f;
        ColorBlendStateCreateInfo.blendConstants[2] = 0.0f;
        ColorBlendStateCreateInfo.blendConstants[3] = 0.0f;

        VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo{};
        PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        PipelineLayoutCreateInfo.setLayoutCount = 0;
        PipelineLayoutCreateInfo.pSetLayouts = nullptr;
        PipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        PipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

        if (vkCreatePipelineLayout(LogicalDevice, &PipelineLayoutCreateInfo, nullptr, &PipelineLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo PipelineCreateInfo{};
        PipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        PipelineCreateInfo.stageCount = 2;
        PipelineCreateInfo.pStages = ShaderStages;
        PipelineCreateInfo.pVertexInputState = &VertexInputCreateInputInfo;
        PipelineCreateInfo.pInputAssemblyState = &InputAssemblyCreateInfo;
        PipelineCreateInfo.pViewportState = &ViewportStateCreateInfo;
        PipelineCreateInfo.pRasterizationState = &RasterizerStateCreateInfo;
        PipelineCreateInfo.pMultisampleState = &MultiSamplingCreateInfo;
        PipelineCreateInfo.pDepthStencilState = nullptr;
        PipelineCreateInfo.pColorBlendState = &ColorBlendStateCreateInfo;
        PipelineCreateInfo.pDynamicState = &DynamicStateCreateInfo;
        PipelineCreateInfo.layout = PipelineLayout;
        PipelineCreateInfo.renderPass = RenderPass;
        PipelineCreateInfo.subpass = 0;

        PipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        PipelineCreateInfo.basePipelineIndex = -1;

        if (vkCreateGraphicsPipelines(LogicalDevice, VK_NULL_HANDLE, 1, &PipelineCreateInfo, nullptr, &GraphicsPipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("Error creating the graphics pipeline!");
        }

        vkDestroyShaderModule(LogicalDevice, VertexShaderModule, nullptr);
        vkDestroyShaderModule(LogicalDevice, FragmentShaderModule, nullptr);
    }

    VkShaderModule CreateShaderModule(const std::vector<char>& CodeSource)
    {
        VkShaderModuleCreateInfo ShaderModuleCreateInfo{};
        ShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        ShaderModuleCreateInfo.codeSize = CodeSource.size();
        ShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(CodeSource.data());

        VkShaderModule ShaderModule;
        if (vkCreateShaderModule(LogicalDevice, &ShaderModuleCreateInfo, nullptr, &ShaderModule) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create a shader module!");
        }
        return ShaderModule;
    }

    void CreateRenderPass()
    {
        VkAttachmentDescription ColorAttachment{};
        ColorAttachment.format = SurfaceFormat.format;
        ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        ColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference ColorAttachmentRef{};
        ColorAttachmentRef.attachment = 0;
        ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription Subpass{};
        Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        Subpass.colorAttachmentCount = 1;
        Subpass.pColorAttachments = &ColorAttachmentRef;

        VkSubpassDependency ExternalDependency{};
        ExternalDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        ExternalDependency.dstSubpass = 0;
        ExternalDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        ExternalDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        ExternalDependency.srcAccessMask = 0;
        ExternalDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo RenderPassCreateInfo{};
        RenderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        RenderPassCreateInfo.attachmentCount = 1;
        RenderPassCreateInfo.pAttachments = &ColorAttachment;
        RenderPassCreateInfo.subpassCount = 1;
        RenderPassCreateInfo.pSubpasses = &Subpass;
        RenderPassCreateInfo.dependencyCount = 1;
        RenderPassCreateInfo.pDependencies = &ExternalDependency;

        if (vkCreateRenderPass(LogicalDevice, &RenderPassCreateInfo, nullptr, &RenderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create a render pass!");
        }
    }

    void CreateFramebuffers()
    {
        SwapChainFramebuffers.resize(SwapChainImagesViews.size());
        for (size_t i = 0; i < SwapChainImagesViews.size(); i++)
        {
            VkImageView Attachments[] = {
                SwapChainImagesViews[i]
            };

            VkFramebufferCreateInfo FramebufferCreateInfo{};
            FramebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            FramebufferCreateInfo.renderPass = RenderPass;
            FramebufferCreateInfo.attachmentCount = 1;
            FramebufferCreateInfo.pAttachments = Attachments;
            FramebufferCreateInfo.width = Extent.width;
            FramebufferCreateInfo.height = Extent.height;
            FramebufferCreateInfo.layers = 1;

            if (vkCreateFramebuffer(LogicalDevice, &FramebufferCreateInfo, nullptr, &this->SwapChainFramebuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create a swap chain framebuffer!");
            }
        }
    }

    void CreateCommandPool()
    {
        QueueFamilyIndices m_QueueFamilyIndices = FindQueueFamilies(PhysicalDevice);


        VkCommandPoolCreateInfo CommandPoolCreateInfo{};
        CommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        CommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        CommandPoolCreateInfo.queueFamilyIndex = m_QueueFamilyIndices.GraphicsFamily.value();

        if (vkCreateCommandPool(LogicalDevice, &CommandPoolCreateInfo, nullptr, &CommandPool) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create commandpool");
        }
    }

    void CreateCommandBuffer()
    {
        VkCommandBufferAllocateInfo AllocCreateInfo{};
        AllocCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        AllocCreateInfo.commandPool = CommandPool;
        AllocCreateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        AllocCreateInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(LogicalDevice, &AllocCreateInfo, &CommandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate command buffers");
        }

    }

    void RecordCommandBuffer(VkCommandBuffer CommandBuffer, uint32_t ImageIndex)
    {
        VkCommandBufferBeginInfo CommandBufferBeginInfo{};
        CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        CommandBufferBeginInfo.flags = 0;
        CommandBufferBeginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to record command buffer");
        }

        VkRenderPassBeginInfo RenderPassBeginInfo{};
        RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        RenderPassBeginInfo.renderPass = RenderPass;
        RenderPassBeginInfo.framebuffer = SwapChainFramebuffers[ImageIndex];
        RenderPassBeginInfo.renderArea.offset = { 0,0 };
        RenderPassBeginInfo.renderArea.extent = Extent;

        VkClearValue ClearColor = { {{0.0f,0.0f,0.0f,1.0f}} };
        RenderPassBeginInfo.clearValueCount = 1;
        RenderPassBeginInfo.pClearValues = &ClearColor;

        vkCmdBeginRenderPass(CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GraphicsPipeline);

        VkViewport Viewport{};
        Viewport.x = 0.0f;
        Viewport.y = 0.0f;
        Viewport.width = static_cast<float>(Extent.width);
        Viewport.height = static_cast<float>(Extent.height);
        Viewport.minDepth = 0.0f;
        Viewport.maxDepth = 1.0f;
        vkCmdSetViewport(CommandBuffer, 0, 1, &Viewport);

        VkRect2D Scissor{};
        Scissor.offset = { 0,0 };
        Scissor.extent = Extent;
        vkCmdSetScissor(CommandBuffer, 0, 1, &Scissor);
        vkCmdDraw(CommandBuffer, 3, 1, 0, 0);
        vkCmdEndRenderPass(CommandBuffer);
        if (vkEndCommandBuffer(CommandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to record command buffer");
        }
    }

    void CreateSyncObjects()
    {
        VkSemaphoreCreateInfo SemaphoreCreateInfo{};
        SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo FenceCreateInfo{};
        FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateSemaphore(LogicalDevice, &SemaphoreCreateInfo, nullptr, &ImageAvailableSemophore) != VK_SUCCESS ||
            vkCreateSemaphore(LogicalDevice, &SemaphoreCreateInfo, nullptr, &RenderFinishedSemophore) != VK_SUCCESS ||
            vkCreateFence(LogicalDevice, &FenceCreateInfo, nullptr, &InFlightFence) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create the semaphores and the fence!");
        }
    }

    void DrawFrame()
    {
        vkWaitForFences(LogicalDevice, 1, &this->InFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(LogicalDevice, 1, &InFlightFence);

        uint32_t ImageIndex;
        vkAcquireNextImageKHR(LogicalDevice, SwapChain, UINT64_MAX, ImageAvailableSemophore, VK_NULL_HANDLE, &ImageIndex);

        vkResetCommandBuffer(CommandBuffer, 0);
        RecordCommandBuffer(CommandBuffer, ImageIndex);

        VkSubmitInfo SubmitInfo{};
        SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore WaitSemaphores[] = { ImageAvailableSemophore };
        VkPipelineStageFlags WaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        SubmitInfo.waitSemaphoreCount = 1;
        SubmitInfo.pWaitSemaphores = WaitSemaphores;
        SubmitInfo.pWaitDstStageMask = WaitStages;
        SubmitInfo.commandBufferCount = 1;
        SubmitInfo.pCommandBuffers = &CommandBuffer;

        VkSemaphore SignalSemaphores[] = { RenderFinishedSemophore };
        SubmitInfo.signalSemaphoreCount = 1;
        SubmitInfo.pSignalSemaphores = SignalSemaphores;

        if (vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, InFlightFence) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to submit draw command buffer!");
        }

        VkPresentInfoKHR PresentInfo{};
        PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        PresentInfo.waitSemaphoreCount = 1;
        PresentInfo.pWaitSemaphores = SignalSemaphores;

        VkSwapchainKHR SwapChains[] = { SwapChain };
        PresentInfo.swapchainCount = 1;
        PresentInfo.pSwapchains = SwapChains;
        PresentInfo.pImageIndices = &ImageIndex;
        PresentInfo.pResults = nullptr;
        vkQueuePresentKHR(PresentQueue, &PresentInfo);
    }
};

int main() {
    
    HelloWorldTriangle App;

    try
    {
        App.Run();
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

