#include "Buffer.h"

Buffer::Buffer()
{

}

Buffer::Buffer(Device* device, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags){
    m_device = device;
    m_size = size;
    m_usageFlags = usageFlags;
    m_memoryPropertyFlags = memoryPropertyFlags;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = m_usageFlags;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_device->getHandle(), &bufferInfo, nullptr, &m_handle) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device->getHandle(), m_handle, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits);

    VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
    if (m_usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
        allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
        allocInfo.pNext = &allocFlagsInfo;
    }

    if (vkAllocateMemory(m_device->getHandle(), &allocInfo, nullptr, &m_memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }
}

void Buffer::bind(VkDeviceSize offset){
    if (vkBindBufferMemory(m_device->getHandle(), m_handle, m_memory, offset) != VK_SUCCESS)
        throw std::runtime_error("failed to bind buffer memory!");
}

void Buffer::map(VkDeviceSize size, VkDeviceSize offset){
    if (vkMapMemory(m_device->getHandle(), m_memory, offset, size, 0, &m_mapped) != VK_SUCCESS)
        throw std::runtime_error("failed to map buffer!");
}

void Buffer::unmap(){
    if (m_mapped)
    {
        vkUnmapMemory(m_device->getHandle(), m_memory);
        m_mapped = (void*) nullptr;
    }
}

void Buffer::copyTo(void* data, VkDeviceSize size){
    memcpy(m_mapped, data, size);
}

uint32_t Buffer::findMemoryType(uint32_t typeFilter) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_device->getPhysicalDevice(), &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & m_memoryPropertyFlags) == m_memoryPropertyFlags) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

VkBuffer Buffer::getHandle(){
    return m_handle;
}

VkDescriptorBufferInfo Buffer::getDescriptorInfo(VkDeviceSize size, VkDeviceSize offset){
    VkDescriptorBufferInfo descriptorBufferInfo{};
    if(m_handle){
        descriptorBufferInfo.buffer = m_handle;
        descriptorBufferInfo.offset = offset;
        descriptorBufferInfo.range = size;
        return descriptorBufferInfo;
    }
    return descriptorBufferInfo;
}

void Buffer::destroy()
{
    if (m_handle){
        vkDestroyBuffer(m_device->getHandle(), m_handle, nullptr);
    }
    if (m_memory){
        vkFreeMemory(m_device->getHandle(), m_memory, nullptr);
    }
}