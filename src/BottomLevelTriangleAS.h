#include "BottomLevelAS.h" 

class BottomLevelTriangleAS : public BottomLevelAS
{
private:
    static uint32_t m_count;
    Buffer m_vertexBuffer;
    Buffer m_indexBuffer;
    Buffer m_transformBuffer;
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    static std::vector<VkDescriptorBufferInfo> m_vertexBufferDescriptors;
    static std::vector<VkDescriptorBufferInfo> m_indexBufferDescriptors;
public:
    static VkDescriptorBufferInfo* getVertexBufferDescriptors();
    static VkDescriptorBufferInfo* getIndexBufferDescriptors();
    static uint32_t getCount();

    BottomLevelTriangleAS(Device* device, std::string name);

    void uploadData(std::string path);
    void uploadData(std::string path, tinyobj::material_t &material_in);

    void create() override;

    void destroy() override;

    ~BottomLevelTriangleAS();
};
