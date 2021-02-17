#include "BottomLevelTriangleAS.h"

uint32_t BottomLevelTriangleAS::m_count = 0;
std::vector<VkDescriptorBufferInfo> BottomLevelTriangleAS::m_vertexBufferDescriptors;
std::vector<VkDescriptorBufferInfo> BottomLevelTriangleAS::m_indexBufferDescriptors;

BottomLevelTriangleAS::BottomLevelTriangleAS(Device* device, std::string name) : BottomLevelAS(device, name, m_count){
    m_count++;
}

void BottomLevelTriangleAS::uploadData(std::string path){
    std::string modelname = path.substr(1, path.size()-1);
    modelname = modelname.substr(0, modelname.find_first_of("/\\"));
    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = "";
    reader_config.triangulate = true;

    uint32_t materialOffset = static_cast<uint32_t>(m_materials.size());

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile((MODEL_PATH + path), reader_config)) {
    if (!reader.Error().empty()) {
        std::cerr << "TinyObjReader: " << reader.Error();
    }
    exit(1);
    }

    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader: " << reader.Warning();
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& objMaterials = reader.GetMaterials();

    std::cout << "Found Materials: " << objMaterials.size() << std::endl;

    for(tinyobj::material_t objMaterial : objMaterials){
        Material material{};
        material.ambient[0] = objMaterial.ambient[0];               material.ambient[1] = objMaterial.ambient[1];               material.ambient[2] = objMaterial.ambient[2];
        material.diffuse[0] = objMaterial.diffuse[0];               material.diffuse[1] = objMaterial.diffuse[1];               material.diffuse[2] = objMaterial.diffuse[2];
        material.specular[0] = objMaterial.specular[0];             material.specular[1] = objMaterial.specular[1];             material.specular[2] = objMaterial.specular[2];
        material.transmittance[0] = objMaterial.transmittance[0];   material.transmittance[1] = objMaterial.transmittance[1];   material.transmittance[2] = objMaterial.transmittance[2];
        material.emission[0] = objMaterial.emission[0];             material.emission[1] = objMaterial.emission[1];             material.emission[2] = objMaterial.emission[2];
        material.shininess = objMaterial.shininess;
        material.ior = objMaterial.ior;                     // index of refraction
        material.dissolve = objMaterial.dissolve;                // 1 == opaque; 0 == fully transparent
        material.illum = objMaterial.illum;                   // Beleuchtungsmodell
        if(objMaterial.ambient_texname.length() != 0 ){
            std::string base_filename = objMaterial.ambient_texname.substr(objMaterial.ambient_texname.find_last_of("/\\") + 1);
            std::string texturepath = "/"+modelname+"/"+base_filename;
            m_textures.push_back(Texture(m_device, texturepath, VK_FORMAT_R8G8B8A8_SRGB));
            material.ambientTexId = static_cast<uint32_t>(m_textures.size() - 1);            // map_Ka
        }else{
            material.ambientTexId = -1;
        }
        if(objMaterial.diffuse_texname.length() != 0 ){
            std::string base_filename = objMaterial.diffuse_texname.substr(objMaterial.diffuse_texname.find_last_of("/\\") + 1);
            m_textures.push_back(Texture(m_device, "/"+modelname+"/"+base_filename, VK_FORMAT_R8G8B8A8_SRGB));
            material.diffuseTexId = static_cast<uint32_t>(m_textures.size() - 1);            // map_Kd
        }else{
            material.diffuseTexId = -1;
        }
        if(objMaterial.specular_texname.length() != 0 ){
            std::string base_filename = objMaterial.specular_texname.substr(objMaterial.specular_texname.find_last_of("/\\") + 1);
            m_textures.push_back(Texture(m_device, "/"+modelname+"/"+base_filename, VK_FORMAT_R8G8B8A8_SRGB));
            material.specularTexId = static_cast<uint32_t>(m_textures.size() - 1);            // map_Ks
        }else{
            material.specularTexId = -1;
        }
        if(objMaterial.specular_highlight_texname.length() != 0 ){
            std::string base_filename = objMaterial.specular_highlight_texname.substr(objMaterial.specular_highlight_texname.find_last_of("/\\") + 1);
            m_textures.push_back(Texture(m_device, "/"+modelname+"/"+base_filename, VK_FORMAT_R8G8B8A8_SRGB));
            material.specularHighlightTexId = static_cast<uint32_t>(m_textures.size() - 1);            // map_Ns
        }else{
            material.specularHighlightTexId = -1;
        }
        if(objMaterial.bump_texname.length() != 0 ){
            std::string base_filename = objMaterial.bump_texname.substr(objMaterial.bump_texname.find_last_of("/\\") + 1);
            m_textures.push_back(Texture(m_device, "/"+modelname+"/"+base_filename, VK_FORMAT_R8G8B8A8_SRGB));
            material.bumpTexId = static_cast<uint32_t>(m_textures.size() - 1);            // map_bump, map_Bump, bump
        }else{
            material.bumpTexId = -1;
        }
        if(objMaterial.displacement_texname.length() != 0 ){
            std::string base_filename = objMaterial.displacement_texname.substr(objMaterial.displacement_texname.find_last_of("/\\") + 1);
            m_textures.push_back(Texture(m_device, "/"+modelname+"/"+base_filename, VK_FORMAT_R8G8B8A8_SRGB));
            material.displacementTexId = static_cast<uint32_t>(m_textures.size() - 1);            // disp
        }else{
            material.displacementTexId = -1;
        }
        if(objMaterial.alpha_texname.length() != 0 ){
            std::string base_filename = objMaterial.alpha_texname.substr(objMaterial.alpha_texname.find_last_of("/\\") + 1);
            m_textures.push_back(Texture(m_device, "/"+modelname+"/"+base_filename, VK_FORMAT_R8G8B8A8_SRGB));
            material.alphaTexId = static_cast<uint32_t>(m_textures.size() - 1);            // map_d
        }else{
            material.alphaTexId = -1;
        }
        if(objMaterial.reflection_texname.length() != 0 ){
            std::string base_filename = objMaterial.reflection_texname.substr(objMaterial.reflection_texname.find_last_of("/\\") + 1);
            m_textures.push_back(Texture(m_device, "/"+modelname+"/"+base_filename, VK_FORMAT_R8G8B8A8_SRGB));
            material.reflectionTexId = static_cast<uint32_t>(m_textures.size() - 1);            // refl
        }else{
            material.reflectionTexId = -1;
        }
        m_materials.push_back(material);
    }

    uint32_t current_index = 0; 
    bool hasNormals = attrib.normals.size() > 0;
    bool hasUVs = attrib.texcoords.size() > 0;

    for (size_t s = 0; s < shapes.size(); s++) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            int fv = shapes[s].mesh.num_face_vertices[f];
            int matID = shapes[s].mesh.material_ids[f];
            glm::vec3 normal;
            if(!hasNormals){
                tinyobj::index_t index = shapes[s].mesh.indices[index_offset];
                glm::vec3 v0 = glm::vec3(attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1],  attrib.vertices[3 * index.vertex_index + 2]);
                index = shapes[s].mesh.indices[index_offset + 1];
                glm::vec3 v1 = glm::vec3(attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1],  attrib.vertices[3 * index.vertex_index + 2]);
                index = shapes[s].mesh.indices[index_offset + 2];
                glm::vec3 v2 = glm::vec3(attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1],  attrib.vertices[3 * index.vertex_index + 2]);
                normal = glm::cross(v1 - v0, v2 - v0);
            }
            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t index = shapes[s].mesh.indices[index_offset + v];
                Vertex vertex{};
                vertex.matID = materialOffset + matID;
                vertex.position[0] = attrib.vertices[3 * index.vertex_index + 0];
                vertex.position[1] = attrib.vertices[3 * index.vertex_index + 1];
                vertex.position[2] = attrib.vertices[3 * index.vertex_index + 2];
                if(hasNormals){
                    vertex.normal[0] = attrib.normals[3 * index.normal_index + 0];
                    vertex.normal[1] = attrib.normals[3 * index.normal_index + 1];
                    vertex.normal[2] = attrib.normals[3 * index.normal_index + 2];
                }else{
                    vertex.normal[0] = normal.x;
                    vertex.normal[1] = normal.y;
                    vertex.normal[2] = normal.z;
                }
                if(hasUVs){
                    vertex.texture[0] = attrib.texcoords[2 * index.texcoord_index + 0];
                    vertex.texture[1] = 1.0f - attrib.texcoords[2 * index.texcoord_index + 1];
                }else{
                    vertex.texture[0] = 0;
                    vertex.texture[1] = 0;
                }
                m_vertices.push_back(vertex);
                m_indices.push_back(current_index);
                current_index++;
            }
            index_offset += fv;
        }
    }
}

void BottomLevelTriangleAS::create(){
    uint32_t numTriangles = static_cast<uint32_t>(m_vertices.size()) / 3;
    uint32_t maxVertex = static_cast<uint32_t>(m_vertices.size());

    VkTransformMatrixKHR transformMatrix = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f
    };

    auto vertexBufferSize = m_vertices.size() * sizeof(Vertex);
    auto indexBufferSize  = m_indices.size() * sizeof(uint32_t);
    auto transformBufferSize = sizeof(transformMatrix);

    const VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    const VkMemoryPropertyFlags memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    m_vertexBuffer = Buffer(m_device, vertexBufferSize, bufferUsageFlags, memoryPropertyFlags);
    m_vertexBuffer.map(vertexBufferSize, 0);
    m_vertexBuffer.copyTo(m_vertices.data(), vertexBufferSize);
    m_vertexBuffer.unmap();

    m_indexBuffer = Buffer(m_device, indexBufferSize, bufferUsageFlags, memoryPropertyFlags);
    m_indexBuffer.map(indexBufferSize, 0);
    m_indexBuffer.copyTo(m_indices.data(), indexBufferSize);
    m_indexBuffer.unmap();

    m_transformBuffer = Buffer(m_device, transformBufferSize, bufferUsageFlags, memoryPropertyFlags);
    m_transformBuffer.map(transformBufferSize, 0);
    m_transformBuffer.copyTo(&transformMatrix, transformBufferSize);
    m_transformBuffer.unmap();

    VkDeviceOrHostAddressConstKHR vertexDataDeviceAddress{};
    VkDeviceOrHostAddressConstKHR indexDataDeviceAddress{};
    VkDeviceOrHostAddressConstKHR transformMatrixDeviceAddress{};

    vertexDataDeviceAddress.deviceAddress      = m_vertexBuffer.getDeviceAddress();
    indexDataDeviceAddress.deviceAddress       = m_indexBuffer.getDeviceAddress();
    transformMatrixDeviceAddress.deviceAddress = m_transformBuffer.getDeviceAddress();

    VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
    accelerationStructureGeometry.sType                            = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.geometryType                     = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    accelerationStructureGeometry.flags                            = VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
    accelerationStructureGeometry.geometry.triangles.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    accelerationStructureGeometry.geometry.triangles.vertexFormat  = VK_FORMAT_R32G32B32_SFLOAT;
    accelerationStructureGeometry.geometry.triangles.vertexData    = vertexDataDeviceAddress;
    accelerationStructureGeometry.geometry.triangles.maxVertex     = maxVertex;
    accelerationStructureGeometry.geometry.triangles.vertexStride  = sizeof(Vertex);
    accelerationStructureGeometry.geometry.triangles.indexType     = VK_INDEX_TYPE_UINT32;
    accelerationStructureGeometry.geometry.triangles.indexData     = indexDataDeviceAddress;
    accelerationStructureGeometry.geometry.triangles.transformData = transformMatrixDeviceAddress;

    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
    accelerationStructureBuildGeometryInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationStructureBuildGeometryInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    accelerationStructureBuildGeometryInfo.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationStructureBuildGeometryInfo.geometryCount = 1;
    accelerationStructureBuildGeometryInfo.pGeometries   = &accelerationStructureGeometry;

    const uint32_t primitiveCount = 1;

    VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
    accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    vkGetAccelerationStructureBuildSizesKHR(m_device->getHandle(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &accelerationStructureBuildGeometryInfo, &numTriangles, &accelerationStructureBuildSizesInfo);

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
    accelerationStructureBuildRangeInfo.primitiveCount                                           = numTriangles;
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

    m_deviceAddress     = vkGetAccelerationStructureDeviceAddressKHR(m_device->getHandle(), &accelerationDeviceAddressInfo);

    m_vertexBufferDescriptors.push_back(m_vertexBuffer.getDescriptorInfo(VK_WHOLE_SIZE, 0));
    m_indexBufferDescriptors.push_back(m_indexBuffer.getDescriptorInfo(VK_WHOLE_SIZE, 0));
}

void BottomLevelTriangleAS::destroy(){
    m_indexBuffer.destroy();
    m_vertexBuffer.destroy();
    m_transformBuffer.destroy();
    m_accelerationStructureBuffer.destroy();
    vkDestroyAccelerationStructureKHR(m_device->getHandle(), m_handle, nullptr);
}

VkDescriptorBufferInfo* BottomLevelTriangleAS::getVertexBufferDescriptors(){
    return m_vertexBufferDescriptors.data();
}

VkDescriptorBufferInfo* BottomLevelTriangleAS::getIndexBufferDescriptors(){
    return m_indexBufferDescriptors.data();
}

uint32_t BottomLevelTriangleAS::getCount(){
    return m_count;
}

BottomLevelTriangleAS::~BottomLevelTriangleAS(){
}