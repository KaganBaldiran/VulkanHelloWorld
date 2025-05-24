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
#include <array>
#include <queue>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <functional>

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stbi/stb_image.h"

#include <assimp/Importer.hpp>      
#include <assimp/scene.h>           
#include <assimp/postprocess.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

const int MAX_FRAMES_IN_FLIGHT = 2;

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

struct Vertex2D {
    glm::vec2 Position;
    glm::vec3 Color;
    glm::vec2 UV;

    static VkVertexInputBindingDescription GetBindingDescription()
    {
        VkVertexInputBindingDescription BindingDescription;
        BindingDescription.binding = 0;
        BindingDescription.stride = sizeof(Vertex2D);
        BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return BindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 3> AttributeDescriptions{};
        AttributeDescriptions[0].binding = 0;
        AttributeDescriptions[0].location = 0;
        AttributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        AttributeDescriptions[0].offset = offsetof(Vertex2D, Position);

        AttributeDescriptions[1].binding = 0;
        AttributeDescriptions[1].location = 1;
        AttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[1].offset = offsetof(Vertex2D, Color);

        AttributeDescriptions[2].binding = 0;
        AttributeDescriptions[2].location = 2;
        AttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        AttributeDescriptions[2].offset = offsetof(Vertex2D, UV);

        return AttributeDescriptions;
    }
};

struct Vertex3D {
    glm::vec3 Position;
    glm::vec3 Color;
    glm::vec2 UV;
    glm::vec3 Normal;

    static VkVertexInputBindingDescription GetBindingDescription()
    {
        VkVertexInputBindingDescription BindingDescription;
        BindingDescription.binding = 0;
        BindingDescription.stride = sizeof(Vertex3D);
        BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return BindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 4> GetAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 4> AttributeDescriptions{};
        AttributeDescriptions[0].binding = 0;
        AttributeDescriptions[0].location = 0;
        AttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[0].offset = offsetof(Vertex3D, Position);

        AttributeDescriptions[1].binding = 0;
        AttributeDescriptions[1].location = 1;
        AttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[1].offset = offsetof(Vertex3D, Color);

        AttributeDescriptions[2].binding = 0;
        AttributeDescriptions[2].location = 2;
        AttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        AttributeDescriptions[2].offset = offsetof(Vertex3D, UV);

        AttributeDescriptions[3].binding = 0;
        AttributeDescriptions[3].location = 3;
        AttributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[3].offset = offsetof(Vertex3D, Normal);
        return AttributeDescriptions;
    }
};

struct Mesh
{
    std::vector<Vertex3D> Vertices;
    std::vector<uint32_t> Indices;
};

struct Model3D
{
    std::vector<Mesh> Meshes;

    void GetCombinedVerticesIndicesCount(uint32_t& VertexCount, uint32_t& IndexCount)
    {
        VertexCount = IndexCount = 0;
        for (auto& Mesh : Meshes)
        {
            VertexCount += Mesh.Vertices.size();
            IndexCount += Mesh.Indices.size();
        }
    }

    void GetCombinedVerticesIndices(std::vector<Vertex3D>& DstCombinedVertices, std::vector<uint32_t>& DstCombinedIndices)
    {
        for (auto& Mesh : Meshes)
        {
            DstCombinedVertices.insert(DstCombinedVertices.end(), Mesh.Vertices.begin(), Mesh.Vertices.end());
            DstCombinedIndices.insert(DstCombinedIndices.end(), Mesh.Indices.begin(), Mesh.Indices.end());
        }
    }
};

void Import3Dmodel(const char* FilePath, Model3D& DstModel)
{
    Assimp::Importer Importer;
    const aiScene* scene = Importer.ReadFile(FilePath,
        aiProcess_CalcTangentSpace |
        aiProcess_Triangulate |
        aiProcess_SortByPType |
        aiProcess_PreTransformVertices |
        aiProcess_GenSmoothNormals);
    aiScene* Scene = const_cast<aiScene*>(scene);

    if (nullptr == Scene) {
        std::cout << "Error code :: " << Importer.GetErrorString() << std::endl;
        throw std::runtime_error("Unable to import a 3D model(" + std::string(FilePath) + ")");
    }

    if (!Scene->HasMeshes()) return;

    std::queue<aiNode*> NodesToProcess;
    NodesToProcess.push(Scene->mRootNode);
    aiNode* Node = nullptr;
    while (!NodesToProcess.empty())
    {
        Node = NodesToProcess.front();
        NodesToProcess.pop();
        for (size_t MeshIndex = 0; MeshIndex < Node->mNumMeshes; MeshIndex++)
        {
            Mesh NewMesh;
            auto& aiMesh = Scene->mMeshes[Node->mMeshes[MeshIndex]];
            NewMesh.Vertices.reserve(aiMesh->mNumVertices);
            for (size_t VertexIndex = 0; VertexIndex < aiMesh->mNumVertices; VertexIndex++)
            {
                Vertex3D Vertex;
                auto& AiVertexPosition = aiMesh->mVertices[VertexIndex];
                Vertex.Position = { AiVertexPosition.x , AiVertexPosition.y , AiVertexPosition.z };

                if (aiMesh->HasNormals())
                {
                    auto& AiVertexNormal = aiMesh->mNormals[VertexIndex];
                    Vertex.Normal = { AiVertexNormal.x , AiVertexNormal.y , AiVertexNormal.z };
                }

                if (aiMesh->HasTextureCoords(0))
                {
                    auto& AiVertexTextCoords = aiMesh->mTextureCoords[0][VertexIndex];
                    Vertex.UV = { AiVertexTextCoords.x , AiVertexTextCoords.y };
                }
                NewMesh.Vertices.push_back(Vertex);
            }

            NewMesh.Indices.reserve(aiMesh->mNumFaces * 3);
            for (size_t FaceIndex = 0; FaceIndex < aiMesh->mNumFaces; FaceIndex++)
            {
                auto& Face = aiMesh->mFaces[FaceIndex];
                for (size_t Index = 0; Index < Face.mNumIndices; Index++)
                {
                    NewMesh.Indices.push_back(Face.mIndices[Index]);
                }
            }
            DstModel.Meshes.push_back(NewMesh);
        }

        for (size_t i = 0; i < Node->mNumChildren; i++)
        {
            NodesToProcess.push(*(Node->mChildren + i));
        }
    }
}

const std::vector<Vertex3D> Vertices = {
    {{-0.5f, -0.5f,0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f},{1.0f,1.0f,1.0f}},
    {{0.5f, -0.5f,0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f},{1.0f,1.0f,1.0f}},
    {{0.5f, 0.5f,0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f},{1.0f,1.0f,1.0f}},
    {{-0.5f, 0.5f,0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f},{1.0f,1.0f,1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};

struct Matrixes {
    glm::mat4 ModelMatrix;
    glm::mat4 ViewMatrix;
    glm::mat4 ProjectionMatrix;
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

    VkDescriptorSetLayout DescriptorSetLayout;
    VkPipelineLayout PipelineLayout;

    VkPipeline GraphicsPipeline;

    VkCommandPool CommandPool;

    std::vector<VkCommandBuffer> CommandBuffers;

    std::vector<VkSemaphore> ImageAvailableSemophores;
    std::vector<VkSemaphore> RenderFinishedSemophores;
    std::vector<VkFence> InFlightFences;

    VkBuffer VertexBuffer;
    VkDeviceMemory VertexBufferMemory;

    VkBuffer IndexBuffer;
    VkDeviceMemory IndexBufferMemory;

    VkDescriptorPool DescriptorPool;
    std::vector<VkDescriptorSet> DescriptorSets;

    std::vector<VkBuffer> UniformBuffers;
    std::vector<VkDeviceMemory> UniformBuffersMemory;
    std::vector<void*> UniformBuffersMapped;

    bool FrameBufferResized = false;
    uint32_t CurrentFrame = 0;

    std::vector<VkFramebuffer> SwapChainFramebuffers;

    VkImage TextureImage;
    VkDeviceMemory TextureImageMemory;
    VkImageView TextureImageView;
    VkSampler TextureSampler;

    VkImage DepthBufferImage;
    VkDeviceMemory DepthBufferImageMemory;
    VkImageView DepthBufferImageView;
    VkFormat DepthImageFormat;

    Model3D Model;

    const std::vector<const char*> ValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> DeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
    };

    void InitWindow()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(WindowInitialWidth, WindowInitialHeight, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);
    }

    static void FramebufferResizeCallback(GLFWwindow* window, int width, int height)
    {
        auto App = reinterpret_cast<HelloWorldTriangle*>(glfwGetWindowUserPointer(window));
        App->FrameBufferResized = true;
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
        CreateDepthBufferResources();
        //CreateRenderPass();
        CreateCommandPool();
        CreateTextureImage("resources\\image.png");
        CreateDescriptorSetLayout();
        CreateDescriptorPool();
        CreateUniformBuffers();
        CreateDescriptorSets();
        CreateGraphicsPipeline();
        //CreateFramebuffers();
        CreateCommandBuffer();
        Import3Dmodel("resources\\Shovel2.obj", Model);
        CreateVertexBuffer();
        CreateIndexBuffer();
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
        CleanupSwapChain();

        vkDestroySampler(LogicalDevice, TextureSampler, nullptr);
        vkDestroyImageView(LogicalDevice, TextureImageView, nullptr);

        vkDestroyImage(LogicalDevice, TextureImage, nullptr);
        vkFreeMemory(LogicalDevice, TextureImageMemory, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroyBuffer(LogicalDevice, UniformBuffers[i], nullptr);
            vkFreeMemory(LogicalDevice, UniformBuffersMemory[i], nullptr);
        }

        vkDestroyDescriptorPool(LogicalDevice, DescriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(LogicalDevice, DescriptorSetLayout, nullptr);

        vkDestroyBuffer(LogicalDevice, IndexBuffer, nullptr);
        vkFreeMemory(LogicalDevice, IndexBufferMemory, nullptr);

        vkDestroyBuffer(LogicalDevice, VertexBuffer, nullptr);
        vkFreeMemory(LogicalDevice, VertexBufferMemory, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(LogicalDevice, RenderFinishedSemophores[i], nullptr);
            vkDestroySemaphore(LogicalDevice, ImageAvailableSemophores[i], nullptr);
            vkDestroyFence(LogicalDevice, InFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(LogicalDevice, CommandPool, nullptr);

        /*for (auto Framebuffer : SwapChainFramebuffers)
        {
            vkDestroyFramebuffer(LogicalDevice, Framebuffer, nullptr);
        }*/
        vkDestroyPipeline(LogicalDevice, GraphicsPipeline, nullptr);
        vkDestroyPipelineLayout(LogicalDevice, PipelineLayout, nullptr);
        //vkDestroyRenderPass(LogicalDevice, RenderPass, nullptr);

        /*for (auto ImageView : SwapChainImagesViews)
        {
            vkDestroyImageView(LogicalDevice, ImageView, nullptr);
        }

        vkDestroySwapchainKHR(LogicalDevice, SwapChain, nullptr);*/
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
        AppInfo.apiVersion = VK_API_VERSION_1_3;

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
            GLFWRequiredExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
            GLFWextensionsCount += 2;
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
        DeviceFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo DeviceCreateInfo{};
        DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        DeviceCreateInfo.pQueueCreateInfos = QueueCreateInfos.data();
        DeviceCreateInfo.queueCreateInfoCount = QueueCreateInfos.size();
        DeviceCreateInfo.pEnabledFeatures = &DeviceFeatures;

        DeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(DeviceExtensions.size());
        DeviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();

        VkPhysicalDeviceDynamicRenderingFeaturesKHR DynamicRenderingFeatures{};
        DynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
        DynamicRenderingFeatures.dynamicRendering = VK_TRUE;

        DeviceCreateInfo.pNext = &DynamicRenderingFeatures;

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
            SwapChainImagesViews[i] = CreateImageView(SwapChainImages[i], SurfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
        }
    }

    int CompileGLSL(const std::string& SourceFileName, const std::string& DestinationFileName)
    {
        auto Command = std::string(".\\shaders\\compile.bat .\\shaders\\") + SourceFileName + std::string(" .\\shaders\\") + DestinationFileName;
        int Result = std::system(Command.c_str());
        if (Result != 0)
        {
            throw std::runtime_error("Failed to compile glsl into spir-v");
        }
    }

    void CreateGraphicsPipeline()
    {
        CompileGLSL("09_shader_base.vert", "vert.spv");
        CompileGLSL("09_shader_base.frag", "frag.spv");

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

        auto BindingDescription = Vertex3D::GetBindingDescription();
        auto AttributeDescription = Vertex3D::GetAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo VertexInputCreateInputInfo{};
        VertexInputCreateInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        VertexInputCreateInputInfo.vertexBindingDescriptionCount = 1;
        VertexInputCreateInputInfo.pVertexBindingDescriptions = &BindingDescription;
        VertexInputCreateInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(AttributeDescription.size());
        VertexInputCreateInputInfo.pVertexAttributeDescriptions = AttributeDescription.data();

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
        RasterizerStateCreateInfo.cullMode = VK_CULL_MODE_NONE;
        RasterizerStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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

        VkPipelineDepthStencilStateCreateInfo DepthStencilStateCreateInfo{};
        DepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        DepthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
        DepthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
        DepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        DepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
        DepthStencilStateCreateInfo.minDepthBounds = 0.0f;
        DepthStencilStateCreateInfo.maxDepthBounds = 1.0f;
        DepthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
        DepthStencilStateCreateInfo.front = {};
        DepthStencilStateCreateInfo.back = {};

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
        PipelineLayoutCreateInfo.setLayoutCount = 1;
        PipelineLayoutCreateInfo.pSetLayouts = &DescriptorSetLayout;
        PipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        PipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

        if (vkCreatePipelineLayout(LogicalDevice, &PipelineLayoutCreateInfo, nullptr, &PipelineLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create pipeline layout!");
        }

        VkPipelineRenderingCreateInfo RenderingCreateInfo{};
        RenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        RenderingCreateInfo.colorAttachmentCount = 1;
        RenderingCreateInfo.pColorAttachmentFormats = &this->SurfaceFormat.format;
        RenderingCreateInfo.depthAttachmentFormat = DepthImageFormat;

        VkGraphicsPipelineCreateInfo PipelineCreateInfo{};
        PipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        PipelineCreateInfo.stageCount = 2;
        PipelineCreateInfo.pStages = ShaderStages;
        PipelineCreateInfo.pVertexInputState = &VertexInputCreateInputInfo;
        PipelineCreateInfo.pInputAssemblyState = &InputAssemblyCreateInfo;
        PipelineCreateInfo.pViewportState = &ViewportStateCreateInfo;
        PipelineCreateInfo.pRasterizationState = &RasterizerStateCreateInfo;
        PipelineCreateInfo.pMultisampleState = &MultiSamplingCreateInfo;
        PipelineCreateInfo.pDepthStencilState = &DepthStencilStateCreateInfo;
        PipelineCreateInfo.pColorBlendState = &ColorBlendStateCreateInfo;
        PipelineCreateInfo.pDynamicState = &DynamicStateCreateInfo;
        PipelineCreateInfo.layout = PipelineLayout;
        PipelineCreateInfo.renderPass = VK_NULL_HANDLE;
        PipelineCreateInfo.subpass = 0;
        PipelineCreateInfo.pNext = &RenderingCreateInfo;

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

        VkAttachmentDescription DepthAttachment{};
        DepthAttachment.format = DepthImageFormat;
        DepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        DepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        DepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        DepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        DepthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference ColorAttachmentRef{};
        ColorAttachmentRef.attachment = 0;
        ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference DepthAttachmentRef;
        DepthAttachmentRef.attachment = 1;
        DepthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription Subpass{};
        Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        Subpass.colorAttachmentCount = 1;
        Subpass.pColorAttachments = &ColorAttachmentRef;
        Subpass.pDepthStencilAttachment = &DepthAttachmentRef;

        VkSubpassDependency ExternalDependency{};
        ExternalDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        ExternalDependency.dstSubpass = 0;
        ExternalDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        ExternalDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        ExternalDependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        ExternalDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 2> Attachments = { ColorAttachment,DepthAttachment };

        VkRenderPassCreateInfo RenderPassCreateInfo{};
        RenderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        RenderPassCreateInfo.attachmentCount = Attachments.size();
        RenderPassCreateInfo.pAttachments = Attachments.data();
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
                SwapChainImagesViews[i],
                DepthBufferImageView
            };

            VkFramebufferCreateInfo FramebufferCreateInfo{};
            FramebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            FramebufferCreateInfo.renderPass = RenderPass;
            FramebufferCreateInfo.attachmentCount = 2;
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
        CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        VkCommandBufferAllocateInfo AllocCreateInfo{};
        AllocCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        AllocCreateInfo.commandPool = CommandPool;
        AllocCreateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        AllocCreateInfo.commandBufferCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkAllocateCommandBuffers(LogicalDevice, &AllocCreateInfo, CommandBuffers.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate command buffers");
        }
    }

    void RecordCommandBuffer(VkCommandBuffer CommandBuffer, uint32_t ImageIndex)
    {
        std::array<VkClearValue, 2> ClearColors{};
        ClearColors[0].color = { {0.0f,0.0f,0.0f,1.0f} };
        ClearColors[1].depthStencil = { 1.0f,0 };

        VkRenderingAttachmentInfo ColorAttachmentInfo{};
        ColorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        ColorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        ColorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        ColorAttachmentInfo.imageView = SwapChainImagesViews[ImageIndex];
        ColorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        ColorAttachmentInfo.clearValue = ClearColors[0];

        VkRenderingAttachmentInfo DepthAttachmentInfo{};
        DepthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        DepthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        DepthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        DepthAttachmentInfo.imageView = DepthBufferImageView;
        DepthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        DepthAttachmentInfo.clearValue = ClearColors[1];

        VkRenderingInfo RenderingInfo{};
        RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        RenderingInfo.pColorAttachments = &ColorAttachmentInfo;
        RenderingInfo.pDepthAttachment = &DepthAttachmentInfo;
        RenderingInfo.colorAttachmentCount = 1;
        RenderingInfo.layerCount = 1;
        RenderingInfo.renderArea = VkRect2D{ {0, 0}, {(uint32_t)Extent.width, (uint32_t)Extent.height} };

        VkCommandBufferBeginInfo CommandBufferBeginInfo{};
        CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        CommandBufferBeginInfo.flags = 0;
        CommandBufferBeginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to record command buffer");
        }

        TransitionImageLayout(CommandBuffer, SwapChainImages[ImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

        TransitionImageLayout(CommandBuffer, DepthBufferImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, 0,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

        //vkCmdBeginRenderPass(CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBeginRendering(CommandBuffer, &RenderingInfo);

        vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GraphicsPipeline);

        VkBuffer VertexBuffers[] = { VertexBuffer };
        VkDeviceSize Offsets[] = { 0 };
        vkCmdBindVertexBuffers(CommandBuffer, 0, 1, VertexBuffers, Offsets);
        vkCmdBindIndexBuffer(CommandBuffer, IndexBuffer, 0, VK_INDEX_TYPE_UINT32);

        VkViewport Viewport{};
        Viewport.x = 0.0f;
        Viewport.y = 0.0f;
        Viewport.width = static_cast<float>(Extent.width);
        Viewport.height = static_cast<float>(Extent.height);
        Viewport.minDepth = 0.0f;
        Viewport.maxDepth = 1.0f;
        vkCmdSetViewport(CommandBuffer, 0, 1, &Viewport);

        unsigned int VertexCount = 0, IndicesCount = 0;
        Model.GetCombinedVerticesIndicesCount(VertexCount, IndicesCount);

        VkRect2D Scissor{};
        Scissor.offset = { 0,0 };
        Scissor.extent = Extent;
        vkCmdSetScissor(CommandBuffer, 0, 1, &Scissor);
        vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 1, &DescriptorSets[CurrentFrame], 0, nullptr);
        vkCmdDrawIndexed(CommandBuffer, static_cast<uint32_t>(IndicesCount), 1, 0, 0, 0);
        vkCmdEndRendering(CommandBuffer);

        TransitionImageLayout(CommandBuffer, SwapChainImages[ImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        //vkCmdEndRenderPass(CommandBuffer);
        if (vkEndCommandBuffer(CommandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to record command buffer");
        }
    }

    void CreateSyncObjects()
    {
        ImageAvailableSemophores.resize(MAX_FRAMES_IN_FLIGHT);
        RenderFinishedSemophores.resize(MAX_FRAMES_IN_FLIGHT);
        InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo SemaphoreCreateInfo{};
        SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo FenceCreateInfo{};
        FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (vkCreateSemaphore(LogicalDevice, &SemaphoreCreateInfo, nullptr, &ImageAvailableSemophores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(LogicalDevice, &SemaphoreCreateInfo, nullptr, &RenderFinishedSemophores[i]) != VK_SUCCESS ||
                vkCreateFence(LogicalDevice, &FenceCreateInfo, nullptr, &InFlightFences[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create the semaphores and the fence!");
            }
        }
    }

    void DrawFrame()
    {
        vkWaitForFences(LogicalDevice, 1, &this->InFlightFences[CurrentFrame], VK_TRUE, UINT64_MAX);

        uint32_t ImageIndex;
        VkResult Result = vkAcquireNextImageKHR(LogicalDevice, SwapChain, UINT64_MAX, ImageAvailableSemophores[CurrentFrame], VK_NULL_HANDLE, &ImageIndex);

        if (Result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            RecreateSwapChain();
            return;
        }
        else if (Result != VK_SUCCESS && Result != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("Failed to acquire swap chain image!");
        }
        vkResetFences(LogicalDevice, 1, &InFlightFences[CurrentFrame]);

        vkResetCommandBuffer(CommandBuffers[CurrentFrame], 0);
        UpdateUniformBuffer(CurrentFrame);
        RecordCommandBuffer(CommandBuffers[CurrentFrame], ImageIndex);

        VkSubmitInfo SubmitInfo{};
        SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore WaitSemaphores[] = { ImageAvailableSemophores[CurrentFrame] };
        VkPipelineStageFlags WaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        SubmitInfo.waitSemaphoreCount = 1;
        SubmitInfo.pWaitSemaphores = WaitSemaphores;
        SubmitInfo.pWaitDstStageMask = WaitStages;
        SubmitInfo.commandBufferCount = 1;
        SubmitInfo.pCommandBuffers = &CommandBuffers[CurrentFrame];

        VkSemaphore SignalSemaphores[] = { RenderFinishedSemophores[CurrentFrame] };
        SubmitInfo.signalSemaphoreCount = 1;
        SubmitInfo.pSignalSemaphores = SignalSemaphores;

        if (vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, InFlightFences[CurrentFrame]) != VK_SUCCESS)
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
        Result = vkQueuePresentKHR(PresentQueue, &PresentInfo);

        if (Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR || FrameBufferResized)
        {
            RecreateSwapChain();
            FrameBufferResized = false;
            return;
        }
        else if (Result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to acquire swap chain image!");
        }

        CurrentFrame = (CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void CleanupSwapChain()
    {
        /*
        for (auto Framebuffer : SwapChainFramebuffers)
        {
            vkDestroyFramebuffer(LogicalDevice, Framebuffer, nullptr);
        }
        */

        for (auto ImageView : SwapChainImagesViews)
        {
            vkDestroyImageView(LogicalDevice, ImageView, nullptr);
        }

        vkDestroySwapchainKHR(LogicalDevice, SwapChain, nullptr);

        vkDestroyImageView(LogicalDevice, DepthBufferImageView, nullptr);
        vkDestroyImage(LogicalDevice, DepthBufferImage, nullptr);
        vkFreeMemory(LogicalDevice, DepthBufferImageMemory, nullptr);
    }

    void RecreateSwapChain()
    {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0)
        {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }
        vkDeviceWaitIdle(this->LogicalDevice);

        CleanupSwapChain();

        CreateSwapChain();
        CreateImageViews();
        CreateDepthBufferResources();
        //CreateFramebuffers();
    }

    void ExecuteSingleTimeCommand(std::function<void(VkCommandBuffer& CommandBuffer)> Task, VkCommandPool& Pool, VkQueue& Queue)
    {
        VkCommandBufferAllocateInfo CommandBufferAllocateInfo{};
        CommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        CommandBufferAllocateInfo.commandPool = Pool;
        CommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        CommandBufferAllocateInfo.commandBufferCount = 1;

        VkCommandBuffer SingleUseCommandBuffer;
        vkAllocateCommandBuffers(LogicalDevice, &CommandBufferAllocateInfo, &SingleUseCommandBuffer);

        VkCommandBufferBeginInfo CommandBufferBeginInfo{};
        CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(SingleUseCommandBuffer, &CommandBufferBeginInfo);

        Task(SingleUseCommandBuffer);

        vkEndCommandBuffer(SingleUseCommandBuffer);

        VkSubmitInfo CommandBufferSubmitInfo{};
        CommandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        CommandBufferSubmitInfo.commandBufferCount = 1;
        CommandBufferSubmitInfo.pCommandBuffers = &SingleUseCommandBuffer;

        vkQueueSubmit(Queue, 1, &CommandBufferSubmitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(Queue);

        vkFreeCommandBuffers(LogicalDevice, Pool, 1, &SingleUseCommandBuffer);
    }

    void CreateVertexBuffer()
    {
        std::vector<Vertex3D> CombinedVertices;
        std::vector<unsigned int> CombinedIndices;
        Model.GetCombinedVerticesIndices(CombinedVertices, CombinedIndices);
        VkDeviceSize BufferSize = sizeof(Vertex3D) * CombinedVertices.size();

        VkBuffer StagingBuffer;
        VkDeviceMemory StagingBufferMemory;
        CreateBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, StagingBuffer, StagingBufferMemory);

        void* Data;
        vkMapMemory(LogicalDevice, StagingBufferMemory, 0, BufferSize, 0, &Data);
        memcpy(Data, CombinedVertices.data(), (size_t)BufferSize);
        vkUnmapMemory(LogicalDevice, StagingBufferMemory);

        CreateBuffer(BufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VertexBuffer, VertexBufferMemory);
        CopyBuffer(StagingBuffer, VertexBuffer, BufferSize);

        vkDestroyBuffer(LogicalDevice, StagingBuffer, nullptr);
        vkFreeMemory(LogicalDevice, StagingBufferMemory, nullptr);
    }

    void CreateIndexBuffer()
    {
        std::vector<Vertex3D> CombinedVertices;
        std::vector<uint32_t> CombinedIndices;
        Model.GetCombinedVerticesIndices(CombinedVertices, CombinedIndices);
        VkDeviceSize BufferSize = sizeof(uint32_t) * CombinedIndices.size();

        VkBuffer StagingBuffer;
        VkDeviceMemory StagingBufferMemory;
        CreateBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, StagingBuffer, StagingBufferMemory);

        void* Data;
        vkMapMemory(LogicalDevice, StagingBufferMemory, 0, BufferSize, 0, &Data);
        memcpy(Data, CombinedIndices.data(), (size_t)BufferSize);
        vkUnmapMemory(LogicalDevice, StagingBufferMemory);

        CreateBuffer(BufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, IndexBuffer, IndexBufferMemory);
        CopyBuffer(StagingBuffer, IndexBuffer, BufferSize);

        vkDestroyBuffer(LogicalDevice, StagingBuffer, nullptr);
        vkFreeMemory(LogicalDevice, StagingBufferMemory, nullptr);
    }

    void CopyBuffer(VkBuffer SourceBuffer, VkBuffer DestinationBuffer, VkDeviceSize Size)
    {
        auto CopyCommand = [&](VkCommandBuffer& CommandBuffer) {
            VkBufferCopy CopyRegion{};
            CopyRegion.srcOffset = 0;
            CopyRegion.dstOffset = 0;
            CopyRegion.size = Size;
            vkCmdCopyBuffer(CommandBuffer, SourceBuffer, DestinationBuffer, 1, &CopyRegion);
            };
        ExecuteSingleTimeCommand(CopyCommand, CommandPool, GraphicsQueue);
    }

    uint32_t FindMemoryType(uint32_t TypeFilter, VkMemoryPropertyFlags Properties)
    {
        VkPhysicalDeviceMemoryProperties MemoryProperties;
        vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &MemoryProperties);

        for (size_t i = 0; i < MemoryProperties.memoryTypeCount; i++)
        {
            if (TypeFilter & (1 << i) && (MemoryProperties.memoryTypes[i].propertyFlags & Properties) == Properties)
            {
                return i;
            }
        }
    }

    void CreateBuffer(VkDeviceSize Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties, VkBuffer& Buffer, VkDeviceMemory& DeviceMemory)
    {
        VkBufferCreateInfo BufferCreateInfo{};
        BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        BufferCreateInfo.size = Size;
        BufferCreateInfo.usage = Usage;
        BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(LogicalDevice, &BufferCreateInfo, nullptr, &Buffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create vertex buffer!");
        }

        VkMemoryRequirements MemoryRequirements{};
        vkGetBufferMemoryRequirements(LogicalDevice, Buffer, &MemoryRequirements);

        VkMemoryAllocateInfo AllocationInfo{};
        AllocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        AllocationInfo.allocationSize = MemoryRequirements.size;
        AllocationInfo.memoryTypeIndex = FindMemoryType(MemoryRequirements.memoryTypeBits, Properties);

        if (vkAllocateMemory(LogicalDevice, &AllocationInfo, nullptr, &DeviceMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate memory!");
        }

        vkBindBufferMemory(LogicalDevice, Buffer, DeviceMemory, 0);
    }

    void CreateDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding UboLayoutBinding{};
        UboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        UboLayoutBinding.binding = 0;
        UboLayoutBinding.descriptorCount = 1;
        UboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        UboLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding TextureLayoutBinding{};
        TextureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        TextureLayoutBinding.binding = 1;
        TextureLayoutBinding.descriptorCount = 1;
        TextureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        TextureLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding Bindings[2] = { UboLayoutBinding,TextureLayoutBinding };
        VkDescriptorSetLayoutCreateInfo LayoutCreateInfo{};
        LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        LayoutCreateInfo.bindingCount = 2;
        LayoutCreateInfo.pBindings = Bindings;

        if (vkCreateDescriptorSetLayout(LogicalDevice, &LayoutCreateInfo, nullptr, &DescriptorSetLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create descriptor set layout!");
        }
    }

    void CreateUniformBuffers()
    {
        VkDeviceSize BufferSize = sizeof(Matrixes);

        UniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        UniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        UniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            CreateBuffer(BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, UniformBuffers[i], UniformBuffersMemory[i]);
            vkMapMemory(LogicalDevice, UniformBuffersMemory[i], 0, BufferSize, 0, &UniformBuffersMapped[i]);
        }
    }

    glm::vec3 Angles = glm::vec3(0.0f);

    void UpdateUniformBuffer(uint32_t CurrentImage)
    {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        {
            Angles.y += 1.0f;
        }
        else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        {
            Angles.y -= 1.0f;
        }
        else if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        {
            Angles.x += 1.0f;
        }
        else if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        {
            Angles.x -= 1.0f;
        }

        Matrixes MatrixUBO;
        MatrixUBO.ModelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(Angles.x), { 0.0f,1.0f,0.0f }) * glm::rotate(glm::mat4(1.0f), glm::radians(Angles.y), { 0.0f,0.0f,1.0f });
        MatrixUBO.ViewMatrix = glm::lookAt(glm::vec3(30.0f), glm::vec3(0.0f), { 0.0f,0.0f,1.0f });
        MatrixUBO.ProjectionMatrix = glm::perspective(glm::radians(45.0f), (float)Extent.width / (float)Extent.height, 0.01f, 1000.0f);
        MatrixUBO.ProjectionMatrix[1][1] *= -1;
        memcpy(UniformBuffersMapped[CurrentImage], &MatrixUBO, sizeof(MatrixUBO));
    }

    void CreateDescriptorPool()
    {
        VkDescriptorPoolSize UBOPoolSize{};
        UBOPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        UBOPoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolSize TexturePoolSize{};
        TexturePoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        TexturePoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        std::vector<VkDescriptorPoolSize> PoolSizes = { UBOPoolSize,TexturePoolSize };

        VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo{};
        DescriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        DescriptorPoolCreateInfo.poolSizeCount = PoolSizes.size();
        DescriptorPoolCreateInfo.pPoolSizes = PoolSizes.data();
        DescriptorPoolCreateInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(LogicalDevice, &DescriptorPoolCreateInfo, nullptr, &DescriptorPool) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create a descriptor pool");
        }
    }

    void CreateDescriptorSets()
    {
        std::vector<VkDescriptorSetLayout> Layouts(MAX_FRAMES_IN_FLIGHT, DescriptorSetLayout);
        VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo{};
        DescriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        DescriptorSetAllocateInfo.descriptorPool = DescriptorPool;
        DescriptorSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        DescriptorSetAllocateInfo.pSetLayouts = Layouts.data();

        DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(LogicalDevice, &DescriptorSetAllocateInfo, DescriptorSets.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            VkDescriptorBufferInfo DescriptorBufferInfo{};
            DescriptorBufferInfo.buffer = UniformBuffers[i];
            DescriptorBufferInfo.offset = 0;
            DescriptorBufferInfo.range = sizeof(Matrixes);

            VkDescriptorImageInfo DescriptorCombinedSamplerImageInfo{};
            DescriptorCombinedSamplerImageInfo.sampler = TextureSampler;
            DescriptorCombinedSamplerImageInfo.imageView = TextureImageView;
            DescriptorCombinedSamplerImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkWriteDescriptorSet UboDescriptorWrite{};
            UboDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            UboDescriptorWrite.dstSet = DescriptorSets[i];
            UboDescriptorWrite.dstBinding = 0;
            UboDescriptorWrite.dstArrayElement = 0;
            UboDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            UboDescriptorWrite.descriptorCount = 1;
            UboDescriptorWrite.pBufferInfo = &DescriptorBufferInfo;
            UboDescriptorWrite.pImageInfo = nullptr;
            UboDescriptorWrite.pTexelBufferView = nullptr;

            VkWriteDescriptorSet CombinedImageSamplerDescriptorWrite{};
            CombinedImageSamplerDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            CombinedImageSamplerDescriptorWrite.dstSet = DescriptorSets[i];
            CombinedImageSamplerDescriptorWrite.dstBinding = 1;
            CombinedImageSamplerDescriptorWrite.dstArrayElement = 0;
            CombinedImageSamplerDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            CombinedImageSamplerDescriptorWrite.descriptorCount = 1;
            CombinedImageSamplerDescriptorWrite.pBufferInfo = nullptr;
            CombinedImageSamplerDescriptorWrite.pImageInfo = &DescriptorCombinedSamplerImageInfo;
            CombinedImageSamplerDescriptorWrite.pTexelBufferView = nullptr;

            std::vector<VkWriteDescriptorSet> DescriptorWrites = { UboDescriptorWrite ,CombinedImageSamplerDescriptorWrite };
            vkUpdateDescriptorSets(LogicalDevice, DescriptorWrites.size(), DescriptorWrites.data(), 0, nullptr);
        }
    }

    void CreateTextureImage(const char* ImageFilePath)
    {
        int Width, Height, ChannelCount;
        auto Pixels = stbi_load(ImageFilePath, &Width, &Height, &ChannelCount, STBI_rgb_alpha);
        VkDeviceSize ImageSize = Width * Height * 4;

        if (!Pixels)
        {
            throw std::runtime_error("Unable to load the image(" + std::string(ImageFilePath) + ")");
        }

        VkBuffer StagingBuffer;
        VkDeviceMemory StagingBufferMemory;

        CreateBuffer(ImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, StagingBuffer, StagingBufferMemory);

        void* Data;
        vkMapMemory(LogicalDevice, StagingBufferMemory, 0, ImageSize, 0, &Data);
        memcpy(Data, Pixels, ImageSize);
        vkUnmapMemory(LogicalDevice, StagingBufferMemory);

        stbi_image_free(Pixels);

        CreateImage(Width, Height, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, TextureImage, TextureImageMemory);

        auto CopyCommand = [&](VkCommandBuffer& CommandBuffer) {
            TransitionImageLayout(CommandBuffer, TextureImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
            CopyBufferToImage(CommandBuffer, StagingBuffer, TextureImage, Width, Height);
            TransitionImageLayout(CommandBuffer, TextureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
            };

        ExecuteSingleTimeCommand(CopyCommand, CommandPool, GraphicsQueue);

        TextureImageView = CreateImageView(TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
        CreateTextureSampler();

        vkDestroyBuffer(LogicalDevice, StagingBuffer, nullptr);
        vkFreeMemory(LogicalDevice, StagingBufferMemory, nullptr);
    }

    void CreateImage(const uint32_t& Width, const uint32_t& Height, VkImageTiling Tiling, VkFormat Format, VkImageUsageFlags Usage, VkMemoryPropertyFlags Properties, VkImage& Image, VkDeviceMemory& ImageMemory)
    {
        VkImageCreateInfo ImageCreateInfo{};
        ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        ImageCreateInfo.format = Format;
        ImageCreateInfo.mipLevels = 1;
        ImageCreateInfo.extent.width = static_cast<uint32_t>(Width);
        ImageCreateInfo.extent.height = static_cast<uint32_t>(Height);
        ImageCreateInfo.extent.depth = 1;
        ImageCreateInfo.arrayLayers = 1;
        ImageCreateInfo.tiling = Tiling;
        ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        ImageCreateInfo.usage = Usage;
        ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        ImageCreateInfo.flags = 0;

        if (vkCreateImage(LogicalDevice, &ImageCreateInfo, nullptr, &Image) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create image!");
        }

        VkMemoryRequirements ImageMemoryRequirements;
        vkGetImageMemoryRequirements(LogicalDevice, Image, &ImageMemoryRequirements);

        VkMemoryAllocateInfo ImageMemoryAllocationInfo{};
        ImageMemoryAllocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        ImageMemoryAllocationInfo.allocationSize = ImageMemoryRequirements.size;
        ImageMemoryAllocationInfo.memoryTypeIndex = FindMemoryType(ImageMemoryRequirements.memoryTypeBits, Properties);

        if (vkAllocateMemory(LogicalDevice, &ImageMemoryAllocationInfo, nullptr, &ImageMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate memory for the image");
        }

        vkBindImageMemory(LogicalDevice, Image, ImageMemory, 0);
    }

    void TransitionImageLayout(VkCommandBuffer& DstCommandBuffer, VkImage& Image, VkImageLayout OldLayout, VkImageLayout NewLayout, VkAccessFlags SrcAccessMask,
        VkAccessFlags DstAccessMask, VkPipelineStageFlags SrcStage, VkPipelineStageFlags DstStage, VkImageAspectFlags AspectMask)
    {
        VkImageMemoryBarrier ImageBarrier{};
        ImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        ImageBarrier.oldLayout = OldLayout;
        ImageBarrier.newLayout = NewLayout;
        ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.image = Image;
        ImageBarrier.subresourceRange.aspectMask = AspectMask;
        ImageBarrier.subresourceRange.baseMipLevel = 0;
        ImageBarrier.subresourceRange.levelCount = 1;
        ImageBarrier.subresourceRange.baseArrayLayer = 0;
        ImageBarrier.subresourceRange.layerCount = 1;
        ImageBarrier.srcAccessMask = SrcAccessMask;
        ImageBarrier.dstAccessMask = DstAccessMask;

        vkCmdPipelineBarrier(DstCommandBuffer, SrcStage, DstStage
            , 0, 0, nullptr, 0, nullptr, 1, &ImageBarrier);
    }

    void CopyBufferToImage(VkCommandBuffer& DstCommandBuffer, VkBuffer& SrcBuffer, VkImage& DstImage, uint32_t Width, uint32_t Height)
    {
        VkBufferImageCopy CopyRegion{};
        CopyRegion.bufferOffset = 0;
        CopyRegion.bufferRowLength = 0;
        CopyRegion.bufferImageHeight = 0;

        CopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        CopyRegion.imageSubresource.mipLevel = 0;
        CopyRegion.imageSubresource.baseArrayLayer = 0;
        CopyRegion.imageSubresource.layerCount = 1;

        CopyRegion.imageOffset = { 0,0,0 };
        CopyRegion.imageExtent = { Width,Height,1 };

        vkCmdCopyBufferToImage(DstCommandBuffer, SrcBuffer, DstImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &CopyRegion);
    }

    VkImageView CreateImageView(VkImage& Image, VkFormat Format, VkImageAspectFlags AspectMask)
    {
        VkImageViewCreateInfo ImageViewCreateInfo{};
        ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ImageViewCreateInfo.format = Format;
        ImageViewCreateInfo.image = Image;
        ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        ImageViewCreateInfo.subresourceRange.levelCount = 1;
        ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        ImageViewCreateInfo.subresourceRange.layerCount = 1;
        ImageViewCreateInfo.subresourceRange.aspectMask = AspectMask;
        ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

        ImageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        ImageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        ImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        ImageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        VkImageView ImageView;
        if (vkCreateImageView(LogicalDevice, &ImageViewCreateInfo, nullptr, &ImageView) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create an image view!");
        }
        return ImageView;
    }

    void CreateTextureSampler()
    {
        VkPhysicalDeviceProperties DeviceProperties;
        vkGetPhysicalDeviceProperties(PhysicalDevice, &DeviceProperties);

        VkSamplerCreateInfo SamplerCreateInfo{};
        SamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        SamplerCreateInfo.magFilter = VK_FILTER_LINEAR;
        SamplerCreateInfo.minFilter = VK_FILTER_LINEAR;
        SamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
        SamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        SamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        SamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        SamplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        SamplerCreateInfo.maxAnisotropy = DeviceProperties.limits.maxSamplerAnisotropy;
        SamplerCreateInfo.anisotropyEnable = VK_TRUE;
        SamplerCreateInfo.compareEnable = VK_FALSE;
        SamplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        SamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        SamplerCreateInfo.mipLodBias = 0.0f;
        SamplerCreateInfo.minLod = 0.0f;
        SamplerCreateInfo.maxLod = 0.0f;

        if (vkCreateSampler(LogicalDevice, &SamplerCreateInfo, nullptr, &TextureSampler) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create texture sampler!");
        }
    }

    VkFormat FindSupportedFormat(const std::vector<VkFormat>& Candidates, VkImageTiling Tiling, VkFormatFeatureFlags Features)
    {
        for (auto Format : Candidates)
        {
            VkFormatProperties Properties;
            vkGetPhysicalDeviceFormatProperties(PhysicalDevice, Format, &Properties);

            if (Tiling == VK_IMAGE_TILING_LINEAR && (Properties.linearTilingFeatures & Features) == Features)
            {
                return Format;
            }
            else if (Tiling == VK_IMAGE_TILING_OPTIMAL && (Properties.optimalTilingFeatures & Features) == Features)
            {
                return Format;
            }

            throw std::runtime_error("Unable to find a suitable format!");
        }
    }

    void CreateDepthBufferResources()
    {
        DepthImageFormat = FindSupportedFormat({ VK_FORMAT_D32_SFLOAT,VK_FORMAT_D32_SFLOAT_S8_UINT,VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
        CreateImage(Extent.width, Extent.height, VK_IMAGE_TILING_OPTIMAL, DepthImageFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, DepthBufferImage, DepthBufferImageMemory);
        DepthBufferImageView = CreateImageView(DepthBufferImage, DepthImageFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
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