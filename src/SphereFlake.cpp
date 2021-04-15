#include "SphereFlake.h"

SphereFlake::SphereFlake(){
    float radius = 0.5f;
}

void SphereFlake::generateSphereFlake(int recursionDepth, float radius){
    generateSphereFlake(recursionDepth, glm::mat4(1.f), radius);
    std::cout<<"Sphereflake Size: "<<m_spheres.size()<<std::endl;
}

void SphereFlake::generateSphereFlake(int recursionDepth,  glm::mat4 ModelMatrix, float radius){
    glm::vec4 tempCenter = ModelMatrix * glm::vec4(0.f, 0.f, 0.f, 1.f);
    Sphere sphere;
    sphere.matID = 0;
    sphere.center[0] = tempCenter.x; 
    sphere.center[1] = tempCenter.y; 
    sphere.center[2] = tempCenter.z;
    sphere.radius = radius;
    sphere.aabbmin[0] = sphere.center[0] - sphere.radius; 
    sphere.aabbmin[1] = sphere.center[1] - sphere.radius; 
    sphere.aabbmin[2] = sphere.center[2] - sphere.radius;
    sphere.aabbmax[0] = sphere.center[0] + sphere.radius; 
    sphere.aabbmax[1] = sphere.center[1] + sphere.radius; 
    sphere.aabbmax[2] = sphere.center[2] + sphere.radius;

    m_spheres.push_back(sphere);
	
	if (recursionDepth-- > 0)
	{
		float newRadius = radius / 3.f;
		
		glm::vec3 xVec = glm::vec3(glm::column( ModelMatrix, 0));
		glm::vec3 yVec = glm::vec3(glm::column( ModelMatrix, 1));
		glm::vec3 zVec = glm::vec3(glm::column( ModelMatrix, 2));
		glm::vec3 center = glm::vec3(glm::column( ModelMatrix, 3));

		float rho = 2.f * glm::pi<float>() / 6.f;
		
		for (int i = 0; i < 6; i++)
		{			
			float phi = i * rho;
			
			glm::vec3 newy = ((float)cos(phi)) * xVec + ((float)sin(phi)) * zVec;
			glm::vec3 newx = glm::cross( newy, yVec);
			glm::vec3 newz = glm::cross( newx, newy);
			glm::vec3 newcenter = center + newy * (radius + newRadius);

			glm::mat4 newmat = glm::mat4( glm::vec4( newx, 0), glm::vec4( newy, 0), glm::vec4( newz, 0), glm::vec4( newcenter, 1.f));

			generateSphereFlake(recursionDepth, newmat, newRadius);
		}	
		
		for (int i = 0; i < 3; i++)
		{
			float phi = (i + 0.5f) * 2.f * rho;
			
			glm::vec3 tmpy = ((float)cos(phi)) * xVec + ((float)sin(phi)) * zVec;
			glm::vec3 newy = ((float)cos(rho)) * tmpy + ((float)sin(rho)) * yVec;
			glm::vec3 newx = glm::cross( newy, yVec);
			newx = glm::normalize( newx);
			glm::vec3 newz = glm::cross( newx, newy);
			glm::vec3 newcenter = center + newy * (radius + newRadius);

			glm::mat4 newmat = glm::mat4( glm::vec4( newx, 0), glm::vec4( newy, 0), glm::vec4( newz, 0), glm::vec4( newcenter, 1.f));
			
			generateSphereFlake(recursionDepth, newmat, newRadius);
		}	
	}	

}


std::vector<Sphere> SphereFlake::getSpheres(){
    return m_spheres;
}

SphereFlake::~SphereFlake(){

}