
#include "Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Texture::Texture(/* args */)
{

}

Texture::Texture(Device* device, std::string filepath, VkFormat format)
{
    m_usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    m_memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    m_format = format;
    m_device = device;


    std::string texture_path = TEXTURE_PATH;
    texture_path += filepath;
    const char* path = texture_path.c_str();
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    m_width = static_cast<uint32_t>(texWidth);
    m_height = static_cast<uint32_t>(texHeight);

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    Buffer stagingBuffer = Buffer(m_device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    stagingBuffer.map(imageSize, 0);
    stagingBuffer.copyTo(pixels, static_cast<size_t>(imageSize));
    stagingBuffer.unmap();

    stbi_image_free(pixels);
    createImage(m_width, m_height, format, VK_IMAGE_TILING_OPTIMAL, m_usageFlags, m_memoryPropertyFlags);
    VkCommandBuffer command_buffer = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        setImageLayout(command_buffer, m_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {m_width, m_height, 1};
        vkCmdCopyBufferToImage(command_buffer, stagingBuffer.getHandle(), m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        setImageLayout(command_buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

    flushCommandBuffer(command_buffer, m_device->getGraphicsQueue());

    stagingBuffer.destroy();

    createTextureImageView();
    createTextureSampler();
}

Texture::Texture(Device* device, uint32_t width, uint32_t height, VkFormat format)
{
    m_usageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    m_memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    m_format = format;
    m_device = device;
    m_width = width;
    m_height = height;

    createImage(m_width, m_height, format, VK_IMAGE_TILING_OPTIMAL, m_usageFlags, m_memoryPropertyFlags);
    createTextureImageView();
    VkCommandBuffer command_buffer = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
        setImageLayout(command_buffer, m_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
    flushCommandBuffer(command_buffer, m_device->getGraphicsQueue());
}

VkDescriptorImageInfo Texture::getDescriptorInfo(){
    VkDescriptorImageInfo imageInfo{};
    if(m_sampler != VK_NULL_HANDLE && m_imageView != VK_NULL_HANDLE){
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_imageView;
        imageInfo.sampler = m_sampler;
    }else if(m_sampler == VK_NULL_HANDLE && m_imageView != VK_NULL_HANDLE){
		imageInfo.imageView = m_imageView;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    }else{
        throw std::runtime_error("No valid ImageView for Descriptor");
    }
    return imageInfo;
}

VkImage Texture::getImage(){
    return m_image;
}

void Texture::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(m_device->getHandle(), &imageInfo, nullptr, &m_image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device->getHandle(), m_image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits);

    if (vkAllocateMemory(m_device->getHandle(), &allocInfo, nullptr, &m_imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(m_device->getHandle(), m_image, m_imageMemory, 0);
}

void Texture::createTextureImageView() {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = m_format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(m_device->getHandle(), &viewInfo, nullptr, &m_imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }
}

void Texture::createTextureSampler() {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(m_device->getPhysicalDevice(), &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(m_device->getHandle(), &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

 void Texture::setImageLayout(VkCommandBuffer command_buffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subresourceRange, VkPipelineStageFlags srcMask, VkPipelineStageFlags dstMask)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.oldLayout           = oldLayout;
    barrier.newLayout           = newLayout;
    barrier.image               = image;
    barrier.subresourceRange    = subresourceRange;

    switch (oldLayout)
    {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            barrier.srcAccessMask = 0;
            break;

        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            break;
    }

    switch (newLayout)
    {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            barrier.dstAccessMask = barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            if (barrier.srcAccessMask == 0)
            {
                barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
            }
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            break;
    }
    vkCmdPipelineBarrier(command_buffer, srcMask, dstMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

VkCommandBuffer Texture::createCommandBuffer(VkCommandBufferLevel level, bool begin)
{
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

void Texture::flushCommandBuffer(VkCommandBuffer command_buffer, VkQueue queue, bool free, VkSemaphore signalSemaphore) 
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

uint32_t Texture::findMemoryType(uint32_t typeFilter) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_device->getPhysicalDevice(), &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & m_memoryPropertyFlags) == m_memoryPropertyFlags) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

void Texture::destroy(){
    vkDestroySampler(m_device->getHandle(), m_sampler, nullptr);
    vkDestroyImageView(m_device->getHandle(), m_imageView, nullptr);
    vkDestroyImage(m_device->getHandle(), m_image, nullptr);
    vkFreeMemory(m_device->getHandle(), m_imageMemory, nullptr);
}

Texture::~Texture()
{
}