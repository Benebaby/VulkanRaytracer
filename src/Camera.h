#pragma once
#include "GlobalDefs.h"

class Camera
{
private:
	glm::mat4 m_viewMatrix;
    glm::vec3 m_eye;
	glm::vec2 m_oldPos, m_newPos;
    glm::vec2 m_angle = glm::vec2(0.0f);
    uint32_t m_width, m_height;
    GLFWwindow* m_window;
    static void MouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static float m_radius;
    static float m_xpos;
    static float m_ypos;
public:
    Camera();
	Camera(GLFWwindow* window, uint32_t width, uint32_t height, glm::vec3 eye);
    void update();
    glm::mat4 getView();
};