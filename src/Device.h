#pragma once

#include "Instance.h"
#include "GlobalDefs.h"

class Device
{
private:
    VkDevice m_handle;
    Instance* m_instance;
    VkPhysicalDevice m_physical_device;
    VkQueue m_graphics_queue;
    VkQueue m_present_queue;
    std::vector<const char*> m_extensions;

    VkPhysicalDeviceProperties2 m_deviceProperties2{};
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rayTracingPipelineProperties{};
    VkPhysicalDeviceAccelerationStructurePropertiesKHR m_accelerationStructureProperties{};

    VkPhysicalDeviceFeatures2 m_deviceFeatures2{};
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR m_rayTracingPipelineFeatures{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR m_accelerationStructureFeatures{};

    VkPhysicalDeviceBufferDeviceAddressFeatures m_enabledBufferDeviceAddressFeatures{};
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR m_enabledRayTracingPipelineFeatures{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR m_enabledAccelerationStructureFeatures{};
    bool isDeviceSuitable(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
public:
    Device(Instance* instance);
    void addExtension(const char* extension);
    void pickPhysicalDevice();
    void createLogicalDevice();
    void printPropertiesAndFeatures();
    VkPhysicalDevice getPhysicalDevice();
    VkDevice getHandle();
    VkQueue getGraphicsQueue();
    VkQueue getPresentQueue();
    bool supportsAccelerationStructureHostCommands();
    uint32_t getShaderGroupHandleSize();
    uint32_t getShaderGroupHandleAlignment();
    SwapChainSupportDetails querySwapChainSupport();
    QueueFamilyIndices findQueueFamilies();
    void destroy();
    ~Device();
};