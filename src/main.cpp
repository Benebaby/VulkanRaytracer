#define GLFW_INCLUDE_VULKAN
#define VK_FLAGS_NONE 0
#define DEFAULT_FENCE_TIMEOUT 100000000000
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <optional>
#include <set>
#include <cstdint>
#include <chrono>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <unordered_map>

const uint32_t WIDTH = 1600;
const uint32_t HEIGHT = 900;
const bool enableValidationLayers = true;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation",
    "VK_LAYER_LUNARG_monitor"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
    VK_KHR_SPIRV_1_4_EXTENSION_NAME,
    VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME
};

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct ScratchBuffer
{
	uint64_t       device_address;
	VkBuffer       buffer;
	VkDeviceMemory memory;
};

struct AccelerationStructure
{
	uint64_t                    device_address;
	VkBuffer                    buffer;
    VkDeviceMemory              memory;
	VkAccelerationStructureKHR  accelerationStructure;
};

struct StorageImage {
    VkDeviceMemory memory;
    VkImage        image = VK_NULL_HANDLE;
    VkImageView    view;
    VkFormat       format;
    uint32_t       width;
    uint32_t       height;
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct Vertex
{
    float pos[3];
};

class VulkanRaytracer {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;

    VkInstance instance;

    PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
    PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
    PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;

    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties{};
    VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructureProperties{};
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
    VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddressFeatures{};
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingPipelineFeatures{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures{};

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline rayTracingPipeline;
    
    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    StorageImage storageImage;
    AccelerationStructure bottomLevelAccelerationStructure;
	AccelerationStructure topLevelAccelerationStructure;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    VkBuffer transformBuffer;
    VkDeviceMemory transformBufferMemory;
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;

    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups{};
    VkBuffer raygenShaderBindingTable;
    VkDeviceMemory raygenShaderBindingTableMemory;
    VkBuffer missShaderBindingTable;
    VkDeviceMemory missShaderBindingTableMemory;
    VkBuffer hitShaderBindingTable;
    VkDeviceMemory hitShaderBindingTableMemory;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    bool framebufferResized = false;

    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Raytracing RTV", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<VulkanRaytracer*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createFramebuffers();
        createCommandPool();
        getExtensionFunctionPointers();
        createStorageImage();
        loadModel("/viking_room.obj");
        createBottomLevelAccelerationStructure();
        createTopLevelAccelerationStructure();
        createUniformBuffer();
        createRayTracingPipeline();
        createShaderBindingTables();
        createDescriptorSets();
        createCommandBuffers();
        createSemaphores();
    }

    void mainLoop() {
        int fps = 0;
        double time = glfwGetTime();
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
        }
        vkDeviceWaitIdle(device);
    }

    void handleResize(){
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }
        vkDeviceWaitIdle(device);
        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
        vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
        vkDestroyPipeline(device, rayTracingPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);
        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }
        vkDestroySwapchainKHR(device, swapChain, nullptr);
        vkDestroyBuffer(device, uniformBuffer, nullptr);
        vkFreeMemory(device, uniformBufferMemory, nullptr);
        vkDestroyImageView(device, storageImage.view, nullptr);
        vkDestroyImage(device, storageImage.image, nullptr);
        vkFreeMemory(device, storageImage.memory, nullptr);
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

        createSwapChain();
        createImageViews();
        createRenderPass();
        createRayTracingPipeline();
        createFramebuffers();
        createUniformBuffer();
        createStorageImage();
        createDescriptorSets();
        createCommandBuffers();
    }

    void cleanup() {
        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
        
        vkDestroyPipeline(device, rayTracingPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);

        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(device, swapChain, nullptr);
        
        vkDestroyBuffer(device, uniformBuffer, nullptr);
        vkFreeMemory(device, uniformBufferMemory, nullptr);


        vkDestroyImageView(device, storageImage.view, nullptr);
        vkDestroyImage(device, storageImage.image, nullptr);
        vkFreeMemory(device, storageImage.memory, nullptr);
        
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

        //-----
    
        vkDestroyBuffer(device, topLevelAccelerationStructure.buffer, nullptr);
        vkFreeMemory(device, topLevelAccelerationStructure.memory, nullptr);
        vkDestroyAccelerationStructureKHR(device, topLevelAccelerationStructure.accelerationStructure, nullptr);

        vkDestroyBuffer(device, bottomLevelAccelerationStructure.buffer, nullptr);
        vkFreeMemory(device, bottomLevelAccelerationStructure.memory, nullptr);
        vkDestroyAccelerationStructureKHR(device, bottomLevelAccelerationStructure.accelerationStructure, nullptr);

        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);

        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);

        vkDestroyBuffer(device, transformBuffer, nullptr);
        vkFreeMemory(device, transformBufferMemory, nullptr);

        vkDestroyBuffer(device, raygenShaderBindingTable, nullptr);
        vkFreeMemory(device, raygenShaderBindingTableMemory, nullptr);
        vkDestroyBuffer(device, missShaderBindingTable, nullptr);
        vkFreeMemory(device, missShaderBindingTableMemory, nullptr);
        vkDestroyBuffer(device, hitShaderBindingTable, nullptr);
        vkFreeMemory(device, hitShaderBindingTableMemory, nullptr);

        vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);

        vkDestroyCommandPool(device, commandPool, nullptr);
        vkDestroyDevice(device, nullptr);

        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate();
    }

    void createInstance() {
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "VulkanRaytracer";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "RTV";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    void setupDebugMessenger() {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    void createSurface() {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    void pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }
        accelerationStructureProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
        rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
        rayTracingPipelineProperties.pNext = &accelerationStructureProperties;
        accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
        rayTracingPipelineFeatures.pNext = &accelerationStructureFeatures;

        VkPhysicalDeviceProperties2 deviceProps2{};
        deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        deviceProps2.pNext = &rayTracingPipelineProperties;

        VkPhysicalDeviceFeatures2 deviceFeatures2{};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &rayTracingPipelineFeatures;

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                break;
            }
        }

        if (physicalDevice != VK_NULL_HANDLE) {
            vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProps2);
            vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

            std::cout << "Picked Device: " <<deviceProps2.properties.deviceName << std::endl;
            std::cout << std::endl;
            std::cout << "Acceleration Structure Properties: " << std::endl;
            std::cout << "Max Geometry Count: " << accelerationStructureProperties.maxGeometryCount << std::endl;
            std::cout << "Max Instance Count: " << accelerationStructureProperties.maxInstanceCount << std::endl;
            std::cout << "Max Primitive Count: " << accelerationStructureProperties.maxPrimitiveCount << std::endl;
            std::cout << "Max Per Stage Descriptor Acceleration Structures: " << accelerationStructureProperties.maxPerStageDescriptorAccelerationStructures << std::endl;
            std::cout << "Max Per Stage Descriptor Update After Bind Acceleration Structures: " << accelerationStructureProperties.maxPerStageDescriptorUpdateAfterBindAccelerationStructures << std::endl;
            std::cout << "Max Descriptor SetAcceleration Structures: " << accelerationStructureProperties.maxDescriptorSetAccelerationStructures << std::endl;
            std::cout << "Max Descriptor Set Update After Bind Acceleration Structures: " << accelerationStructureProperties.maxDescriptorSetUpdateAfterBindAccelerationStructures << std::endl;
            std::cout << "Min Acceleration Structure Scratch Offset Alignment: " << accelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment << std::endl;
            std::cout << std::endl;
            std::cout << "Raytracing Pipeline Properties: " << std::endl;
            std::cout << "Shader Group Handle Size: " << rayTracingPipelineProperties.shaderGroupHandleSize << std::endl;
            std::cout << "Max Ray Recursion Depth: " << rayTracingPipelineProperties.maxRayRecursionDepth << std::endl;
            std::cout << "Max Shader Group Stride: " << rayTracingPipelineProperties.maxShaderGroupStride << std::endl;
            std::cout << "Shader Group Base Alignment: " << rayTracingPipelineProperties.shaderGroupBaseAlignment << std::endl;
            std::cout << "Shader Group Handle Capture Replay Size: " << rayTracingPipelineProperties.shaderGroupHandleCaptureReplaySize << std::endl;
            std::cout << "Max Ray Dispatch Invocation Count: " << rayTracingPipelineProperties.maxRayDispatchInvocationCount << std::endl;
            std::cout << "Shader Group Handle Alignment: " << rayTracingPipelineProperties.shaderGroupHandleAlignment << std::endl;
            std::cout << "Max Ray Hit Attribute Size: " << rayTracingPipelineProperties.maxRayHitAttributeSize << std::endl;
            std::cout << std::endl;
            std::cout << "Acceleration Structure Available Features: " << std::endl;
            std::cout << "Acceleration Structure: " << accelerationStructureFeatures.accelerationStructure << std::endl;
            std::cout << "Acceleration Structure Capture Replay: " << accelerationStructureFeatures.accelerationStructureCaptureReplay << std::endl;
            std::cout << "Acceleration Structure Indirect Build: " << accelerationStructureFeatures.accelerationStructureIndirectBuild << std::endl;
            std::cout << "Acceleration Structure Host Commands: " << accelerationStructureFeatures.accelerationStructureHostCommands << std::endl;
            std::cout << "Descriptor Binding Acceleration Structure Update After Bind: " << accelerationStructureFeatures.descriptorBindingAccelerationStructureUpdateAfterBind << std::endl;
            std::cout << std::endl;
            std::cout << "Raytracing Pipeline Available Features: " << std::endl;
            std::cout << "Ray Tracing Pipeline: " << rayTracingPipelineFeatures.rayTracingPipeline << std::endl;
            std::cout << "Ray Tracing Pipeline Shader Group Handle Capture Replay: " << rayTracingPipelineFeatures.rayTracingPipelineShaderGroupHandleCaptureReplay << std::endl;
            std::cout << "Ray Tracing Pipeline Shader Group Handle Capture Replay Mixed: " << rayTracingPipelineFeatures.rayTracingPipelineShaderGroupHandleCaptureReplayMixed << std::endl;
            std::cout << "Ray Tracing Pipeline Trace Rays Indirect: " << rayTracingPipelineFeatures.rayTracingPipelineTraceRaysIndirect << std::endl;
            std::cout << "Ray Traversal Primitive Culling: " << rayTracingPipelineFeatures.rayTraversalPrimitiveCulling << std::endl;
        } else {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        enabledBufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
        enabledBufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
        
        enabledRayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
        enabledRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
        enabledRayTracingPipelineFeatures.pNext = &enabledBufferDeviceAddressFeatures;

        enabledAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        enabledAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
        enabledAccelerationStructureFeatures.pNext = &enabledRayTracingPipelineFeatures;

        VkPhysicalDeviceFeatures2 deviceFeatures2{};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.features.samplerAnisotropy = VK_TRUE;
        deviceFeatures2.pNext = &enabledAccelerationStructureFeatures;

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pEnabledFeatures = NULL;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        createInfo.pNext = &deviceFeatures2;
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }

    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    void createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image views!");
            }
        }
    }

    void getExtensionFunctionPointers(){
        vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));
        vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));
		vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkBuildAccelerationStructuresKHR"));
		vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
		vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
		vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
		vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR"));
		vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR"));
		vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR"));
		vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));
    }

    ScratchBuffer createScratchBuffer(VkDeviceSize size){
        ScratchBuffer scratchBuffer{};
        VkBufferCreateInfo buffer_create_info = {};
        buffer_create_info.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_create_info.size               = size;
        buffer_create_info.usage              = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        if(vkCreateBuffer(device, &buffer_create_info, nullptr, &scratchBuffer.buffer) != VK_SUCCESS)
            throw std::runtime_error("failed to create scratch buffer!");

        VkMemoryRequirements memoryRequirements = {};
        vkGetBufferMemoryRequirements(device, scratchBuffer.buffer, &memoryRequirements);

        VkMemoryAllocateFlagsInfo memory_allocate_flags_info = {};
        memory_allocate_flags_info.sType                     = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
        memory_allocate_flags_info.flags                     = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

        VkMemoryAllocateInfo memoryAllocateInfo = {};
        memoryAllocateInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.pNext                = &memory_allocate_flags_info;
        memoryAllocateInfo.allocationSize       = memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex      = findMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if(vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &scratchBuffer.memory) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate scratch buffer memory!");
        if(vkBindBufferMemory(device, scratchBuffer.buffer, scratchBuffer.memory, 0) != VK_SUCCESS)
            throw std::runtime_error("failed to bind scratch buffer memory!");

        VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo{};
        bufferDeviceAddressInfo.sType   = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        bufferDeviceAddressInfo.buffer  = scratchBuffer.buffer;
        scratchBuffer.device_address   = vkGetBufferDeviceAddressKHR(device, &bufferDeviceAddressInfo);

        return scratchBuffer;
    }

    void createStorageImage(){
        storageImage.width = swapChainExtent.width;
	    storageImage.height = swapChainExtent.height;
        
        VkImageCreateInfo imageCreateInfo{};
	    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType         = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format            = VK_FORMAT_B8G8R8A8_UNORM;
        imageCreateInfo.extent.width      = storageImage.width;
        imageCreateInfo.extent.height     = storageImage.height;
        imageCreateInfo.extent.depth      = 1;
        imageCreateInfo.mipLevels         = 1;
        imageCreateInfo.arrayLayers       = 1;
        imageCreateInfo.samples           = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage             = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
        imageCreateInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
        if(vkCreateImage(device, &imageCreateInfo, nullptr, &storageImage.image) != VK_SUCCESS){
            throw std::runtime_error("failed to create Storage Image!");
        }

        VkMemoryRequirements memoryRequirements;
        vkGetImageMemoryRequirements(device, storageImage.image, &memoryRequirements);
        VkMemoryAllocateInfo memoryAllocateInfo{};
	    memoryAllocateInfo.sType            = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.allocationSize   = memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &storageImage.memory) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate storage image memory!");
        if (vkBindImageMemory(device, storageImage.image, storageImage.memory, 0) != VK_SUCCESS)
            throw std::runtime_error("failed to bind storage image memory!");

        VkImageViewCreateInfo colorImageViewCreateInfo{};
	    colorImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        colorImageViewCreateInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        colorImageViewCreateInfo.format                          = VK_FORMAT_B8G8R8A8_UNORM;
        colorImageViewCreateInfo.subresourceRange                = {};
        colorImageViewCreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        colorImageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
        colorImageViewCreateInfo.subresourceRange.levelCount     = 1;
        colorImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        colorImageViewCreateInfo.subresourceRange.layerCount     = 1;
        colorImageViewCreateInfo.image                           = storageImage.image;
        if (vkCreateImageView(device, &colorImageViewCreateInfo, nullptr, &storageImage.view) != VK_SUCCESS)
            throw std::runtime_error("failed to create storage image view!");
        VkCommandBuffer command_buffer = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
        setImageLayout(command_buffer, storageImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
        flushCommandBuffer(command_buffer, graphicsQueue);
    }

     void loadModel(std::string Modelpath) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, (MODEL_PATH+Modelpath).c_str())) {
            throw std::runtime_error(warn + err);
        }
        uint32_t current_index = 0; 

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex{};
                vertex = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                // vertex.texCoord = {
                //     attrib.texcoords[2 * index.texcoord_index + 0],
                //     1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                // };

                // vertex.color = {1.0f, 1.0f, 1.0f};
                vertices.push_back(vertex);
                indices.push_back(current_index);
                current_index++;
            }
        }
    }

    void createBottomLevelAccelerationStructure(){
        uint32_t numTriangles = static_cast<uint32_t>(vertices.size()) / 3;
		uint32_t maxVertex = static_cast<uint32_t>(vertices.size());
        VkTransformMatrixKHR transformMatrix = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f
        };

        auto vertexBufferSize = vertices.size() * sizeof(Vertex);
	    auto indexBufferSize  = indices.size() * sizeof(uint32_t);
        auto transformBufferSize = sizeof(transformMatrix);

        const VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        const VkMemoryPropertyFlags memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        createBuffer(vertexBufferSize, bufferUsageFlags, memoryPropertyFlags, vertexBuffer, vertexBufferMemory);
        void* vertexBufferData;
        vkMapMemory(device, vertexBufferMemory, 0, vertexBufferSize, 0, &vertexBufferData);
            memcpy(vertexBufferData, vertices.data(), vertexBufferSize);
        vkUnmapMemory(device, vertexBufferMemory);

        createBuffer(indexBufferSize,bufferUsageFlags, memoryPropertyFlags, indexBuffer, indexBufferMemory);
        void* indexBufferData;
        vkMapMemory(device, indexBufferMemory, 0, indexBufferSize, 0, &indexBufferData);
            memcpy(indexBufferData, indices.data(), indexBufferSize);
        vkUnmapMemory(device, indexBufferMemory);

        createBuffer(transformBufferSize,bufferUsageFlags, memoryPropertyFlags, transformBuffer, transformBufferMemory);
        void* transformBufferData;
        vkMapMemory(device, transformBufferMemory, 0, transformBufferSize, 0, &transformBufferData);
            memcpy(transformBufferData, &transformMatrix, transformBufferSize);
        vkUnmapMemory(device, transformBufferMemory);

        VkDeviceOrHostAddressConstKHR vertexDataDeviceAddress{};
        VkDeviceOrHostAddressConstKHR indexDataDeviceAddress{};
        VkDeviceOrHostAddressConstKHR transformMatrixDeviceAddress{};

        vertexDataDeviceAddress.deviceAddress      = getBufferDeviceAddress(vertexBuffer);
        indexDataDeviceAddress.deviceAddress       = getBufferDeviceAddress(indexBuffer);
        transformMatrixDeviceAddress.deviceAddress = getBufferDeviceAddress(transformBuffer);

        VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
        accelerationStructureGeometry.sType                            = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        accelerationStructureGeometry.geometryType                     = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        accelerationStructureGeometry.flags                            = VK_GEOMETRY_OPAQUE_BIT_KHR;
        accelerationStructureGeometry.geometry.triangles.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        accelerationStructureGeometry.geometry.triangles.vertexFormat  = VK_FORMAT_R32G32B32_SFLOAT;
        accelerationStructureGeometry.geometry.triangles.vertexData    = vertexDataDeviceAddress;
        accelerationStructureGeometry.geometry.triangles.maxVertex     = maxVertex;
        accelerationStructureGeometry.geometry.triangles.vertexStride  = sizeof(Vertex);
        accelerationStructureGeometry.geometry.triangles.indexType     = VK_INDEX_TYPE_UINT32;
        accelerationStructureGeometry.geometry.triangles.indexData     = indexDataDeviceAddress;
        accelerationStructureGeometry.geometry.triangles.transformData = transformMatrixDeviceAddress;

        VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
        accelerationStructureBuildGeometryInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationStructureBuildGeometryInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        accelerationStructureBuildGeometryInfo.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationStructureBuildGeometryInfo.geometryCount = 1;
        accelerationStructureBuildGeometryInfo.pGeometries   = &accelerationStructureGeometry;

        const uint32_t primitiveCount = 1;

        VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
        accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

        vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &accelerationStructureBuildGeometryInfo, &numTriangles, &accelerationStructureBuildSizesInfo);

        createBuffer(accelerationStructureBuildSizesInfo.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR, bottomLevelAccelerationStructure.buffer, bottomLevelAccelerationStructure.memory);

        VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
        accelerationStructureCreateInfo.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        accelerationStructureCreateInfo.buffer = bottomLevelAccelerationStructure.buffer;
        accelerationStructureCreateInfo.size   = accelerationStructureBuildSizesInfo.accelerationStructureSize;
        accelerationStructureCreateInfo.type   = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

        vkCreateAccelerationStructureKHR(device, &accelerationStructureCreateInfo, nullptr, &bottomLevelAccelerationStructure.accelerationStructure);

        ScratchBuffer scratchBuffer = createScratchBuffer(accelerationStructureBuildSizesInfo.buildScratchSize);

        VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
        accelerationBuildGeometryInfo.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationBuildGeometryInfo.type                      = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        accelerationBuildGeometryInfo.flags                     = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationBuildGeometryInfo.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        accelerationBuildGeometryInfo.dstAccelerationStructure  = bottomLevelAccelerationStructure.accelerationStructure;
        accelerationBuildGeometryInfo.geometryCount             = 1;
        accelerationBuildGeometryInfo.pGeometries               = &accelerationStructureGeometry;
        accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.device_address;

        VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo;
        accelerationStructureBuildRangeInfo.primitiveCount                                           = numTriangles;
        accelerationStructureBuildRangeInfo.primitiveOffset                                          = 0;
        accelerationStructureBuildRangeInfo.firstVertex                                              = 0;
        accelerationStructureBuildRangeInfo.transformOffset                                          = 0;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR *> accelerationBuildStructureRangeInfos = {&accelerationStructureBuildRangeInfo};

        VkCommandBuffer command_buffer = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
        vkCmdBuildAccelerationStructuresKHR(command_buffer, 1, &accelerationBuildGeometryInfo, accelerationBuildStructureRangeInfos.data());
        flushCommandBuffer(command_buffer, graphicsQueue);
        vkFreeMemory(device, scratchBuffer.memory, nullptr);
        vkDestroyBuffer(device, scratchBuffer.buffer, nullptr);

        VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
        accelerationDeviceAddressInfo.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        accelerationDeviceAddressInfo.accelerationStructure = bottomLevelAccelerationStructure.accelerationStructure;
        bottomLevelAccelerationStructure.device_address     = vkGetAccelerationStructureDeviceAddressKHR(device, &accelerationDeviceAddressInfo);
    }

    void createTopLevelAccelerationStructure(){
        VkTransformMatrixKHR transformMatrix = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f
        };

        VkAccelerationStructureInstanceKHR accelerationStructureInstance{};
        accelerationStructureInstance.transform                              = transformMatrix;
        accelerationStructureInstance.instanceCustomIndex                    = 0;
        accelerationStructureInstance.mask                                   = 0xFF;
        accelerationStructureInstance.instanceShaderBindingTableRecordOffset = 0;
        accelerationStructureInstance.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        accelerationStructureInstance.accelerationStructureReference         = bottomLevelAccelerationStructure.device_address;

        VkBuffer instancesBuffer;
        VkDeviceMemory instancesBufferMemory;
		createBuffer(sizeof(VkAccelerationStructureInstanceKHR), VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, instancesBuffer, instancesBufferMemory);
        void* instancesBufferData;
        vkMapMemory(device, instancesBufferMemory, 0, sizeof(VkAccelerationStructureInstanceKHR), 0, &instancesBufferData);
            memcpy(instancesBufferData, &accelerationStructureInstance, sizeof(VkAccelerationStructureInstanceKHR));
        vkUnmapMemory(device, instancesBufferMemory);

        VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
	    instanceDataDeviceAddress.deviceAddress = getBufferDeviceAddress(instancesBuffer);

        VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
        accelerationStructureGeometry.sType                              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        accelerationStructureGeometry.geometryType                       = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        accelerationStructureGeometry.flags                              = VK_GEOMETRY_OPAQUE_BIT_KHR;
        accelerationStructureGeometry.geometry.instances.sType           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
        accelerationStructureGeometry.geometry.instances.data            = instanceDataDeviceAddress;

        VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
        accelerationStructureBuildGeometryInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationStructureBuildGeometryInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        accelerationStructureBuildGeometryInfo.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationStructureBuildGeometryInfo.geometryCount = 1;
        accelerationStructureBuildGeometryInfo.pGeometries   = &accelerationStructureGeometry;

        const uint32_t primitiveCount = 1;

        VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
        accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &accelerationStructureBuildGeometryInfo, &primitiveCount, &accelerationStructureBuildSizesInfo);
        createBuffer(accelerationStructureBuildSizesInfo.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR, topLevelAccelerationStructure.buffer, topLevelAccelerationStructure.memory);

        VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
        accelerationStructureCreateInfo.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        accelerationStructureCreateInfo.buffer = topLevelAccelerationStructure.buffer;
        accelerationStructureCreateInfo.size   = accelerationStructureBuildSizesInfo.accelerationStructureSize;
        accelerationStructureCreateInfo.type   = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        vkCreateAccelerationStructureKHR(device, &accelerationStructureCreateInfo, nullptr, &topLevelAccelerationStructure.accelerationStructure);

        ScratchBuffer scratchBuffer = createScratchBuffer(accelerationStructureBuildSizesInfo.buildScratchSize);

        VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
        accelerationBuildGeometryInfo.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationBuildGeometryInfo.type                      = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        accelerationBuildGeometryInfo.flags                     = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationBuildGeometryInfo.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        accelerationBuildGeometryInfo.dstAccelerationStructure  = topLevelAccelerationStructure.accelerationStructure;
        accelerationBuildGeometryInfo.geometryCount             = 1;
        accelerationBuildGeometryInfo.pGeometries               = &accelerationStructureGeometry;
        accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.device_address;

        VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo;
        accelerationStructureBuildRangeInfo.primitiveCount                                           = 1;
        accelerationStructureBuildRangeInfo.primitiveOffset                                          = 0;
        accelerationStructureBuildRangeInfo.firstVertex                                              = 0;
        accelerationStructureBuildRangeInfo.transformOffset                                          = 0;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR *> accelerationBuildStructureRangeInfos = {&accelerationStructureBuildRangeInfo};

        VkCommandBuffer command_buffer = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
        vkCmdBuildAccelerationStructuresKHR(command_buffer, 1, &accelerationBuildGeometryInfo, accelerationBuildStructureRangeInfos.data());
        flushCommandBuffer(command_buffer, graphicsQueue);
        vkFreeMemory(device, scratchBuffer.memory, nullptr);
        vkDestroyBuffer(device, scratchBuffer.buffer, nullptr);

        VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
        accelerationDeviceAddressInfo.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        accelerationDeviceAddressInfo.accelerationStructure = topLevelAccelerationStructure.accelerationStructure;
        topLevelAccelerationStructure.device_address        = vkGetAccelerationStructureDeviceAddressKHR(device, &accelerationDeviceAddressInfo);

        vkDestroyBuffer(device, instancesBuffer, nullptr);
        vkFreeMemory(device, instancesBufferMemory, nullptr);
    }

    void updateUniformBuffer(){
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        glm::mat3 camRotation = glm::mat3(glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f) * camRotation, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float) swapChainExtent.height, 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;

        void* data;
        vkMapMemory(device, uniformBufferMemory, 0, sizeof(ubo), 0, &data);
            memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(device, uniformBufferMemory);
    }

    void createUniformBuffer(){
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer, uniformBufferMemory);
        updateUniformBuffer();
    }

    void createDescriptorSets()
    {
        std::vector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}
        };
        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolInfo.pPoolSizes    = poolSizes.data();
        descriptorPoolInfo.maxSets       = 1;
        if(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool))
            throw std::runtime_error("failed to create descriptor pool!");

        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
        descriptorSetAllocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.descriptorPool     = descriptorPool;
        descriptorSetAllocateInfo.pSetLayouts        = &descriptorSetLayout;
        descriptorSetAllocateInfo.descriptorSetCount = 1;
        if(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorSet))
            throw std::runtime_error("failed to allocate descriptor sets!");


        VkWriteDescriptorSetAccelerationStructureKHR descriptor_acceleration_structure_info{};
        descriptor_acceleration_structure_info.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        descriptor_acceleration_structure_info.accelerationStructureCount = 1;
        descriptor_acceleration_structure_info.pAccelerationStructures    = &topLevelAccelerationStructure.accelerationStructure;

        VkWriteDescriptorSet accelerationStructureWrite{};
        accelerationStructureWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        accelerationStructureWrite.dstSet          = descriptorSet;
        accelerationStructureWrite.dstBinding      = 0;
        accelerationStructureWrite.descriptorCount = 1;
        accelerationStructureWrite.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        accelerationStructureWrite.pNext = &descriptor_acceleration_structure_info;

        VkDescriptorImageInfo storageImageDescriptor{};
		storageImageDescriptor.imageView = storageImage.view;
		storageImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkWriteDescriptorSet resultImageWrite{};
        resultImageWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        resultImageWrite.dstSet          = descriptorSet;
        resultImageWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        resultImageWrite.dstBinding      = 1;
        resultImageWrite.pImageInfo      = &storageImageDescriptor;
        resultImageWrite.descriptorCount = 1;

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet uniformBufferWrite{};
        uniformBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        uniformBufferWrite.dstSet = descriptorSet;
        uniformBufferWrite.dstBinding = 2;
        uniformBufferWrite.dstArrayElement = 0;
        uniformBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBufferWrite.descriptorCount = 1;
        uniformBufferWrite.pBufferInfo = &bufferInfo;
        uniformBufferWrite.pImageInfo = nullptr; // Optional
        uniformBufferWrite.pTexelBufferView = nullptr; // Optional

        std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
            accelerationStructureWrite,
	        resultImageWrite,
            uniformBufferWrite
        };

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, VK_NULL_HANDLE);
    }

    void createRenderPass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
            throw std::runtime_error("failed to create render pass!");
    }

    void createRayTracingPipeline(){
        VkDescriptorSetLayoutBinding acceleration_structure_layout_binding{};
        acceleration_structure_layout_binding.binding         = 0;
        acceleration_structure_layout_binding.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        acceleration_structure_layout_binding.descriptorCount = 1;
        acceleration_structure_layout_binding.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

        VkDescriptorSetLayoutBinding result_image_layout_binding{};
        result_image_layout_binding.binding         = 1;
        result_image_layout_binding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        result_image_layout_binding.descriptorCount = 1;
        result_image_layout_binding.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

        VkDescriptorSetLayoutBinding uniform_buffer_binding{};
        uniform_buffer_binding.binding         = 2;
        uniform_buffer_binding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniform_buffer_binding.descriptorCount = 1;
        uniform_buffer_binding.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

        std::vector<VkDescriptorSetLayoutBinding> bindings = {
            acceleration_structure_layout_binding,
            result_image_layout_binding,
            uniform_buffer_binding
        };

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings    = bindings.data();
        if(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor set layout!");

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts    = &descriptorSetLayout;

        if(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("failed to create pipeline layout!");

        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

        auto raygenShaderCode = readFile("/rgen.spv");
        auto missShaderCode = readFile("/rmiss.spv");
        auto rchitShaderCode = readFile("/rchit.spv");
        VkShaderModule raygenShaderModule = createShaderModule(raygenShaderCode);
        VkShaderModule missShaderModule = createShaderModule(missShaderCode);
        VkShaderModule rchitShaderModule = createShaderModule(rchitShaderCode);

        VkPipelineShaderStageCreateInfo raygenShaderStageInfo{};
        raygenShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        raygenShaderStageInfo.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
        raygenShaderStageInfo.module = raygenShaderModule;
        raygenShaderStageInfo.pName = "main";
        shaderStages.push_back(raygenShaderStageInfo);

        VkRayTracingShaderGroupCreateInfoKHR raygenGroupCreateInfo{};
		raygenGroupCreateInfo.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		raygenGroupCreateInfo.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		raygenGroupCreateInfo.generalShader      = static_cast<uint32_t>(shaderStages.size()) - 1;
		raygenGroupCreateInfo.closestHitShader   = VK_SHADER_UNUSED_KHR;
		raygenGroupCreateInfo.anyHitShader       = VK_SHADER_UNUSED_KHR;
		raygenGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(raygenGroupCreateInfo);

        VkPipelineShaderStageCreateInfo missShaderStageInfo{};
        missShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        missShaderStageInfo.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
        missShaderStageInfo.module = missShaderModule;
        missShaderStageInfo.pName = "main";
        shaderStages.push_back(missShaderStageInfo);

        VkRayTracingShaderGroupCreateInfoKHR missGroupCreateInfo{};
		missGroupCreateInfo.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		missGroupCreateInfo.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		missGroupCreateInfo.generalShader      = static_cast<uint32_t>(shaderStages.size()) - 1;
		missGroupCreateInfo.closestHitShader   = VK_SHADER_UNUSED_KHR;
		missGroupCreateInfo.anyHitShader       = VK_SHADER_UNUSED_KHR;
		missGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(missGroupCreateInfo);

        VkPipelineShaderStageCreateInfo rchitShaderStageInfo{};
        rchitShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        rchitShaderStageInfo.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        rchitShaderStageInfo.module = rchitShaderModule;
        rchitShaderStageInfo.pName = "main";
        shaderStages.push_back(rchitShaderStageInfo);

        VkRayTracingShaderGroupCreateInfoKHR closesHitGroupCreateInfo{};
		closesHitGroupCreateInfo.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		closesHitGroupCreateInfo.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		closesHitGroupCreateInfo.generalShader      = VK_SHADER_UNUSED_KHR;
		closesHitGroupCreateInfo.closestHitShader   = static_cast<uint32_t>(shaderStages.size()) - 1;
		closesHitGroupCreateInfo.anyHitShader       = VK_SHADER_UNUSED_KHR;
		closesHitGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(closesHitGroupCreateInfo);

        VkRayTracingPipelineCreateInfoKHR raytracingPipelineCreateInfo{};
        raytracingPipelineCreateInfo.sType                        = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
        raytracingPipelineCreateInfo.stageCount                   = static_cast<uint32_t>(shaderStages.size());
        raytracingPipelineCreateInfo.pStages                      = shaderStages.data();
        raytracingPipelineCreateInfo.groupCount                   = static_cast<uint32_t>(shaderGroups.size());
        raytracingPipelineCreateInfo.pGroups                      = shaderGroups.data();
        raytracingPipelineCreateInfo.maxPipelineRayRecursionDepth = 1;
        raytracingPipelineCreateInfo.layout                       = pipelineLayout;
        if(vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &raytracingPipelineCreateInfo, nullptr, &rayTracingPipeline) != VK_SUCCESS)
            throw std::runtime_error("failed to create ray tracing pipeline!");
        vkDestroyShaderModule(device, raygenShaderModule, nullptr);
        vkDestroyShaderModule(device, missShaderModule, nullptr);
        vkDestroyShaderModule(device, rchitShaderModule, nullptr);
    }

    void createShaderBindingTables(){
        const uint32_t handleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
		const uint32_t handleSizeAligned = alignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);
		const uint32_t groupCount = static_cast<uint32_t>(shaderGroups.size());
		const uint32_t sbtSize = groupCount * handleSizeAligned;

		std::vector<uint8_t> shaderHandleStorage(sbtSize);
		if(vkGetRayTracingShaderGroupHandlesKHR(device, rayTracingPipeline, 0, groupCount, sbtSize, shaderHandleStorage.data()) != VK_SUCCESS)
            throw std::runtime_error("failed to get ray tracing shader group handles!");

		const VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		const VkMemoryPropertyFlags memoryUsageFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		createBuffer(handleSize, bufferUsageFlags, memoryUsageFlags, raygenShaderBindingTable, raygenShaderBindingTableMemory);
		createBuffer(handleSize, bufferUsageFlags, memoryUsageFlags, missShaderBindingTable, missShaderBindingTableMemory);
		createBuffer(handleSize, bufferUsageFlags, memoryUsageFlags, hitShaderBindingTable, hitShaderBindingTableMemory);

        void* raygenShaderBindingTableData;
        vkMapMemory(device, raygenShaderBindingTableMemory, 0, handleSize, 0, &raygenShaderBindingTableData);
            memcpy(raygenShaderBindingTableData, shaderHandleStorage.data(), handleSize);
        vkUnmapMemory(device, raygenShaderBindingTableMemory);

        void* missShaderBindingTableData;
        vkMapMemory(device, missShaderBindingTableMemory, 0, handleSize, 0, &missShaderBindingTableData);
            memcpy(missShaderBindingTableData, shaderHandleStorage.data() + handleSizeAligned, handleSize);
        vkUnmapMemory(device, missShaderBindingTableMemory);

        void* hitShaderBindingTableData;
        vkMapMemory(device, hitShaderBindingTableMemory, 0, handleSize, 0, &hitShaderBindingTableData);
            memcpy(hitShaderBindingTableData, shaderHandleStorage.data() + handleSizeAligned * 2, handleSize);
        vkUnmapMemory(device, hitShaderBindingTableMemory);
    }

    void createFramebuffers() {
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            VkImageView attachments[] = {
                swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void createCommandPool() {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    void createCommandBuffers() {
        commandBuffers.resize(swapChainFramebuffers.size());

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkImageSubresourceRange subresource_range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        for (size_t i = 0; i < commandBuffers.size(); i++) {

            if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
                throw std::runtime_error("failed to begin recording command buffer!");
            }

            /*
                Setup the strided device address regions pointing at the shader identifiers in the shader binding table
            */

            const uint32_t handle_size_aligned = alignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);

            VkStridedDeviceAddressRegionKHR raygen_shader_sbt_entry{};
            raygen_shader_sbt_entry.deviceAddress = getBufferDeviceAddress(raygenShaderBindingTable);
            raygen_shader_sbt_entry.stride        = handle_size_aligned;
            raygen_shader_sbt_entry.size          = handle_size_aligned;

            VkStridedDeviceAddressRegionKHR miss_shader_sbt_entry{};
            miss_shader_sbt_entry.deviceAddress = getBufferDeviceAddress(missShaderBindingTable);
            miss_shader_sbt_entry.stride        = handle_size_aligned;
            miss_shader_sbt_entry.size          = handle_size_aligned;

            VkStridedDeviceAddressRegionKHR hit_shader_sbt_entry{};
            hit_shader_sbt_entry.deviceAddress = getBufferDeviceAddress(hitShaderBindingTable);
            hit_shader_sbt_entry.stride        = handle_size_aligned;
            hit_shader_sbt_entry.size          = handle_size_aligned;

            VkStridedDeviceAddressRegionKHR callable_shader_sbt_entry{};

            /*
                Dispatch the ray tracing commands
            */
            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rayTracingPipeline);
            vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineLayout, 0, 1, &descriptorSet, 0, 0);

            vkCmdTraceRaysKHR(commandBuffers[i], &raygen_shader_sbt_entry, &miss_shader_sbt_entry, &hit_shader_sbt_entry, &callable_shader_sbt_entry, swapChainExtent.width, swapChainExtent.height, 1);

            /*
                Copy ray tracing output to swap chain image
            */

            // Prepare current swap chain image as transfer destination
            setImageLayout(commandBuffers[i], swapChainImages[i], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresource_range);

            // Prepare ray tracing output image as transfer source
            setImageLayout( commandBuffers[i], storageImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresource_range);

            VkImageCopy copy_region{};
            copy_region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
            copy_region.srcOffset      = {0, 0, 0};
            copy_region.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
            copy_region.dstOffset      = {0, 0, 0};
            copy_region.extent         = {swapChainExtent.width, swapChainExtent.height, 1};
            vkCmdCopyImage(commandBuffers[i], storageImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapChainImages[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

            // Transition swap chain image back for presentation
            setImageLayout(commandBuffers[i], swapChainImages[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, subresource_range);

            // Transition ray tracing output image back to general layout
            setImageLayout(commandBuffers[i], storageImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, subresource_range);

            if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to record command buffer!");
            }
        }
    }

    void drawFrame() {
        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            handleResize();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        updateUniformBuffer();

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }
        
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; // Optional
        result = vkQueuePresentKHR(presentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            handleResize();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }
        vkQueueWaitIdle(presentQueue);
    }

    void createSemaphores(){
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS || vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphores!");
        }
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = glm::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = glm::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

            return actualExtent;
        }
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    bool isDeviceSuitable(VkPhysicalDevice device) {
        QueueFamilyIndices indices = findQueueFamilies(device);

        bool extensionsSupported = checkDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }
        
        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (presentSupport) {
                indices.presentFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    std::vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        return extensions;
    }

    bool checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(SHADER_PATH+filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        }
        return VK_FALSE;
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void setImageLayout(VkCommandBuffer command_buffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subresourceRange, VkPipelineStageFlags srcMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VkPipelineStageFlags dstMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT)
    {
        // Create an image barrier object
        VkImageMemoryBarrier barrier{};
        barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.oldLayout           = oldLayout;
        barrier.newLayout           = newLayout;
        barrier.image               = image;
        barrier.subresourceRange    = subresourceRange;

        // Source layouts (old)
        // Source access mask controls actions that have to be finished on the old layout
        // before it will be transitioned to the new layout
        switch (oldLayout)
        {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                // Image layout is undefined (or does not matter)
                // Only valid as initial layout
                // No flags required, listed only for completeness
                barrier.srcAccessMask = 0;
                break;

            case VK_IMAGE_LAYOUT_PREINITIALIZED:
                // Image is preinitialized
                // Only valid as initial layout for linear images, preserves memory contents
                // Make sure host writes have been finished
                barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                // Image is a color attachment
                // Make sure any writes to the color buffer have been finished
                barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                // Image is a depth/stencil attachment
                // Make sure any writes to the depth/stencil buffer have been finished
                barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                // Image is a transfer source
                // Make sure any reads from the image have been finished
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                // Image is a transfer destination
                // Make sure any writes to the image have been finished
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                // Image is read by a shader
                // Make sure any shader reads from the image have been finished
                barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;
            default:
                // Other source layouts aren't handled (yet)
                break;
        }

        // Target layouts (new)
        // Destination access mask controls the dependency for the new image layout
        switch (newLayout)
        {
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                // Image will be used as a transfer destination
                // Make sure any writes to the image have been finished
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                // Image will be used as a transfer source
                // Make sure any reads from the image have been finished
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                // Image will be used as a color attachment
                // Make sure any writes to the color buffer have been finished
                barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                // Image layout will be used as a depth/stencil attachment
                // Make sure any writes to depth/stencil buffer have been finished
                barrier.dstAccessMask = barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                // Image will be read in a shader (sampler, input attachment)
                // Make sure any writes to the image have been finished
                if (barrier.srcAccessMask == 0)
                {
                    barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
                }
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;
            default:
                // Other source layouts aren't handled (yet)
                break;
        }

        // Put barrier inside setup command buffer
        vkCmdPipelineBarrier(command_buffer, srcMask, dstMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, bool begin)
    {
        VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
        cmdBufAllocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufAllocateInfo.commandPool        = commandPool;
        cmdBufAllocateInfo.level              = level;
        cmdBufAllocateInfo.commandBufferCount = 1;

        VkCommandBuffer command_buffer;
        if(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &command_buffer) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate command buffer!");

        // If requested, also start recording for the new command buffer
        if (begin)
        {
            VkCommandBufferBeginInfo command_buffer_info{};
            command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            if(vkBeginCommandBuffer(command_buffer, &command_buffer_info) != VK_SUCCESS)
                throw std::runtime_error("failed to start recording command buffer!");
        }

        return command_buffer;
    }

    void flushCommandBuffer(VkCommandBuffer command_buffer, VkQueue queue, bool free = true, VkSemaphore signalSemaphore = VK_NULL_HANDLE) 
    {
        if (command_buffer == VK_NULL_HANDLE)
        {
            return;
        }

        if(vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
            throw std::runtime_error("failed to flush command buffer!");

        VkSubmitInfo submit_info{};
        submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers    = &command_buffer;
        if (signalSemaphore)
        {
            submit_info.pSignalSemaphores    = &signalSemaphore;
            submit_info.signalSemaphoreCount = 1;
        }

        // Create fence to ensure that the command buffer has finished executing
        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FLAGS_NONE;

        VkFence fence;
        if(vkCreateFence(device, &fence_info, nullptr, &fence) != VK_SUCCESS)
            throw std::runtime_error("failed to create Fence while flushing command buffer!");

        // Submit to the queue
        VkResult result = vkQueueSubmit(queue, 1, &submit_info, fence);
        // Wait for the fence to signal that command buffer has finished executing
        if(vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT) != VK_SUCCESS)
            throw std::runtime_error("failed to wait for the fence to signal that command buffer has finished executing!");

        vkDestroyFence(device, fence, nullptr);

        vkFreeCommandBuffers(device, commandPool, 1, &command_buffer);
    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
        if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
            allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
            allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
            allocInfo.pNext = &allocFlagsInfo;
        }

        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        if (vkBindBufferMemory(device, buffer, bufferMemory, 0) != VK_SUCCESS)
            throw std::runtime_error("failed to bind buffer memory!");
    }

    uint64_t getBufferDeviceAddress(VkBuffer buffer)
    {
        VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo{};
        bufferDeviceAddressInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        bufferDeviceAddressInfo.buffer = buffer;
        return vkGetBufferDeviceAddressKHR(device, &bufferDeviceAddressInfo);
    }

    inline uint32_t alignedSize(uint32_t value, uint32_t alignment)
    {
        return (value + alignment - 1) & ~(alignment - 1);
    }
};

int main() {
    VulkanRaytracer app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}