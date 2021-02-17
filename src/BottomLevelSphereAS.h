#include "BottomLevelAS.h" 

class BottomLevelSphereAS : public BottomLevelAS
{
private:
    static uint32_t m_count;
    std::vector<Sphere> m_spheres;
    Buffer m_sphereBuffer;
    Buffer m_transformBuffer;
    static std::vector<VkDescriptorBufferInfo> m_sphereBufferDescriptors;
public:
    static VkDescriptorBufferInfo* getSphereBufferDescriptors();

    static uint32_t getCount();

    BottomLevelSphereAS(Device* device, std::string name);

    void createSpheres();

    void create() override;

    void destroy() override;

    ~BottomLevelSphereAS();
};
