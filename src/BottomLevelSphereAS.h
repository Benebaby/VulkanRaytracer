#include "BottomLevelAS.h" 

class BottomLevelSphereAS : public BottomLevelAS
{
private:
    static uint32_t m_count;
    std::vector<Sphere> m_spheres;
    Buffer m_sphereBuffer;
    Buffer m_transformBuffer;
public:
    BottomLevelSphereAS(Device* device, std::string name);

    void createSpheres() override;

    void create() override;

    void destroy() override;

    VkDescriptorBufferInfo getSphereBufferDescriptor() override;

    ~BottomLevelSphereAS();
};
