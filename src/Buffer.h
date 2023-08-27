#pragma once

#include "Device.h"
#include "GlobalDefs.h"

class Buffer
{
private:
    Device* m_device;
    VkBuffer m_handle = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkDeviceSize m_size = 0;
    void* m_mapped = (void*) nullptr;
    VkBufferUsageFlags m_usageFlags;
    VkMemoryPropertyFlags m_memoryPropertyFlags;
    VkDeviceAddress m_deviceAddress;
    uint32_t findMemoryType(uint32_t typeFilter);
public:
    Buffer();
    Buffer(Device* device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    void map(VkDeviceSize size, VkDeviceSize offset);
    void bind(VkDeviceSize offset);
    void unmap();
    void copyTo(void* data, VkDeviceSize size);
    void destroy();
    VkBuffer getHandle();
    VkDeviceAddress getDeviceAddress();
    VkDescriptorBufferInfo getDescriptorInfo(VkDeviceSize size, VkDeviceSize offset);
};

