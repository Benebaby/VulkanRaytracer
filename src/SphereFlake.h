#pragma once
#include "GlobalDefs.h"
#include <glm/gtc/matrix_access.hpp>

class SphereFlake
{
private:
    std::vector<Sphere> m_spheres;
    void generateSphereFlake(int recursionDepth,  glm::mat4 ModelMatrix, float radius);
public:
    SphereFlake();
    
    void generateSphereFlake(int recursionDepth, float radius);

    std::vector<Sphere> getSpheres();

    ~SphereFlake();
};