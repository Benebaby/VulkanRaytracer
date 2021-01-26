#pragma once

#include "GlobalDefs.h"

class Instance
{
    private:
        VkInstance m_handle;
        GLFWwindow* m_window;
        VkSurfaceKHR m_surface;
        VkDebugUtilsMessengerEXT m_debugMessenger;
        std::string m_title;
        uint32_t m_width, m_height;
        uint32_t m_apiVersion;
        bool m_validation;
        bool m_resized;
        std::vector<const char*> m_extensions;
        std::vector<const char*> m_layers;

        static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
            auto instance = reinterpret_cast<Instance*>(glfwGetWindowUserPointer(window));
            instance->m_resized = true;
        }
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
            if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
                std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
            return VK_FALSE;
        }
        VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
        void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
    public:
        Instance();
        Instance(std::string title, uint32_t width, uint32_t height, uint32_t apiVersion, bool validation);
        ~Instance();
        void addExtension(const char* extension);
        const std::vector<const char*>& getLayers() const;
        void addLayer(const char* layer);
        void create();
        void createSurface();
        VkInstance getHandle();
        VkSurfaceKHR getSurface();
        GLFWwindow* getWindow();
        bool isResized();
        bool hasValidation();
        void setResized(bool resized);
        void destroy();
};
