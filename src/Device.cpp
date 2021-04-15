#include "Device.h"

Device::Device(Instance* instance){
    m_instance = instance;
    m_physical_device = VK_NULL_HANDLE;
}

void Device::addExtension(const char* extension){
    m_extensions.push_back(extension);
}

void Device::pickPhysicalDevice(){
    //Properties chained up for query
    VkPhysicalDeviceProperties2 m_deviceProperties2{};
    m_deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    m_deviceProperties2.pNext = &m_rayTracingPipelineProperties;
    m_rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    m_rayTracingPipelineProperties.pNext = &m_accelerationStructureProperties;
    m_accelerationStructureProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;

    //Features chained up for query
    VkPhysicalDeviceFeatures2 m_deviceFeatures2{};
    m_deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    m_deviceFeatures2.pNext = &m_rayTracingPipelineFeatures;
    m_rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    m_rayTracingPipelineFeatures.pNext = &m_accelerationStructureFeatures;
    m_accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    m_accelerationStructureFeatures.pNext = &m_descriptorIndexingFeatures;
    m_descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;

    uint32_t deviceCount = 0;

    vkEnumeratePhysicalDevices(m_instance->getHandle(), &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance->getHandle(), &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            m_physical_device = device;
            break;
        }
    }

    if (m_physical_device != VK_NULL_HANDLE) {
        vkGetPhysicalDeviceProperties2(m_physical_device, &m_deviceProperties2);
        vkGetPhysicalDeviceFeatures2(m_physical_device, &m_deviceFeatures2);
    } else {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

void Device::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(m_physical_device);

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

    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    //anisotropische Texturfilterung
    deviceFeatures2.features.samplerAnisotropy = VK_TRUE;
    deviceFeatures2.pNext = &m_enabledAccelerationStructureFeatures;
    //Beschleunigungsstruktur Features
    m_enabledAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    m_enabledAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
    m_enabledAccelerationStructureFeatures.pNext = &m_enabledRayTracingPipelineFeatures;
    //Raytracing Pipline Features
    m_enabledRayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    m_enabledRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
    m_enabledRayTracingPipelineFeatures.pNext = &m_enabledBufferDeviceAddressFeatures;
    //Zum Erstellen von 64bit Pointern auf verschiedene Vulkan Objekte
    m_enabledBufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    m_enabledBufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
    m_enabledBufferDeviceAddressFeatures.pNext = &m_enabledDescriptorIndexingFeatures;
    //Um die Länge von UniformBuffer Arrays nicht sperat in Shader übergeben zu müssen
    m_enabledDescriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    m_enabledDescriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pEnabledFeatures = NULL;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(m_extensions.size());
    createInfo.ppEnabledExtensionNames = m_extensions.data();
    createInfo.pNext = &deviceFeatures2;
    if (m_instance->hasValidation()) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(m_instance->getLayers().size());
        createInfo.ppEnabledLayerNames = m_instance->getLayers().data();
    } else {
        createInfo.enabledLayerCount = 0;
    }
    if (vkCreateDevice(m_physical_device, &createInfo, nullptr, &m_handle) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }
    vkGetDeviceQueue(m_handle, indices.graphicsFamily.value(), 0, &m_graphics_queue);
    vkGetDeviceQueue(m_handle, indices.presentFamily.value(), 0, &m_present_queue);
}

VkDevice Device::getHandle(){
    return m_handle;
}

VkPhysicalDevice Device::getPhysicalDevice(){
    return m_physical_device;
}

VkQueue Device::getGraphicsQueue(){
    return m_graphics_queue;
}

VkQueue Device::getPresentQueue(){
    return m_present_queue;
}

bool Device::supportsAccelerationStructureHostCommands(){
    return m_accelerationStructureFeatures.accelerationStructureHostCommands;
}

uint32_t Device::getShaderGroupHandleSize(){
    return m_rayTracingPipelineProperties.shaderGroupHandleSize;
}

uint32_t Device::getShaderGroupHandleAlignment(){
    return m_rayTracingPipelineProperties.shaderGroupHandleAlignment;
}

void Device::printPropertiesAndFeatures(){
    std::cout << "Picked Device: " <<m_deviceProperties2.properties.deviceName << std::endl;
    std::cout << std::endl;
    std::cout << "Acceleration Structure Properties: " << std::endl;
    std::cout << "Max Geometry Count: " << m_accelerationStructureProperties.maxGeometryCount << std::endl;
    std::cout << "Max Instance Count: " << m_accelerationStructureProperties.maxInstanceCount << std::endl;
    std::cout << "Max Primitive Count: " << m_accelerationStructureProperties.maxPrimitiveCount << std::endl;
    std::cout << "Max Per Stage Descriptor Acceleration Structures: " << m_accelerationStructureProperties.maxPerStageDescriptorAccelerationStructures << std::endl;
    std::cout << "Max Per Stage Descriptor Update After Bind Acceleration Structures: " << m_accelerationStructureProperties.maxPerStageDescriptorUpdateAfterBindAccelerationStructures << std::endl;
    std::cout << "Max Descriptor SetAcceleration Structures: " << m_accelerationStructureProperties.maxDescriptorSetAccelerationStructures << std::endl;
    std::cout << "Max Descriptor Set Update After Bind Acceleration Structures: " << m_accelerationStructureProperties.maxDescriptorSetUpdateAfterBindAccelerationStructures << std::endl;
    std::cout << "Min Acceleration Structure Scratch Offset Alignment: " << m_accelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment << std::endl;
    std::cout << std::endl;
    std::cout << "Raytracing Pipeline Properties: " << std::endl;
    std::cout << "Shader Group Handle Size: " << m_rayTracingPipelineProperties.shaderGroupHandleSize << std::endl;
    std::cout << "Max Ray Recursion Depth: " << m_rayTracingPipelineProperties.maxRayRecursionDepth << std::endl;
    std::cout << "Max Shader Group Stride: " << m_rayTracingPipelineProperties.maxShaderGroupStride << std::endl;
    std::cout << "Shader Group Base Alignment: " << m_rayTracingPipelineProperties.shaderGroupBaseAlignment << std::endl;
    std::cout << "Shader Group Handle Capture Replay Size: " << m_rayTracingPipelineProperties.shaderGroupHandleCaptureReplaySize << std::endl;
    std::cout << "Max Ray Dispatch Invocation Count: " << m_rayTracingPipelineProperties.maxRayDispatchInvocationCount << std::endl;
    std::cout << "Shader Group Handle Alignment: " << m_rayTracingPipelineProperties.shaderGroupHandleAlignment << std::endl;
    std::cout << "Max Ray Hit Attribute Size: " << m_rayTracingPipelineProperties.maxRayHitAttributeSize << std::endl;
    std::cout << std::endl;
    std::cout << "Acceleration Structure Available Features: " << std::endl;
    std::cout << "Acceleration Structure: " << m_accelerationStructureFeatures.accelerationStructure << std::endl;
    std::cout << "Acceleration Structure Capture Replay: " << m_accelerationStructureFeatures.accelerationStructureCaptureReplay << std::endl;
    std::cout << "Acceleration Structure Indirect Build: " << m_accelerationStructureFeatures.accelerationStructureIndirectBuild << std::endl;
    std::cout << "Acceleration Structure Host Commands: " << m_accelerationStructureFeatures.accelerationStructureHostCommands << std::endl;
    std::cout << "Descriptor Binding Acceleration Structure Update After Bind: " << m_accelerationStructureFeatures.descriptorBindingAccelerationStructureUpdateAfterBind << std::endl;
    std::cout << std::endl;
    std::cout << "Raytracing Pipeline Available Features: " << std::endl;
    std::cout << "Ray Tracing Pipeline: " << m_rayTracingPipelineFeatures.rayTracingPipeline << std::endl;
    std::cout << "Ray Tracing Pipeline Shader Group Handle Capture Replay: " << m_rayTracingPipelineFeatures.rayTracingPipelineShaderGroupHandleCaptureReplay << std::endl;
    std::cout << "Ray Tracing Pipeline Shader Group Handle Capture Replay Mixed: " << m_rayTracingPipelineFeatures.rayTracingPipelineShaderGroupHandleCaptureReplayMixed << std::endl;
    std::cout << "Ray Tracing Pipeline Trace Rays Indirect: " << m_rayTracingPipelineFeatures.rayTracingPipelineTraceRaysIndirect << std::endl;
    std::cout << "Ray Traversal Primitive Culling: " << m_rayTracingPipelineFeatures.rayTraversalPrimitiveCulling << std::endl;
}

bool Device::isDeviceSuitable(VkPhysicalDevice device) {
    bool swapChainAdequate = false;
    bool requiredFeaturesSupported = false;
    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceBufferDeviceAddressFeatures supportedBufferDeviceAddressFeatures{};
    supportedBufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR supportedRayTracingPipelineFeatures{};
    supportedRayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    supportedRayTracingPipelineFeatures.pNext = &supportedBufferDeviceAddressFeatures;
    VkPhysicalDeviceAccelerationStructureFeaturesKHR supportedAccelerationStructureFeatures{};
    supportedAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    supportedAccelerationStructureFeatures.pNext = &supportedRayTracingPipelineFeatures;
    VkPhysicalDeviceFeatures2 supportedFeatures{};
    supportedFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    supportedFeatures.pNext = &supportedAccelerationStructureFeatures;
    vkGetPhysicalDeviceFeatures2(device, &supportedFeatures);

    if(supportedFeatures.features.samplerAnisotropy && supportedAccelerationStructureFeatures.accelerationStructure && supportedRayTracingPipelineFeatures.rayTracingPipeline && supportedBufferDeviceAddressFeatures.bufferDeviceAddress)
        requiredFeaturesSupported = true;

    return indices.isComplete() && extensionsSupported && swapChainAdequate && requiredFeaturesSupported;
}

QueueFamilyIndices Device::findQueueFamilies(VkPhysicalDevice device) {
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
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_instance->getSurface(), &presentSupport);

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

QueueFamilyIndices Device::findQueueFamilies() {
    return findQueueFamilies(m_physical_device);
}

bool Device::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(m_extensions.begin(), m_extensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

SwapChainSupportDetails Device::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_instance->getSurface(), &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_instance->getSurface(), &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_instance->getSurface(), &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_instance->getSurface(), &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_instance->getSurface(), &presentModeCount, details.presentModes.data());
    }

    return details;
}

SwapChainSupportDetails Device::querySwapChainSupport() {
    return querySwapChainSupport(m_physical_device);
}

void Device::createCommandPool(){
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies();
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(m_handle, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

VkCommandPool Device::getCommandPool(){
    return m_commandPool;
}

Device::~Device(){}

void Device::destroy(){
    vkDestroyCommandPool(m_handle, m_commandPool, nullptr);
    vkDestroyDevice(m_handle, nullptr);
}