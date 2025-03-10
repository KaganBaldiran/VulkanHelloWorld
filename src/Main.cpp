#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <optional>

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
    std::optional<uint32_t> graphicsFamily;

    bool isComplete() {
        return graphicsFamily.has_value();
    }
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

    const std::vector<const char*> ValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
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
        PickPhysicalDevice();
        CreateLogicalDevice();
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
        vkDestroyDevice(LogicalDevice, nullptr);
        if (EnableValidationLayers)
        {
            DestroyDebugUtilsMessengerEXT(Instance, DebugMessenger, nullptr);
        }

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
    }

    bool CheckDeviceSuitability(VkPhysicalDevice Device)
    {
        VkPhysicalDeviceProperties DeviceProperties;
        vkGetPhysicalDeviceProperties(Device, &DeviceProperties);
        std::cout << "Device: " << DeviceProperties.deviceName << std::endl;

        int Score = 0;

        if (DeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            Score += 1000;
        }

        Score += DeviceProperties.limits.maxImageDimension2D;
        QueueFamilyIndices indices = FindQueueFamilies(Device);

        Score *= static_cast<int>(indices.graphicsFamily.has_value());

        return Score;
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
            if (QueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                Indices.graphicsFamily = i;
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

        VkDeviceQueueCreateInfo QueueCreateInfo{};
        QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        QueueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
        QueueCreateInfo.queueCount = 1;

        float QueuePriority = 1.0f;
        QueueCreateInfo.pQueuePriorities = &QueuePriority;

        VkPhysicalDeviceFeatures DeviceFeatures{};

        VkDeviceCreateInfo DeviceCreateInfo{};
        DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        DeviceCreateInfo.pQueueCreateInfos = &QueueCreateInfo;
        DeviceCreateInfo.queueCreateInfoCount = 1;
        DeviceCreateInfo.pEnabledFeatures = &DeviceFeatures;

        DeviceCreateInfo.enabledExtensionCount = 0;

        if (vkCreateDevice(PhysicalDevice, &DeviceCreateInfo, nullptr, &LogicalDevice) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create the logical device!");
        }

        vkGetDeviceQueue(LogicalDevice, indices.graphicsFamily.value(), 0, &GraphicsQueue);
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

