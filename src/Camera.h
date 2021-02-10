#pragma once
#include <chrono>
#include "GlobalDefs.h"

class Camera
{
public:
    enum Type{
        TypeTrackBall = 0,
        TypeFirstPerson = 1
    };
    Camera();
	Camera(Type type, GLFWwindow* window, uint32_t width, uint32_t height, glm::vec3 eye, glm::vec3 center);
    void update();
    glm::mat4 getView();
private:
	glm::mat4 m_viewMatrix;
    glm::vec3 m_eye;
    glm::vec3 m_center;
    glm::vec3 m_forward;
	glm::vec2 m_oldPos, m_newPos;
    glm::vec2 m_angle = glm::vec2(0.0f);
    uint32_t m_width, m_height;
    double m_timeout = 0.0;
    double m_lastTime = 0.0;
    GLFWwindow* m_window;
    static void MouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static float m_radius;
    static float m_xpos;
    static float m_ypos;
    Type m_type;
};