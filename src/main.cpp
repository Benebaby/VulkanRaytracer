#include <fstream>
#include <chrono>
#include <vulkan/vulkan.hpp>

#include "GlobalDefs.h"
#include "Instance.h"
#include "Device.h"
#include "Buffer.h"
#include "Texture.h"
#include "Camera.h"
#include "BottomLevelTriangleAS.h"
#include "BottomLevelSphereAS.h"

struct AccelerationStructure
{
	uint64_t                    device_address;
    Buffer                      buffer;
	VkAccelerationStructureKHR  accelerationStructure;
};

class VulkanRaytracer {
public:
    void run() {
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    Instance* m_instance;
    Device* m_device;

    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
    PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
    PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline rayTracingPipeline;
    
    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;

    std::vector<VkCommandBuffer> commandBuffers;

    Texture* storageImage;
	AccelerationStructure topLevelAccelerationStructure;

    Buffer* uniformBuffer;

    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups{};
    Buffer* raygenShaderBindingTable;
    Buffer* missShaderBindingTable;
    Buffer* hitShaderBindingTable;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;

    std::vector<BottomLevelAS*> BLAS;

    Camera cam;

    void initVulkan() {
        m_instance = new Instance("Vulkan Raytracing", 1600, 900, VK_API_VERSION_1_2, true);
        m_instance->addExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        m_instance->addLayer("VK_LAYER_KHRONOS_validation");
        m_instance->addLayer("VK_LAYER_LUNARG_monitor");
        m_instance->create();
        m_instance->createSurface();
        
        m_device = new Device(m_instance);
        m_device->addExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        m_device->addExtension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        m_device->addExtension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
        m_device->addExtension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
        m_device->addExtension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        m_device->addExtension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
        m_device->addExtension(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
        m_device->pickPhysicalDevice();
        m_device->createLogicalDevice();
        m_device->createCommandPool();

        createSwapChain();
        createImageViews();
        createRenderPass();
        createFramebuffers();
        
        cam = Camera(Camera::TypeFirstPerson ,m_instance->getWindow(), swapChainExtent.width, swapChainExtent.height, glm::vec3(-7.5f, 1.f, 0.f), glm::vec3(0.0f));

        getExtensionFunctionPointers();
        createStorageImage();

        BottomLevelTriangleAS* sibenik = new BottomLevelTriangleAS(m_device, "sibenik");
        sibenik->uploadData("/sibenik/sibenik.obj");
        sibenik->create();
        BLAS.push_back(sibenik);

        BottomLevelTriangleAS* rossi = new BottomLevelTriangleAS(m_device, "rossi");
        rossi->uploadData("/rossi/MCRN_Tachi.obj");
        rossi->create();
        BLAS.push_back(rossi);

        BottomLevelSphereAS* singleSphere = new BottomLevelSphereAS(m_device, "sphere0");
        singleSphere->createSpheres();
        singleSphere->create();
        BLAS.push_back(singleSphere);

        BottomLevelSphereAS* singleSphere1 = new BottomLevelSphereAS(m_device, "sphere1");
        singleSphere1->createSpheres();
        singleSphere1->create();
        BLAS.push_back(singleSphere1);

        createTopLevelAccelerationStructure();
        createUniformBuffer();
        createRayTracingPipeline();
        createShaderBindingTables();
        createDescriptorSets();
        createCommandBuffers();
        createSemaphores();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(m_instance->getWindow())) {
            glfwPollEvents();
            drawFrame();
        }
        vkDeviceWaitIdle(m_device->getHandle());
    }

    void handleResize(){
        int width = 0, height = 0;
        glfwGetFramebufferSize(m_instance->getWindow(), &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(m_instance->getWindow(), &width, &height);
            glfwWaitEvents();
        }
        vkDeviceWaitIdle(m_device->getHandle());
        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(m_device->getHandle(), framebuffer, nullptr);
        }
        vkFreeCommandBuffers(m_device->getHandle(), m_device->getCommandPool(), static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
        vkDestroyRenderPass(m_device->getHandle(), renderPass, nullptr);
        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(m_device->getHandle(), imageView, nullptr);
        }
        vkDestroySwapchainKHR(m_device->getHandle(), swapChain, nullptr);
        
        uniformBuffer->destroy();
        delete uniformBuffer;

        storageImage->destroy();
        vkDestroyDescriptorPool(m_device->getHandle(), descriptorPool, nullptr);

        createSwapChain();
        createImageViews();
        createRenderPass();
        createFramebuffers();
        createUniformBuffer();
        createStorageImage();
        createDescriptorSets();
        createCommandBuffers();
    }

    void cleanup() {
        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(m_device->getHandle(), framebuffer, nullptr);
        }

        vkFreeCommandBuffers(m_device->getHandle(), m_device->getCommandPool(), static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
        
        vkDestroyPipeline(m_device->getHandle(), rayTracingPipeline, nullptr);
        vkDestroyPipelineLayout(m_device->getHandle(), pipelineLayout, nullptr);
        vkDestroyRenderPass(m_device->getHandle(), renderPass, nullptr);

        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(m_device->getHandle(), imageView, nullptr);
        }

        vkDestroySwapchainKHR(m_device->getHandle(), swapChain, nullptr);
        
        uniformBuffer->destroy();
        delete uniformBuffer;


        storageImage->destroy();
        
        vkDestroyDescriptorPool(m_device->getHandle(), descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(m_device->getHandle(), descriptorSetLayout, nullptr);

        //-----
    
        BottomLevelAS::destroyTextures();
        BottomLevelAS::destroyMaterials();


        for(BottomLevelAS* blas : BLAS){
            blas->destroy();
            delete blas;
        }


        topLevelAccelerationStructure.buffer.destroy();
        vkDestroyAccelerationStructureKHR(m_device->getHandle(), topLevelAccelerationStructure.accelerationStructure, nullptr);


        raygenShaderBindingTable->destroy();
        delete raygenShaderBindingTable;
        missShaderBindingTable->destroy();
        delete missShaderBindingTable;
        hitShaderBindingTable->destroy();
        delete hitShaderBindingTable;

        vkDestroySemaphore(m_device->getHandle(), renderFinishedSemaphore, nullptr);
        vkDestroySemaphore(m_device->getHandle(), imageAvailableSemaphore, nullptr);

        m_device->destroy(); //destroys device and its command pool
        delete m_device;
        m_instance->destroy();
        delete m_instance;
    }

    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = m_device->querySwapChainSupport();

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_instance->getSurface();

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        QueueFamilyIndices indices = m_device->findQueueFamilies();
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(m_device->getHandle(), &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(m_device->getHandle(), swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(m_device->getHandle(), swapChain, &imageCount, swapChainImages.data());

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    void createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(m_device->getHandle(), &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image views!");
            }
        }
    }

    void getExtensionFunctionPointers(){
        vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(m_device->getHandle(), "vkCmdBuildAccelerationStructuresKHR"));
		vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(m_device->getHandle(), "vkBuildAccelerationStructuresKHR"));
		vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(m_device->getHandle(), "vkCreateAccelerationStructureKHR"));
		vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(m_device->getHandle(), "vkDestroyAccelerationStructureKHR"));
		vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(m_device->getHandle(), "vkGetAccelerationStructureBuildSizesKHR"));
		vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(m_device->getHandle(), "vkGetAccelerationStructureDeviceAddressKHR"));
		vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(m_device->getHandle(), "vkCmdTraceRaysKHR"));
		vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(m_device->getHandle(), "vkGetRayTracingShaderGroupHandlesKHR"));
		vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(m_device->getHandle(), "vkCreateRayTracingPipelinesKHR"));
    }

    Buffer createScratchBuffer(VkDeviceSize size){
        Buffer scratchBuffer = Buffer(m_device, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        return scratchBuffer;
    }

    void createStorageImage(){
        storageImage = new Texture(m_device, swapChainExtent.width, swapChainExtent.height, VK_FORMAT_B8G8R8A8_UNORM);
    }

    void createTopLevelAccelerationStructure(){
        BottomLevelAS::createMaterialBuffer(m_device);

        // VkTransformMatrixKHR transformMatrix0 = {
        //     1.0f, 0.0f, 0.0f, 0.0f,
        //     0.0f, 1.0f, 0.0f, -1.55f,
        //     0.0f, 0.0f, 1.0f, 0.0f
        // };
        VkTransformMatrixKHR transformMatrix0 = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 1.55f,
            0.0f, 0.0f, 1.0f, 0.0f
        };
        VkTransformMatrixKHR transformMatrix1 = {
            0.2f, 0.0f, 0.0f, -3.0f,
            0.0f, 0.2f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.2f, 0.5f
        };
        VkTransformMatrixKHR transformMatrix3 = {
            0.0f, -1.0f, 0.0f, 0.f,
            1.0f, 0.0f, 0.0f, 0.f,
            0.0f, 0.0f, 1.0f, -0.6f
        };
        VkTransformMatrixKHR transformMatrix4 = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.6f
        };

        VkAccelerationStructureInstanceKHR accelerationStructureInstance0{};
        accelerationStructureInstance0.transform                              = transformMatrix0;
        accelerationStructureInstance0.instanceCustomIndex                    = BLAS[0]->getId();
        accelerationStructureInstance0.mask                                   = 0xFF;
        accelerationStructureInstance0.instanceShaderBindingTableRecordOffset = 0;
        accelerationStructureInstance0.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        accelerationStructureInstance0.accelerationStructureReference         = BLAS[0]->getDeviceAdress();

        VkAccelerationStructureInstanceKHR accelerationStructureInstance1{};
        accelerationStructureInstance1.transform                              = transformMatrix1;
        accelerationStructureInstance1.instanceCustomIndex                    = BLAS[1]->getId();
        accelerationStructureInstance1.mask                                   = 0xFF;
        accelerationStructureInstance1.instanceShaderBindingTableRecordOffset = 0;
        accelerationStructureInstance1.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        accelerationStructureInstance1.accelerationStructureReference         = BLAS[1]->getDeviceAdress();

        VkAccelerationStructureInstanceKHR accelerationStructureInstance3{};
        accelerationStructureInstance3.transform                              = transformMatrix3;
        accelerationStructureInstance3.instanceCustomIndex                    = BLAS[2]->getId();
        accelerationStructureInstance3.mask                                   = 0xFF;
        accelerationStructureInstance3.instanceShaderBindingTableRecordOffset = 1;
        accelerationStructureInstance3.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        accelerationStructureInstance3.accelerationStructureReference         = BLAS[2]->getDeviceAdress();

        VkAccelerationStructureInstanceKHR accelerationStructureInstance4{};
        accelerationStructureInstance4.transform                              = transformMatrix4;
        accelerationStructureInstance4.instanceCustomIndex                    = BLAS[3]->getId();
        accelerationStructureInstance4.mask                                   = 0xFF;
        accelerationStructureInstance4.instanceShaderBindingTableRecordOffset = 1;
        accelerationStructureInstance4.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        accelerationStructureInstance4.accelerationStructureReference         = BLAS[3]->getDeviceAdress();


        std::vector<VkAccelerationStructureInstanceKHR> geometryInstances {accelerationStructureInstance0, accelerationStructureInstance1, accelerationStructureInstance3, accelerationStructureInstance4};
        VkDeviceSize geometryInstancesSize = geometryInstances.size() * sizeof(VkAccelerationStructureInstanceKHR);

        Buffer instancesBuffer = Buffer(m_device, geometryInstancesSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);        instancesBuffer.map(geometryInstancesSize, 0);
        instancesBuffer.copyTo(geometryInstances.data(), geometryInstancesSize);
        instancesBuffer.unmap();

        VkDeviceOrHostAddressConstKHR instancesDataDeviceAddress{};
	    instancesDataDeviceAddress.deviceAddress = instancesBuffer.getDeviceAddress();

        VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
        accelerationStructureGeometry.sType                              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        accelerationStructureGeometry.geometryType                       = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        accelerationStructureGeometry.flags                              = VK_GEOMETRY_OPAQUE_BIT_KHR;
        accelerationStructureGeometry.geometry.instances.sType           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
        accelerationStructureGeometry.geometry.instances.data            = instancesDataDeviceAddress;

        VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
        accelerationStructureBuildGeometryInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationStructureBuildGeometryInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        accelerationStructureBuildGeometryInfo.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationStructureBuildGeometryInfo.geometryCount = 1;
        accelerationStructureBuildGeometryInfo.pGeometries   = &accelerationStructureGeometry;

        const uint32_t primitiveCount = (uint32_t) geometryInstances.size();

        VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
        accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        vkGetAccelerationStructureBuildSizesKHR(m_device->getHandle(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &accelerationStructureBuildGeometryInfo, &primitiveCount, &accelerationStructureBuildSizesInfo);

        topLevelAccelerationStructure.buffer = Buffer(m_device, accelerationStructureBuildSizesInfo.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR);
        VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
        accelerationStructureCreateInfo.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        accelerationStructureCreateInfo.buffer = topLevelAccelerationStructure.buffer.getHandle();
        accelerationStructureCreateInfo.size   = accelerationStructureBuildSizesInfo.accelerationStructureSize;
        accelerationStructureCreateInfo.type   = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        vkCreateAccelerationStructureKHR(m_device->getHandle(), &accelerationStructureCreateInfo, nullptr, &topLevelAccelerationStructure.accelerationStructure);

        Buffer scratchBuffer = createScratchBuffer(accelerationStructureBuildSizesInfo.buildScratchSize);

        VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
        accelerationBuildGeometryInfo.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationBuildGeometryInfo.type                      = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        accelerationBuildGeometryInfo.flags                     = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationBuildGeometryInfo.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        accelerationBuildGeometryInfo.dstAccelerationStructure  = topLevelAccelerationStructure.accelerationStructure;
        accelerationBuildGeometryInfo.geometryCount             = 1;
        accelerationBuildGeometryInfo.pGeometries               = &accelerationStructureGeometry;
        accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.getDeviceAddress();

        VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo;
        accelerationStructureBuildRangeInfo.primitiveCount                                           = (uint32_t) geometryInstances.size();
        accelerationStructureBuildRangeInfo.primitiveOffset                                          = 0;
        accelerationStructureBuildRangeInfo.firstVertex                                              = 0;
        accelerationStructureBuildRangeInfo.transformOffset                                          = 0;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR *> accelerationBuildStructureRangeInfos = {&accelerationStructureBuildRangeInfo};

        if (m_device->supportsAccelerationStructureHostCommands())
		{
			vkBuildAccelerationStructuresKHR(m_device->getHandle(), VK_NULL_HANDLE, 1, &accelerationBuildGeometryInfo, accelerationBuildStructureRangeInfos.data());
		}else{
            VkCommandBuffer command_buffer = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
            vkCmdBuildAccelerationStructuresKHR(command_buffer, 1, &accelerationBuildGeometryInfo, accelerationBuildStructureRangeInfos.data());
            flushCommandBuffer(command_buffer, m_device->getGraphicsQueue());
            scratchBuffer.destroy();
        }

        VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
        accelerationDeviceAddressInfo.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        accelerationDeviceAddressInfo.accelerationStructure = topLevelAccelerationStructure.accelerationStructure;
        topLevelAccelerationStructure.device_address        = vkGetAccelerationStructureDeviceAddressKHR(m_device->getHandle(), &accelerationDeviceAddressInfo);

        instancesBuffer.destroy();
    }

    void updateUniformBuffer(){
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        glm::mat3 camRotation = glm::mat3(glm::rotate(glm::mat4(1.0f), (time/5) * glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
        // sponza 
        //ubo.view = glm::lookAt(glm::vec3(-7.5f, 2.f, 0.f), glm::vec3(0.0f, 3.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        // viking_room
        // ubo.view = glm::lookAt(glm::vec3(-1.5, 1.1, 1.5), glm::vec3(0.0f, 0.3f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        // dragon
        //ubo.view = glm::lookAt(glm::vec3(0.75, 0.6, 0.75), glm::vec3(0.0f, 0.3f, 0), glm::vec3(0.0f, 1.0f, 0.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float) swapChainExtent.height, 0.1f, 1000.0f);
        ubo.proj[1][1] *= -1;

        cam.update();
        ubo.view = cam.getView();
        ubo.proj = glm::inverse(ubo.proj);
        ubo.light = glm::vec4(glm::vec3(-1.f, 1.0f, 1.f) * camRotation, 1.0f);

        uniformBuffer->map(sizeof(ubo), 0);
        uniformBuffer->copyTo(&ubo, sizeof(ubo));
        uniformBuffer->unmap();
    }

    void createUniformBuffer(){
        uniformBuffer = new Buffer(m_device, sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        updateUniformBuffer();
    }

    void createDescriptorSets(){
        std::vector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, BottomLevelAS::getTextureCount()}
        };
        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolInfo.pPoolSizes    = poolSizes.data();
        descriptorPoolInfo.maxSets       = 1;
        if(vkCreateDescriptorPool(m_device->getHandle(), &descriptorPoolInfo, nullptr, &descriptorPool))
            throw std::runtime_error("failed to create descriptor pool!");

        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
        descriptorSetAllocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.descriptorPool     = descriptorPool;
        descriptorSetAllocateInfo.pSetLayouts        = &descriptorSetLayout;
        descriptorSetAllocateInfo.descriptorSetCount = 1;
        if(vkAllocateDescriptorSets(m_device->getHandle(), &descriptorSetAllocateInfo, &descriptorSet))
            throw std::runtime_error("failed to allocate descriptor sets!");


        VkWriteDescriptorSetAccelerationStructureKHR descriptor_acceleration_structure_info{};
        descriptor_acceleration_structure_info.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        descriptor_acceleration_structure_info.accelerationStructureCount = 1;
        descriptor_acceleration_structure_info.pAccelerationStructures    = &topLevelAccelerationStructure.accelerationStructure;

        VkWriteDescriptorSet accelerationStructureWrite{};
        accelerationStructureWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        accelerationStructureWrite.dstSet          = descriptorSet;
        accelerationStructureWrite.dstBinding      = 0;
        accelerationStructureWrite.descriptorCount = 1;
        accelerationStructureWrite.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        accelerationStructureWrite.pNext = &descriptor_acceleration_structure_info;

        VkDescriptorImageInfo storageImageDescriptor = storageImage->getDescriptorInfo();

        VkWriteDescriptorSet resultImageWrite{};
        resultImageWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        resultImageWrite.dstSet          = descriptorSet;
        resultImageWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        resultImageWrite.dstBinding      = 1;
        resultImageWrite.pImageInfo      = &storageImageDescriptor;
        resultImageWrite.descriptorCount = 1;

        VkDescriptorBufferInfo uniformBufferDescriptor = uniformBuffer->getDescriptorInfo(sizeof(UniformBufferObject), 0);

        VkWriteDescriptorSet uniformBufferWrite{};
        uniformBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        uniformBufferWrite.dstSet = descriptorSet;
        uniformBufferWrite.dstBinding = 2;
        uniformBufferWrite.dstArrayElement = 0;
        uniformBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBufferWrite.descriptorCount = 1;
        uniformBufferWrite.pBufferInfo = &uniformBufferDescriptor;

        VkWriteDescriptorSet vertexBufferWrite{};
        vertexBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vertexBufferWrite.dstSet = descriptorSet;
        vertexBufferWrite.dstBinding = 3;
        vertexBufferWrite.dstArrayElement = 0;
        vertexBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        vertexBufferWrite.descriptorCount = BottomLevelTriangleAS::getCount();
        vertexBufferWrite.pBufferInfo = BottomLevelTriangleAS::getVertexBufferDescriptors();

        VkWriteDescriptorSet indexBufferWrite{};
        indexBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        indexBufferWrite.dstSet = descriptorSet;
        indexBufferWrite.dstBinding = 4;
        indexBufferWrite.dstArrayElement = 0;
        indexBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        indexBufferWrite.descriptorCount = BottomLevelTriangleAS::getCount();
        indexBufferWrite.pBufferInfo = BottomLevelTriangleAS::getIndexBufferDescriptors();
        
        VkWriteDescriptorSet textureImageWrite{};
        textureImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        textureImageWrite.dstSet = descriptorSet;
        textureImageWrite.dstBinding = 5;
        textureImageWrite.dstArrayElement = 0;
        textureImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        textureImageWrite.descriptorCount = BottomLevelAS::getTextureCount();
        textureImageWrite.pImageInfo = BottomLevelAS::getTextureDescriptors();

        VkWriteDescriptorSet sphereBufferWrite{};
        sphereBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        sphereBufferWrite.dstSet = descriptorSet;
        sphereBufferWrite.dstBinding = 6;
        sphereBufferWrite.dstArrayElement = 0;
        sphereBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        sphereBufferWrite.descriptorCount = BottomLevelSphereAS::getCount();
        sphereBufferWrite.pBufferInfo = BottomLevelSphereAS::getSphereBufferDescriptors();


        VkWriteDescriptorSet materialBufferWrite{};
        materialBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        materialBufferWrite.dstSet = descriptorSet;
        materialBufferWrite.dstBinding = 7;
        materialBufferWrite.dstArrayElement = 0;
        materialBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        materialBufferWrite.descriptorCount = 1;
        materialBufferWrite.pBufferInfo = BottomLevelAS::getMaterialBufferDescriptor();

        std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
            accelerationStructureWrite,
	        resultImageWrite,
            uniformBufferWrite,
            vertexBufferWrite,
            indexBufferWrite,
            textureImageWrite,
            sphereBufferWrite,
            materialBufferWrite
        };

        vkUpdateDescriptorSets(m_device->getHandle(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, VK_NULL_HANDLE);
    }

    void createRenderPass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(m_device->getHandle(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
            throw std::runtime_error("failed to create render pass!");
    }

    void createRayTracingPipeline(){
        VkDescriptorSetLayoutBinding acceleration_structure_layout_binding{};
        acceleration_structure_layout_binding.binding         = 0;
        acceleration_structure_layout_binding.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        acceleration_structure_layout_binding.descriptorCount = 1;
        acceleration_structure_layout_binding.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

        VkDescriptorSetLayoutBinding result_image_layout_binding{};
        result_image_layout_binding.binding         = 1;
        result_image_layout_binding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        result_image_layout_binding.descriptorCount = 1;
        result_image_layout_binding.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

        VkDescriptorSetLayoutBinding uniform_buffer_binding{};
        uniform_buffer_binding.binding         = 2;
        uniform_buffer_binding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniform_buffer_binding.descriptorCount = 1;
        uniform_buffer_binding.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;

        VkDescriptorSetLayoutBinding vertex_buffer_binding{};
        vertex_buffer_binding.binding         = 3;
        vertex_buffer_binding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        vertex_buffer_binding.descriptorCount = BottomLevelTriangleAS::getCount();
        vertex_buffer_binding.stageFlags      = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR;

        VkDescriptorSetLayoutBinding index_buffer_binding{};
        index_buffer_binding.binding         = 4;
        index_buffer_binding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        index_buffer_binding.descriptorCount = BottomLevelTriangleAS::getCount();
        index_buffer_binding.stageFlags      = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR;

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 5;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.descriptorCount = BottomLevelAS::getTextureCount();
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR;

        VkDescriptorSetLayoutBinding sphere_buffer_binding{};
        sphere_buffer_binding.binding         = 6;
        sphere_buffer_binding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        sphere_buffer_binding.descriptorCount = BottomLevelSphereAS::getCount();
        sphere_buffer_binding.stageFlags      = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR;

        VkDescriptorSetLayoutBinding material_buffer_binding{};
        material_buffer_binding.binding         = 7;
        material_buffer_binding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        material_buffer_binding.descriptorCount = 1;
        material_buffer_binding.stageFlags      = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR;

        std::vector<VkDescriptorSetLayoutBinding> bindings = {
            acceleration_structure_layout_binding,
            result_image_layout_binding,
            uniform_buffer_binding,
            vertex_buffer_binding,
            index_buffer_binding,
            samplerLayoutBinding,
            sphere_buffer_binding,
            material_buffer_binding
        };

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings    = bindings.data();
        if(vkCreateDescriptorSetLayout(m_device->getHandle(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor set layout!");

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts    = &descriptorSetLayout;

        if(vkCreatePipelineLayout(m_device->getHandle(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("failed to create pipeline layout!");

        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

        auto raygenShaderCode = readFile("/raygen.spv");
        auto missShaderCode = readFile("/miss.spv");
        auto missShadowShaderCode = readFile("/shadow.spv");
        auto rchitShaderCode = readFile("/closesthit.spv");
        auto rchitSphereShaderCode = readFile("/closesthitsphere.spv");
        auto rintShaderCode = readFile("/intersection.spv");
        auto rahitShaderCode = readFile("/anyhit.spv");
        VkShaderModule raygenShaderModule = createShaderModule(raygenShaderCode);
        VkShaderModule missShaderModule = createShaderModule(missShaderCode);
        VkShaderModule missShadowShaderModule = createShaderModule(missShadowShaderCode);
        VkShaderModule rchitShaderModule = createShaderModule(rchitShaderCode);
        VkShaderModule rchitSphereShaderModule = createShaderModule(rchitSphereShaderCode);
        VkShaderModule rintShaderModule = createShaderModule(rintShaderCode);
        VkShaderModule rahitShaderModule = createShaderModule(rahitShaderCode);

        VkPipelineShaderStageCreateInfo raygenShaderStageInfo{};
        raygenShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        raygenShaderStageInfo.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
        raygenShaderStageInfo.module = raygenShaderModule;
        raygenShaderStageInfo.pName = "main";
        shaderStages.push_back(raygenShaderStageInfo);

        VkRayTracingShaderGroupCreateInfoKHR raygenGroupCreateInfo{};
		raygenGroupCreateInfo.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		raygenGroupCreateInfo.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		raygenGroupCreateInfo.generalShader      = static_cast<uint32_t>(shaderStages.size()) - 1;
		raygenGroupCreateInfo.closestHitShader   = VK_SHADER_UNUSED_KHR;
		raygenGroupCreateInfo.anyHitShader       = VK_SHADER_UNUSED_KHR;
		raygenGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(raygenGroupCreateInfo);

        VkPipelineShaderStageCreateInfo missShaderStageInfo{};
        missShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        missShaderStageInfo.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
        missShaderStageInfo.module = missShaderModule;
        missShaderStageInfo.pName = "main";
        shaderStages.push_back(missShaderStageInfo);

        VkRayTracingShaderGroupCreateInfoKHR missGroupCreateInfo{};
		missGroupCreateInfo.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		missGroupCreateInfo.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		missGroupCreateInfo.generalShader      = static_cast<uint32_t>(shaderStages.size()) - 1;
		missGroupCreateInfo.closestHitShader   = VK_SHADER_UNUSED_KHR;
		missGroupCreateInfo.anyHitShader       = VK_SHADER_UNUSED_KHR;
		missGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(missGroupCreateInfo);

        VkPipelineShaderStageCreateInfo missShadowShaderStageInfo{};
        missShadowShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        missShadowShaderStageInfo.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
        missShadowShaderStageInfo.module = missShadowShaderModule;
        missShadowShaderStageInfo.pName = "main";
        shaderStages.push_back(missShadowShaderStageInfo);

        missGroupCreateInfo.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
        shaderGroups.push_back(missGroupCreateInfo);

        VkPipelineShaderStageCreateInfo rchitShaderStageInfo{};
        rchitShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        rchitShaderStageInfo.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        rchitShaderStageInfo.module = rchitShaderModule;
        rchitShaderStageInfo.pName = "main";
        shaderStages.push_back(rchitShaderStageInfo);

        VkRayTracingShaderGroupCreateInfoKHR closesHitGroupCreateInfo{};
		closesHitGroupCreateInfo.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		closesHitGroupCreateInfo.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		closesHitGroupCreateInfo.generalShader      = VK_SHADER_UNUSED_KHR;
		closesHitGroupCreateInfo.closestHitShader   = static_cast<uint32_t>(shaderStages.size()) - 1;
		closesHitGroupCreateInfo.anyHitShader       = static_cast<uint32_t>(shaderStages.size());
		closesHitGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(closesHitGroupCreateInfo);

        VkPipelineShaderStageCreateInfo rahitShaderStageInfo{};
        rahitShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        rahitShaderStageInfo.stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
        rahitShaderStageInfo.module = rahitShaderModule;
        rahitShaderStageInfo.pName = "main";
        shaderStages.push_back(rahitShaderStageInfo);

        VkPipelineShaderStageCreateInfo rchitSphereShaderStageInfo{};
        rchitSphereShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        rchitSphereShaderStageInfo.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        rchitSphereShaderStageInfo.module = rchitSphereShaderModule;
        rchitSphereShaderStageInfo.pName = "main";
        shaderStages.push_back(rchitSphereShaderStageInfo);

         VkRayTracingShaderGroupCreateInfoKHR rchitSphereGroupCreateInfo{};
		rchitSphereGroupCreateInfo.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		rchitSphereGroupCreateInfo.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
		rchitSphereGroupCreateInfo.generalShader      = VK_SHADER_UNUSED_KHR;
		rchitSphereGroupCreateInfo.closestHitShader   = static_cast<uint32_t>(shaderStages.size()) - 1;
		rchitSphereGroupCreateInfo.anyHitShader       = VK_SHADER_UNUSED_KHR;
		rchitSphereGroupCreateInfo.intersectionShader = static_cast<uint32_t>(shaderStages.size());
		shaderGroups.push_back(rchitSphereGroupCreateInfo);

        VkPipelineShaderStageCreateInfo rintShaderStageInfo{};
        rintShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        rintShaderStageInfo.stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
        rintShaderStageInfo.module = rintShaderModule;
        rintShaderStageInfo.pName = "main";
        shaderStages.push_back(rintShaderStageInfo);

        VkRayTracingPipelineCreateInfoKHR raytracingPipelineCreateInfo{};
        raytracingPipelineCreateInfo.sType                        = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
        raytracingPipelineCreateInfo.stageCount                   = static_cast<uint32_t>(shaderStages.size());
        raytracingPipelineCreateInfo.pStages                      = shaderStages.data();
        raytracingPipelineCreateInfo.groupCount                   = static_cast<uint32_t>(shaderGroups.size());
        raytracingPipelineCreateInfo.pGroups                      = shaderGroups.data();
        raytracingPipelineCreateInfo.maxPipelineRayRecursionDepth = 31;
        raytracingPipelineCreateInfo.layout                       = pipelineLayout;

        if(vkCreateRayTracingPipelinesKHR(m_device->getHandle(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &raytracingPipelineCreateInfo, nullptr, &rayTracingPipeline) != VK_SUCCESS)
            throw std::runtime_error("failed to create ray tracing pipeline!");
        vkDestroyShaderModule(m_device->getHandle(), raygenShaderModule, nullptr);
        vkDestroyShaderModule(m_device->getHandle(), missShaderModule, nullptr);
        vkDestroyShaderModule(m_device->getHandle(), missShadowShaderModule, nullptr);
        vkDestroyShaderModule(m_device->getHandle(), rchitShaderModule, nullptr);
        vkDestroyShaderModule(m_device->getHandle(), rchitSphereShaderModule, nullptr);
        vkDestroyShaderModule(m_device->getHandle(), rintShaderModule, nullptr);
        vkDestroyShaderModule(m_device->getHandle(), rahitShaderModule, nullptr);
    }

    void createShaderBindingTables(){
        const uint32_t handleSize = m_device->getShaderGroupHandleSize();
		const uint32_t handleSizeAligned = alignedSize(m_device->getShaderGroupHandleSize(), m_device-> getShaderGroupHandleAlignment());
		const uint32_t groupCount = static_cast<uint32_t>(shaderGroups.size());
		const uint32_t sbtSize = groupCount * handleSizeAligned;

		std::vector<uint8_t> shaderHandleStorage(sbtSize);
		if(vkGetRayTracingShaderGroupHandlesKHR(m_device->getHandle(), rayTracingPipeline, 0, groupCount, sbtSize, shaderHandleStorage.data()) != VK_SUCCESS)
            throw std::runtime_error("failed to get ray tracing shader group handles!");

		const VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		const VkMemoryPropertyFlags memoryUsageFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        raygenShaderBindingTable = new Buffer(m_device, handleSize, bufferUsageFlags, memoryUsageFlags);
        raygenShaderBindingTable->map(handleSize, 0);
        raygenShaderBindingTable->copyTo(shaderHandleStorage.data(), handleSize);
        raygenShaderBindingTable->unmap();

        missShaderBindingTable = new Buffer(m_device, handleSize * 2, bufferUsageFlags, memoryUsageFlags);
        missShaderBindingTable->map(handleSize * 2, 0);
        missShaderBindingTable->copyTo(shaderHandleStorage.data() + handleSizeAligned, handleSize * 2);
        missShaderBindingTable->unmap();

        hitShaderBindingTable = new Buffer(m_device, handleSize * 2, bufferUsageFlags, memoryUsageFlags);
        hitShaderBindingTable->map(handleSize * 2, 0);
        hitShaderBindingTable->copyTo(shaderHandleStorage.data() + handleSizeAligned * 3, handleSize * 2);
        hitShaderBindingTable->unmap();
    }

    void createFramebuffers() {
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            VkImageView attachments[] = {
                swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(m_device->getHandle(), &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void createCommandBuffers() {
        commandBuffers.resize(swapChainFramebuffers.size());

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_device->getCommandPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

        if (vkAllocateCommandBuffers(m_device->getHandle(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkImageSubresourceRange subresource_range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        for (size_t i = 0; i < commandBuffers.size(); i++) {

            if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
                throw std::runtime_error("failed to begin recording command buffer!");
            }

            /*
                Setup the strided device address regions pointing at the shader identifiers in the shader binding table
            */

            const uint32_t handle_size_aligned = alignedSize(m_device->getShaderGroupHandleSize(), m_device-> getShaderGroupHandleAlignment());

            VkStridedDeviceAddressRegionKHR raygen_shader_sbt_entry{};
            raygen_shader_sbt_entry.deviceAddress = raygenShaderBindingTable->getDeviceAddress();
            raygen_shader_sbt_entry.stride        = handle_size_aligned;
            raygen_shader_sbt_entry.size          = handle_size_aligned;

            VkStridedDeviceAddressRegionKHR miss_shader_sbt_entry{};
            miss_shader_sbt_entry.deviceAddress = missShaderBindingTable->getDeviceAddress();
            miss_shader_sbt_entry.stride        = handle_size_aligned;
            miss_shader_sbt_entry.size          = handle_size_aligned * 2;

            VkStridedDeviceAddressRegionKHR hit_shader_sbt_entry{};
            hit_shader_sbt_entry.deviceAddress = hitShaderBindingTable->getDeviceAddress();
            hit_shader_sbt_entry.stride        = handle_size_aligned;
            hit_shader_sbt_entry.size          = handle_size_aligned * 2;

            VkStridedDeviceAddressRegionKHR callable_shader_sbt_entry{};

            /*
                Dispatch the ray tracing commands
            */
            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rayTracingPipeline);
            vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineLayout, 0, 1, &descriptorSet, 0, 0);

            vkCmdTraceRaysKHR(commandBuffers[i], &raygen_shader_sbt_entry, &miss_shader_sbt_entry, &hit_shader_sbt_entry, &callable_shader_sbt_entry, swapChainExtent.width, swapChainExtent.height, 1);

            /*
                Copy ray tracing output to swap chain image
            */

            // Prepare current swap chain image as transfer destination
            setImageLayout(commandBuffers[i], swapChainImages[i], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresource_range);

            // Prepare ray tracing output image as transfer source
            setImageLayout( commandBuffers[i], storageImage->getImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresource_range);

            VkImageCopy copy_region{};
            copy_region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
            copy_region.srcOffset      = {0, 0, 0};
            copy_region.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
            copy_region.dstOffset      = {0, 0, 0};
            copy_region.extent         = {swapChainExtent.width, swapChainExtent.height, 1};
            vkCmdCopyImage(commandBuffers[i], storageImage->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapChainImages[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

            // Transition swap chain image back for presentation
            setImageLayout(commandBuffers[i], swapChainImages[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, subresource_range);

            // Transition ray tracing output image back to general layout
            setImageLayout(commandBuffers[i], storageImage->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, subresource_range);

            if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to record command buffer!");
            }
        }
    }

    void drawFrame() {
        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(m_device->getHandle(), swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            handleResize();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        updateUniformBuffer();

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(m_device->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }
        
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; // Optional
        result = vkQueuePresentKHR(m_device->getPresentQueue(), &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_instance->isResized()) {
            m_instance->setResized(false);
            handleResize();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }
        vkQueueWaitIdle(m_device->getPresentQueue());
    }

    void createSemaphores(){
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        if (vkCreateSemaphore(m_device->getHandle(), &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS || vkCreateSemaphore(m_device->getHandle(), &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphores!");
        }
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(m_device->getHandle(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(m_instance->getWindow(), &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = glm::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = glm::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

            return actualExtent;
        }
    }

    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(SHADER_PATH+filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_device->getPhysicalDevice(), &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void setImageLayout(VkCommandBuffer command_buffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subresourceRange, VkPipelineStageFlags srcMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VkPipelineStageFlags dstMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT)
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

    VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, bool begin)
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

    void flushCommandBuffer(VkCommandBuffer command_buffer, VkQueue queue, bool free = true, VkSemaphore signalSemaphore = VK_NULL_HANDLE) 
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

    inline uint32_t alignedSize(uint32_t value, uint32_t alignment)
    {
        return (value + alignment - 1) & ~(alignment - 1);
    }

}; 

int main() {
    VulkanRaytracer app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}