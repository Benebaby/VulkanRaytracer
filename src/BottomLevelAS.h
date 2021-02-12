#pragma once

#include <tiny_obj_loader.h>
#include "Device.h"
#include "Texture.h"
#include "GlobalDefs.h"

class BottomLevelAS
{
protected:
    VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, bool begin);
    void flushCommandBuffer(VkCommandBuffer command_buffer, VkQueue queue, bool free = true, VkSemaphore signalSemaphore = VK_NULL_HANDLE);
    BottomLevelAS(Device* device, std::string name, uint32_t id);
    PFN_vkCmdBuildAccelerationStructuresKHR  vkCmdBuildAccelerationStructuresKHR;            
    PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;                     
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;                     
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;       
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;  
public:
    static std::vector<Material> m_materials;
    static std::vector<Texture> m_textures;
    static Buffer m_materialBuffer;
    Device* m_device;
    std::string m_name;
    uint32_t m_id;
    Buffer m_accelerationStructureBuffer;
    VkDeviceAddress m_deviceAddress = VK_NULL_HANDLE;
    VkAccelerationStructureKHR  m_handle = VK_NULL_HANDLE;
    uint32_t getId() const;
    VkDeviceAddress getDeviceAdress() const;
    virtual void uploadData(std::string path);
    virtual void createSpheres();
    virtual VkDescriptorBufferInfo getIndexBufferDescriptor();
    virtual VkDescriptorBufferInfo getVertexBufferDescriptor();
    virtual VkDescriptorBufferInfo getSphereBufferDescriptor();
    virtual void create() = 0;
    virtual void destroy() = 0;
};