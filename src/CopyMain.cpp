/*
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
#include <sstream>

#ifdef NDEBUG
     const bool EnableValidationLayers = false;
#else
     const bool EnableValidationLayers = true;
#endif

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                           VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto Func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (Func)
    {
        return Func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    auto Func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (Func)
    {
        Func(instance, debugMessenger, pAllocator);
    }
}

std::string ReadFile(const char* FileName)
{
    std::ifstream File(FileName, std::ios::binary);
    std::stringstream Buffer;

    if (File.is_open())
    {
        Buffer << File.rdbuf();
        File.close();
    }

    return Buffer.str();
}

struct QueueFamilyIndices {
    std::optional<uint32_t> GraphicsFamily;
    std::optional<uint32_t> PresentFamily;

    bool isComplete()
    {
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

    const std::vector<const char*> ValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> DeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    void Run()
    {
        InitWindow();
        InitVulkan();
        MainLoop();
        CleanUp();
    }
    HelloWorldTriangle();
    ~HelloWorldTriangle();

private:

    void InitWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WindowInitialWidth, WindowInitialHeight, "Vulkan", nullptr, nullptr);
    };
    void InitVulkan() {
        CreateInstance();
        SetupDebugMessenger();
    };
    void MainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    };
    void CleanUp()
    {
        if (EnableValidationLayers)
        {
            DestroyDebugUtilsMessengerEXT(Instance, DebugMessenger, nullptr);
        }

        vkDestroyInstance(Instance, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
    };

    void CreateInstance() {
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

        VkInstanceCreateInfo InstanceCreateInfo{};
        InstanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        InstanceCreateInfo.pApplicationInfo = &AppInfo;

        uint32_t GLFWextensionCount = 0;
        const char** GLFWextentionsString;

        GLFWextentionsString = glfwGetRequiredInstanceExtensions(&GLFWextensionCount);
        
        std::vector<const char*> GLFWrequiredExtensions;
        GLFWrequiredExtensions.resize(GLFWextensionCount);

        for (size_t i = 0; i < GLFWextensionCount; i++)
        {
            GLFWrequiredExtensions[i] = *(GLFWextentionsString + i);
            std::cout << *(GLFWextentionsString + i) << std::endl;
        }

        if (EnableValidationLayers)
        {
            GLFWrequiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            GLFWextensionCount++;
        }

        InstanceCreateInfo.enabledExtensionCount = GLFWextensionCount;
        InstanceCreateInfo.ppEnabledExtensionNames = GLFWrequiredExtensions.data();

        VkDebugUtilsMessengerCreateInfoEXT DebugCreateInfo{};
        if (EnableValidationLayers)
        {
            InstanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
            InstanceCreateInfo.ppEnabledLayerNames = ValidationLayers.data();


        }
        else
        {
            InstanceCreateInfo.enabledLayerCount = 0;
            InstanceCreateInfo.pNext = nullptr;
        }

        CheckRequiredExtensionsAvailability(GLFWrequiredExtensions, GLFWextensionCount);

        if (vkCreateInstance(&InstanceCreateInfo, nullptr, &Instance) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create an instance!");
        }
    };

    void CheckRequiredExtensionsAvailability(std::vector<const char*>& GLFWrequiredExtensions,const uint32_t &GLFWextensionCount)
    {
        uint32_t ExtensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, nullptr);

        std::vector<VkExtensionProperties> Extensions(ExtensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, Extensions.data());

        unsigned int FoundExtensionCount = 0;
        for (const auto& Extension : Extensions)
        {
            for (const auto& RequiredExtension : GLFWrequiredExtensions)
            {
                if (strcmp(RequiredExtension, Extension.extensionName) == 0)
                {
                    FoundExtensionCount++;
                }
            }
        }

        if (FoundExtensionCount != GLFWextensionCount)
        {
            std::cout << "Some of the required extensions seems to be missing!" << std::endl;
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
            bool LayerFound = false;
            for (const auto& AvailableLayer : AvailableLayers)
            {
                if (strcmp(AvailableLayer.layerName, LayerName) == 0)
                {
                    LayerFound = true;
                    break;
                }
            }

            if (!LayerFound) return false;
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

    void PopulateMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& CreateInfo)
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
        PopulateMessengerCreateInfo(CreateInfo);

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

        std::multimap<int, VkPhysicalDevice> Candidates;

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
        QueueFamilyIndices Indices = FindQueueFamilies(Device);

        bool IsExtensionSupported = CheckDeviceExtensionSupport(Device);

        Score *= static_cast<int>(Indices.GraphicsFamily.has_value());
        Score *= static_cast<int>(IsExtensionSupported);

        if (IsExtensionSupported)
        {

        }

        return Score;
    }

    bool CheckDeviceExtensionSupport(VkPhysicalDevice Device)
    {
        uint32_t ExtensionCount;
        vkEnumerateDeviceExtensionProperties(Device, nullptr, &ExtensionCount, nullptr);
        std::vector<VkExtensionProperties> AvailableDeviceExtensions(ExtensionCount);
        vkEnumerateDeviceExtensionProperties(Device, nullptr, &ExtensionCount, AvailableDeviceExtensions.data());

        std::set RequiredDeviceExtensions(DeviceExtensions.begin(), DeviceExtensions.end());

        for (const auto& Extension : AvailableDeviceExtensions)
        {
            RequiredDeviceExtensions.erase(Extension.extensionName);
        }

        return RequiredDeviceExtensions.empty();
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
        QueueFamilyIndices Indices = FindQueueFamilies(PhysicalDevice);

        std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;
        std::set<uint32_t> UniqueQueueFamilies = { Indices.GraphicsFamily.value(),Indices.PresentFamily.value() };
        
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

        vkGetDeviceQueue(LogicalDevice, Indices.GraphicsFamily.value(), 0, &GraphicsQueue);
        vkGetDeviceQueue(LogicalDevice, Indices.PresentFamily.value(), 0, &PresentQueue);
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

    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& AvilablePresentModes)
    {
        for (const auto& AvailablePresentMode : AvilablePresentModes)
        {
            if (AvailablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return AvailablePresentMode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D ChooseSwapExtend(const VkSurfaceCapabilitiesKHR& Capabilities)
    {
        if (Capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return Capabilities.currentExtent;
        }
        else
        {
            int Width, Height;
            glfwGetFramebufferSize(window, &Width, &Height);

            VkExtent2D ActualExtend = {
                static_cast<uint32_t>(Width),
                static_cast<uint32_t>(Height)
            };

            ActualExtend.width = std::clamp(ActualExtend.width, Capabilities.minImageExtent.width, Capabilities.maxImageExtent.width);
            ActualExtend.height = std::clamp(ActualExtend.height, Capabilities.minImageExtent.height, Capabilities.maxImageExtent.height);
            return ActualExtend;
        }
    }
};
*/
