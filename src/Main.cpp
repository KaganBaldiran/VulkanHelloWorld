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
    }

    void MainLoop()
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
        }
    }

    void CleanUp()
    {
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
        AppInfo.applicationVersion = VK_MAKE_API_VERSION(0,1,0,0);
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
            CreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &DebugCreateInfo;
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
            std::cout <<"Some of the required extensions seems to be missing!" << std::endl;
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
                if(strcmp(AvailableLayer.layerName, LayerName) == 0)
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
            ImageViewCreateInfo.subresourceRange.aspectMask = 0;
            ImageViewCreateInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(LogicalDevice, &ImageViewCreateInfo, nullptr, &SwapChainImagesViews[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create an image view! id: [" + std::to_string(i) + "]");
            }
        }
    }

    void CreateGraphicsPipeline()
    {

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

