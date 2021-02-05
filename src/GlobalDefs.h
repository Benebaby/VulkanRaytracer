#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#define VK_FLAGS_NONE 0
#define DEFAULT_FENCE_TIMEOUT 100000000000
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <optional>
#include <set>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

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

struct StorageImage {
    VkDeviceMemory memory;
    VkImage        image = VK_NULL_HANDLE;
    VkImageView    view;
    VkFormat       format;
    uint32_t       width;
    uint32_t       height;
};

struct UniformBufferObject {
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec4 light;
};

struct Vertex
{
    float position[3];
    int matID;
    float normal[3];
    float pad0;
    float texture[2];
    float pad1[2];
};

struct Sphere
{
    float aabbmin[3];
    float aabbmax[3];
    float pad0[2];
    float center[3];
    float radius;
    int matID;
    float pad1[3];
};
