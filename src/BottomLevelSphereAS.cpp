#include "BottomLevelSphereAS.h"

uint32_t BottomLevelSphereAS::m_count = 0;
std::vector<VkDescriptorBufferInfo> BottomLevelSphereAS::m_sphereBufferDescriptors;

BottomLevelSphereAS::BottomLevelSphereAS(Device* device, std::string name) : BottomLevelAS(device, name, m_count){
    m_count++;
}

VkDescriptorBufferInfo* BottomLevelSphereAS::getSphereBufferDescriptors(){
    return m_sphereBufferDescriptors.data();
}

void BottomLevelSphereAS::createSphere(Sphere &sphere, tinyobj::material_t &material_in){
    uint32_t materialOffset = static_cast<uint32_t>(m_materials.size());

    Material material{};
    material.ambient[0] = material_in.ambient[0];               material.ambient[1] = material_in.ambient[1];               material.ambient[2] = material_in.ambient[2];
    material.diffuse[0] = material_in.diffuse[0];               material.diffuse[1] = material_in.diffuse[1];               material.diffuse[2] = material_in.diffuse[2];
    material.specular[0] = material_in.specular[0];             material.specular[1] = material_in.specular[1];             material.specular[2] = material_in.specular[2];
    material.transmittance[0] = material_in.transmittance[0];   material.transmittance[1] = material_in.transmittance[1];   material.transmittance[2] = material_in.transmittance[2];
    material.emission[0] = material_in.emission[0];             material.emission[1] = material_in.emission[1];             material.emission[2] = material_in.emission[2];
    material.shininess = material_in.shininess;
    material.ior = material_in.ior;                     // index of refraction
    material.dissolve = material_in.dissolve;                // 1 == opaque; 0 == fully transparent
    material.illum = material_in.illum;                   // Beleuchtungsmodell
    if(material_in.ambient_texname.length() != 0 ){
        m_textures.push_back(Texture(m_device, material_in.ambient_texname, VK_FORMAT_R8G8B8A8_SRGB));
        material.ambientTexId = static_cast<uint32_t>(m_textures.size() - 1);            // map_Ka
    }else{
        material.ambientTexId = -1;
    }
    if(material_in.diffuse_texname.length() != 0 ){
        m_textures.push_back(Texture(m_device, material_in.diffuse_texname, VK_FORMAT_R8G8B8A8_SRGB));
        material.diffuseTexId = static_cast<uint32_t>(m_textures.size() - 1);            // map_Kd
    }else{
        material.diffuseTexId = -1;
    }
    if(material_in.specular_texname.length() != 0 ){
        m_textures.push_back(Texture(m_device, material_in.specular_texname, VK_FORMAT_R8G8B8A8_SRGB));
        material.specularTexId = static_cast<uint32_t>(m_textures.size() - 1);            // map_Ks
    }else{
        material.specularTexId = -1;
    }
    if(material_in.specular_highlight_texname.length() != 0 ){
        m_textures.push_back(Texture(m_device, material_in.specular_highlight_texname, VK_FORMAT_R8G8B8A8_SRGB));
        material.specularHighlightTexId = static_cast<uint32_t>(m_textures.size() - 1);            // map_Ns
    }else{
        material.specularHighlightTexId = -1;
    }
    if(material_in.bump_texname.length() != 0 ){
        m_textures.push_back(Texture(m_device, material_in.bump_texname, VK_FORMAT_R8G8B8A8_SRGB));
        material.bumpTexId = static_cast<uint32_t>(m_textures.size() - 1);            // map_bump, map_Bump, bump
    }else{
        material.bumpTexId = -1;
    }
    if(material_in.displacement_texname.length() != 0 ){
        m_textures.push_back(Texture(m_device, material_in.displacement_texname, VK_FORMAT_R8G8B8A8_SRGB));
        material.displacementTexId = static_cast<uint32_t>(m_textures.size() - 1);            // disp
    }else{
        material.displacementTexId = -1;
    }
    if(material_in.alpha_texname.length() != 0 ){
        m_textures.push_back(Texture(m_device, material_in.alpha_texname, VK_FORMAT_R8G8B8A8_SRGB));
        material.alphaTexId = static_cast<uint32_t>(m_textures.size() - 1);            // map_d
    }else{
        material.alphaTexId = -1;
    }
    if(material_in.reflection_texname.length() != 0 ){
        m_textures.push_back(Texture(m_device, material_in.reflection_texname, VK_FORMAT_R8G8B8A8_SRGB));
        material.reflectionTexId = static_cast<uint32_t>(m_textures.size() - 1);            // refl
    }else{
        material.reflectionTexId = -1;
    }
    m_materials.push_back(material);

    sphere.matID = materialOffset;
    m_spheres.push_back(sphere);
}

void BottomLevelSphereAS::createSpheres(std::vector<Sphere> &spheres, tinyobj::material_t &material_in){
    uint32_t materialOffset = static_cast<uint32_t>(m_materials.size());

    Material material{};
    material.ambient[0] = material_in.ambient[0];               material.ambient[1] = material_in.ambient[1];               material.ambient[2] = material_in.ambient[2];
    material.diffuse[0] = material_in.diffuse[0];               material.diffuse[1] = material_in.diffuse[1];               material.diffuse[2] = material_in.diffuse[2];
    material.specular[0] = material_in.specular[0];             material.specular[1] = material_in.specular[1];             material.specular[2] = material_in.specular[2];
    material.transmittance[0] = material_in.transmittance[0];   material.transmittance[1] = material_in.transmittance[1];   material.transmittance[2] = material_in.transmittance[2];
    material.emission[0] = material_in.emission[0];             material.emission[1] = material_in.emission[1];             material.emission[2] = material_in.emission[2];
    material.shininess = material_in.shininess;
    material.ior = material_in.ior;                     // index of refraction
    material.dissolve = material_in.dissolve;                // 1 == opaque; 0 == fully transparent
    material.illum = material_in.illum;                   // Beleuchtungsmodell
    if(material_in.ambient_texname.length() != 0 ){
        m_textures.push_back(Texture(m_device, material_in.ambient_texname, VK_FORMAT_R8G8B8A8_SRGB));
        material.ambientTexId = static_cast<uint32_t>(m_textures.size() - 1);            // map_Ka
    }else{
        material.ambientTexId = -1;
    }
    if(material_in.diffuse_texname.length() != 0 ){
        m_textures.push_back(Texture(m_device, material_in.diffuse_texname, VK_FORMAT_R8G8B8A8_SRGB));
        material.diffuseTexId = static_cast<uint32_t>(m_textures.size() - 1);            // map_Kd
    }else{
        material.diffuseTexId = -1;
    }
    if(material_in.specular_texname.length() != 0 ){
        m_textures.push_back(Texture(m_device, material_in.specular_texname, VK_FORMAT_R8G8B8A8_SRGB));
        material.specularTexId = static_cast<uint32_t>(m_textures.size() - 1);            // map_Ks
    }else{
        material.specularTexId = -1;
    }
    if(material_in.specular_highlight_texname.length() != 0 ){
        m_textures.push_back(Texture(m_device, material_in.specular_highlight_texname, VK_FORMAT_R8G8B8A8_SRGB));
        material.specularHighlightTexId = static_cast<uint32_t>(m_textures.size() - 1);            // map_Ns
    }else{
        material.specularHighlightTexId = -1;
    }
    if(material_in.bump_texname.length() != 0 ){
        m_textures.push_back(Texture(m_device, material_in.bump_texname, VK_FORMAT_R8G8B8A8_SRGB));
        material.bumpTexId = static_cast<uint32_t>(m_textures.size() - 1);            // map_bump, map_Bump, bump
    }else{
        material.bumpTexId = -1;
    }
    if(material_in.displacement_texname.length() != 0 ){
        m_textures.push_back(Texture(m_device, material_in.displacement_texname, VK_FORMAT_R8G8B8A8_SRGB));
        material.displacementTexId = static_cast<uint32_t>(m_textures.size() - 1);            // disp
    }else{
        material.displacementTexId = -1;
    }
    if(material_in.alpha_texname.length() != 0 ){
        m_textures.push_back(Texture(m_device, material_in.alpha_texname, VK_FORMAT_R8G8B8A8_SRGB));
        material.alphaTexId = static_cast<uint32_t>(m_textures.size() - 1);            // map_d
    }else{
        material.alphaTexId = -1;
    }
    if(material_in.reflection_texname.length() != 0 ){
        m_textures.push_back(Texture(m_device, material_in.reflection_texname, VK_FORMAT_R8G8B8A8_SRGB));
        material.reflectionTexId = static_cast<uint32_t>(m_textures.size() - 1);            // refl
    }else{
        material.reflectionTexId = -1;
    }
    m_materials.push_back(material);

    for(Sphere sphere : spheres){
        sphere.matID = materialOffset;
        m_spheres.push_back(sphere);
    }
}

uint32_t BottomLevelSphereAS::getCount(){
    return m_count;
}

void BottomLevelSphereAS::create(){

    VkTransformMatrixKHR transformMatrix = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f
    };
    uint32_t numSpheres = static_cast<uint32_t>(m_spheres.size());
    auto sphereBufferSize = numSpheres * sizeof(Sphere);
    auto transformBufferSize = sizeof(transformMatrix);

    const VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    const VkMemoryPropertyFlags memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    m_sphereBuffer = Buffer(m_device, sphereBufferSize, bufferUsageFlags, memoryPropertyFlags);
    m_sphereBuffer.map(sphereBufferSize, 0);
    m_sphereBuffer.copyTo(m_spheres.data(), sphereBufferSize);
    m_sphereBuffer.unmap();

    m_transformBuffer = Buffer(m_device, transformBufferSize, bufferUsageFlags, memoryPropertyFlags);
    m_transformBuffer.map(transformBufferSize, 0);
    m_transformBuffer.copyTo(&transformMatrix, transformBufferSize);
    m_transformBuffer.unmap();

    VkDeviceOrHostAddressConstKHR sphereDataDeviceAddress{};
    VkDeviceOrHostAddressConstKHR transformMatrixDeviceAddress{};

    sphereDataDeviceAddress.deviceAddress      = m_sphereBuffer.getDeviceAddress();
    transformMatrixDeviceAddress.deviceAddress = m_transformBuffer.getDeviceAddress();

    VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
    accelerationStructureGeometry.sType                            = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.geometryType                     = VK_GEOMETRY_TYPE_AABBS_KHR;
    accelerationStructureGeometry.flags                            = VK_GEOMETRY_OPAQUE_BIT_KHR;
    accelerationStructureGeometry.geometry.aabbs.sType             = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
    accelerationStructureGeometry.geometry.aabbs.data              = sphereDataDeviceAddress;
    accelerationStructureGeometry.geometry.aabbs.stride            = sizeof(Sphere);

    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
    accelerationStructureBuildGeometryInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationStructureBuildGeometryInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    accelerationStructureBuildGeometryInfo.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationStructureBuildGeometryInfo.geometryCount = 1;
    accelerationStructureBuildGeometryInfo.pGeometries   = &accelerationStructureGeometry;

    const uint32_t primitiveCount = 1;

    VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
    accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    vkGetAccelerationStructureBuildSizesKHR(m_device->getHandle(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &accelerationStructureBuildGeometryInfo, &numSpheres, &accelerationStructureBuildSizesInfo);

    m_accelerationStructureBuffer = Buffer(m_device, accelerationStructureBuildSizesInfo.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR);
    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
    accelerationStructureCreateInfo.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureCreateInfo.buffer = m_accelerationStructureBuffer.getHandle();
    accelerationStructureCreateInfo.size   = accelerationStructureBuildSizesInfo.accelerationStructureSize;
    accelerationStructureCreateInfo.type   = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

    vkCreateAccelerationStructureKHR(m_device->getHandle(), &accelerationStructureCreateInfo, nullptr, &m_handle);

    Buffer scratchBuffer = Buffer(m_device, accelerationStructureBuildSizesInfo.buildScratchSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
    accelerationBuildGeometryInfo.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationBuildGeometryInfo.type                      = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    accelerationBuildGeometryInfo.flags                     = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationBuildGeometryInfo.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelerationBuildGeometryInfo.dstAccelerationStructure  = m_handle;
    accelerationBuildGeometryInfo.geometryCount             = 1;
    accelerationBuildGeometryInfo.pGeometries               = &accelerationStructureGeometry;
    accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.getDeviceAddress();

    VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo;
    accelerationStructureBuildRangeInfo.primitiveCount                                           = numSpheres;
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
    accelerationDeviceAddressInfo.accelerationStructure = m_handle;

    m_deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(m_device->getHandle(), &accelerationDeviceAddressInfo);

    m_sphereBufferDescriptors.push_back(m_sphereBuffer.getDescriptorInfo(VK_WHOLE_SIZE, 0));
}

void BottomLevelSphereAS::destroy(){
    m_sphereBuffer.destroy();
    m_transformBuffer.destroy();
    m_accelerationStructureBuffer.destroy();
    vkDestroyAccelerationStructureKHR(m_device->getHandle(), m_handle, nullptr);
}

BottomLevelSphereAS::~BottomLevelSphereAS(){
}