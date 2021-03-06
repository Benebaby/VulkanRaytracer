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
#include <string>
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

struct UniformBufferObject {
    glm::mat4 view;
    glm::mat4 proj;
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

struct Light
{
    Light(glm::vec3 pos, glm::vec3 color, float ambientIntensity){
        m_pos[0] = pos.x;
        m_pos[1] = pos.y;
        m_pos[2] = pos.z;
        m_pos[3] = 0.f;
        m_color[0] = color.x;
        m_color[1] = color.y;
        m_color[2] = color.z;
        m_ambientIntensity = ambientIntensity;
    }
    Light(glm::vec3 pos, glm::vec3 attenuation, glm::vec3 color, float ambientIntensity){
        m_pos[0] = pos.x;
        m_pos[1] = pos.y;
        m_pos[2] = pos.z;
        m_pos[3] = 1.f;
        m_attenuation[0] = attenuation.x;
        m_attenuation[1] = attenuation.y;
        m_attenuation[2] = attenuation.z;
        m_color[0] = color.x;
        m_color[1] = color.y;
        m_color[2] = color.z;
        m_ambientIntensity = ambientIntensity;
    }
    float m_pos[4];
    float m_attenuation[3];
    float m_ambientIntensity;
    float m_color[3];
    float pad;
};

struct Material
{
  float ambient[3];
  float pad0;
  float diffuse[3];
  float pad1;
  float specular[3];
  float pad2;
  float transmittance[3];
  float pad3;
  float emission[3];
  float shininess;
  float ior;                        // index of refraction
  float dissolve;                   // 1 == opaque; 0 == fully transparent
  int illum;                        // Beleuchtungsmodell
  int32_t ambientTexId;            // map_Ka
  int32_t diffuseTexId;            // map_Kd
  int32_t specularTexId;           // map_Ks
  int32_t specularHighlightTexId;  // map_Ns
  int32_t bumpTexId;               // map_bump, map_Bump, bump
  int32_t displacementTexId;       // disp
  int32_t alphaTexId;              // map_d
  int32_t reflectionTexId;         // refl
  float pad4;
};
