#pragma once

#include "Device.h"
#include "Buffer.h"
#include "GlobalDefs.h"
#include <stb_image.h>

class Texture
{
private:
    Device*                 m_device;
    VkImage                 m_image;
    VkBufferUsageFlags      m_usageFlags;
    VkMemoryPropertyFlags   m_memoryPropertyFlags;
    VkDeviceMemory          m_imageMemory;
    VkImageView             m_imageView = VK_NULL_HANDLE;
    VkSampler               m_sampler = VK_NULL_HANDLE;
    VkFormat                m_format;
    uint32_t                m_width;
    uint32_t                m_height;
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
    void createTextureImageView();
    void createTextureSampler();
    void setImageLayout(VkCommandBuffer command_buffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subresourceRange, VkPipelineStageFlags srcMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VkPipelineStageFlags dstMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, bool begin);
    void flushCommandBuffer(VkCommandBuffer command_buffer, VkQueue queue, bool free = true, VkSemaphore signalSemaphore = VK_NULL_HANDLE);
    uint32_t findMemoryType(uint32_t typeFilter);
public:
    Texture();
    Texture(Device* device, std::string filepath, VkFormat format);
    Texture(Device* device, uint32_t width, uint32_t height, VkFormat format);
    VkDescriptorImageInfo getDescriptorInfo();
    VkImage getImage();
    void destroy();
    ~Texture();
};


