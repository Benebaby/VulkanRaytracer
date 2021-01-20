#include "Instance.h"

Instance::Instance(){

}

Instance::Instance(std::string title, uint32_t width, uint32_t height, uint32_t apiVersion, std::vector<const char*> validationLayers){
    m_width = width;
    m_height = height;
    m_resized = false;
    m_title = title;
    m_apiVersion = apiVersion;
    if(validationLayers.size() > 0)
        m_validation = true;
    else
        m_validation = false;

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    m_extensions = std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (m_validation) {
        m_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

     if(m_validation){
        uint32_t supportedLayersCount;
        vkEnumerateInstanceLayerProperties(&supportedLayersCount, nullptr);
        std::vector<VkLayerProperties> supportedLayers(supportedLayersCount);
        vkEnumerateInstanceLayerProperties(&supportedLayersCount, supportedLayers.data());

        for(const char* layer : validationLayers){
            std::string layerCurrent = layer;
            bool layerFound = false;
            for (const VkLayerProperties layerProps : supportedLayers) {
                std::string supportedLayer = layerProps.layerName;
                if(layerCurrent == supportedLayer){
                    layerFound = true;
                    break;
                }
            }
            if(!layerFound)
                throw std::runtime_error("Requested Validation Layer not Supported!");
        }
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = m_title.c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "RTV";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = m_apiVersion;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugCallback;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(m_extensions.size());
    createInfo.ppEnabledExtensionNames = m_extensions.data();
    if (m_validation) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &m_handle) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }

    if (m_validation){
        if (CreateDebugUtilsMessengerEXT(m_handle, &debugCreateInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }
}

void Instance::addExtension(const char* extension){
    m_extensions.push_back(extension);
}

void Instance::createSurface(){
    if (glfwCreateWindowSurface(m_handle, m_window, nullptr, &m_surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

GLFWwindow* Instance::getWindow(){
    return m_window;
}

VkInstance Instance::getHandle(){
    return m_handle;
}

VkSurfaceKHR Instance::getSurface(){
    return m_surface;
}

bool Instance::isResized(){
    return m_resized;
}

void Instance::setResized(bool resized){
    m_resized = resized;
}

bool Instance::hasValidation(){
    return m_validation;
}

VkResult Instance::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void Instance::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

Instance::~Instance(){}

void Instance::destroy(){
    if (m_validation) {
        DestroyDebugUtilsMessengerEXT(m_handle, m_debugMessenger, nullptr);
    }
    vkDestroySurfaceKHR(m_handle, m_surface, nullptr);
    vkDestroyInstance(m_handle, nullptr);
    glfwDestroyWindow(m_window);
    glfwTerminate();
}