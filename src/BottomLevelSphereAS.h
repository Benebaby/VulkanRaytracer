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

    void createSphere(Sphere &sphere, tinyobj::material_t &material_in);

    void createSpheres(std::vector<Sphere> &spheres, tinyobj::material_t &material_in);

    void create() override;

    void destroy() override;

    ~BottomLevelSphereAS();
};
