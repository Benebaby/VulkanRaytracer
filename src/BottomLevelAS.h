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
    static std::vector<Material> m_materials;
    static Buffer m_materialBuffer;
    static std::vector<Texture> m_textures;
    static std::vector<VkDescriptorImageInfo> m_textureDescriptors;
    static VkDescriptorBufferInfo m_materialBufferDescriptor;
public:
    Device* m_device;
    std::string m_name;
    uint32_t m_id;
    Buffer m_accelerationStructureBuffer;
    VkDeviceAddress m_deviceAddress = VK_NULL_HANDLE;
    VkAccelerationStructureKHR  m_handle = VK_NULL_HANDLE;
    uint32_t getId() const;
    VkDeviceAddress getDeviceAdress() const;
    static void createMaterialBuffer(Device* device);
    static VkDescriptorBufferInfo* getMaterialBufferDescriptor(); 
    static VkDescriptorImageInfo* getTextureDescriptors(); 
    static uint32_t getTextureCount();
    static void destroyTextures();
    static void destroyMaterials();
    virtual void create() = 0;
    virtual void destroy() = 0;
};