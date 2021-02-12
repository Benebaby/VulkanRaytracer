#include "BottomLevelAS.h" 

class BottomLevelTriangleAS : public BottomLevelAS
{
private:
    static uint32_t m_count;
    Buffer m_vertexBuffer;
    Buffer m_indexBuffer;
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    Buffer m_transformBuffer;
public:
    BottomLevelTriangleAS(Device* device, std::string name);

    void uploadData(std::string path) override;

    VkDescriptorBufferInfo getVertexBufferDescriptor() override;

    VkDescriptorBufferInfo getIndexBufferDescriptor() override;

    void create() override;

    void destroy() override;

    ~BottomLevelTriangleAS();
};
