#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <string>
#include <algorithm>

#ifdef NDEBUG
    const bool EnableValidationLayers = false;
#else 
    const bool EnableValidationLayers = true;
#endif // !NDEBUG

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

        uint32_t GLFExtensionsCount = 0;
        const char** GLFExtensions;

        GLFExtensions = glfwGetRequiredInstanceExtensions(&GLFExtensionsCount);

        std::vector<const char*> GLFWRequiredExtensions;
        GLFWRequiredExtensions.resize(GLFExtensionsCount);

        for (size_t i = 0; i < GLFExtensionsCount; i++)
        {
            GLFWRequiredExtensions[i] = *(GLFExtensions + i);
            std::cout << *(GLFExtensions + i) << std::endl;
        }

        if (EnableValidationLayers) {
            GLFWRequiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        CreateInfo.enabledExtensionCount = GLFExtensionsCount;
        CreateInfo.ppEnabledExtensionNames = GLFWRequiredExtensions.data();
        if (EnableValidationLayers)
        {
            CreateInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
            CreateInfo.ppEnabledLayerNames = ValidationLayers.data();
        }
        else CreateInfo.enabledLayerCount = 0;
        
        uint32_t ExtensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, nullptr);

        std::vector<VkExtensionProperties> Extensions(ExtensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, Extensions.data());

        std::cout << "Available extensions: " << std::endl;

        unsigned int FoundExtensionsCount = 0;
        for (const auto& extension : Extensions)
        {
            if (std::find(GLFWRequiredExtensions.begin(), GLFWRequiredExtensions.end(), extension.extensionName) != GLFWRequiredExtensions.end())
            {
                FoundExtensionsCount++;
            }
            std::cout << '\t' << extension.extensionName << std::endl;
        }

        if (FoundExtensionsCount != GLFExtensionsCount)
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

