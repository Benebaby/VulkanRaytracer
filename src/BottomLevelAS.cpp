#include "BottomLevelAS.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

std::vector<Material> BottomLevelAS::m_materials = std::vector<Material>(0);
std::vector<Texture> BottomLevelAS::m_textures = std::vector<Texture>(0);
Buffer BottomLevelAS::m_materialBuffer;
std::vector<VkDescriptorImageInfo> BottomLevelAS::m_textureDescriptors(0);
VkDescriptorBufferInfo BottomLevelAS::m_materialBufferDescriptor;

BottomLevelAS::BottomLevelAS(Device* device, std::string name, uint32_t id) : m_device(device), m_name(name), m_id(id) {
    vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(m_device->getHandle(), "vkCmdBuildAccelerationStructuresKHR"));
    vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(m_device->getHandle(), "vkBuildAccelerationStructuresKHR"));
    vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(m_device->getHandle(), "vkCreateAccelerationStructureKHR"));
    vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(m_device->getHandle(), "vkGetAccelerationStructureBuildSizesKHR"));
    vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(m_device->getHandle(), "vkGetAccelerationStructureDeviceAddressKHR"));
    vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(m_device->getHandle(), "vkDestroyAccelerationStructureKHR"));
}

uint32_t BottomLevelAS::getId() const{
    return m_id;
}

VkDeviceAddress BottomLevelAS::getDeviceAdress() const{
    return m_deviceAddress;
}

void BottomLevelAS::createMaterialBuffer(Device* device){
    auto materialBufferSize = m_materials.size() * sizeof(Material);
    const VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    const VkMemoryPropertyFlags memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    m_materialBuffer = Buffer(device, materialBufferSize, bufferUsageFlags, memoryPropertyFlags);
    m_materialBuffer.map(materialBufferSize, 0);
    m_materialBuffer.copyTo(m_materials.data(), materialBufferSize);
    m_materialBuffer.unmap();
}

VkDescriptorBufferInfo* BottomLevelAS::getMaterialBufferDescriptor(){
    m_materialBufferDescriptor = m_materialBuffer.getDescriptorInfo(VK_WHOLE_SIZE, 0);
    return &m_materialBufferDescriptor; 
}

VkDescriptorImageInfo* BottomLevelAS::getTextureDescriptors(){
    std::vector<VkDescriptorImageInfo>	descriptorImageInfos(m_textures.size());
    for (size_t i = 0; i < m_textures.size(); i++)
    {
        descriptorImageInfos[i] = m_textures[i].getDescriptorInfo();
    }
    m_textureDescriptors = descriptorImageInfos;
    return m_textureDescriptors.data();
}

uint32_t BottomLevelAS::getTextureCount(){
    return static_cast<uint32_t>(m_textures.size());
}

void BottomLevelAS::destroyTextures(){
    for(Texture texture : m_textures){
        texture.destroy();
    }
}

void BottomLevelAS::destroyMaterials(){
    m_materialBuffer.destroy();
}

VkCommandBuffer BottomLevelAS::createCommandBuffer(VkCommandBufferLevel level, bool begin){
    VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
    cmdBufAllocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufAllocateInfo.commandPool        = m_device->getCommandPool();
    cmdBufAllocateInfo.level              = level;
    cmdBufAllocateInfo.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    if(vkAllocateCommandBuffers(m_device->getHandle(), &cmdBufAllocateInfo, &command_buffer) != VK_SUCCESS)
        throw std::runtime_error("failed to allocate command buffer!");
    if (begin)
    {
        VkCommandBufferBeginInfo command_buffer_info{};
        command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        if(vkBeginCommandBuffer(command_buffer, &command_buffer_info) != VK_SUCCESS)
            throw std::runtime_error("failed to start recording command buffer!");
    }

    return command_buffer;
}

void BottomLevelAS::flushCommandBuffer(VkCommandBuffer command_buffer, VkQueue queue, bool free, VkSemaphore signalSemaphore){
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

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FLAGS_NONE;

    VkFence fence;
    if(vkCreateFence(m_device->getHandle(), &fence_info, nullptr, &fence) != VK_SUCCESS)
        throw std::runtime_error("failed to create Fence while flushing command buffer!");

    VkResult result = vkQueueSubmit(queue, 1, &submit_info, fence);
    if(vkWaitForFences(m_device->getHandle(), 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT) != VK_SUCCESS)
        throw std::runtime_error("failed to wait for the fence to signal that command buffer has finished executing!");

    vkDestroyFence(m_device->getHandle(), fence, nullptr);

    vkFreeCommandBuffers(m_device->getHandle(), m_device->getCommandPool(), 1, &command_buffer);
}