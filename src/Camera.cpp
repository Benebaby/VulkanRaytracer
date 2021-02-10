#include "Camera.h"

float Camera::m_radius;
float Camera::m_xpos;
float Camera::m_ypos;

Camera::Camera(){

}

Camera::Camera(Type type, GLFWwindow* window, uint32_t width, uint32_t height, glm::vec3 eye, glm::vec3 center)
{
    Camera::m_xpos = 0.0f;
    Camera::m_ypos = 0.0f;
    Camera::m_radius = 5.0f;
    m_type = type;
    m_center = center;
    m_eye = eye;
    m_forward = glm::normalize(m_center - m_eye);
    m_forward.y = 0.0f;
    m_forward = glm::normalize(m_forward);
    m_angle = glm::vec2(0.5f * 3.141592f, 0.4f * 3.141592f);
    m_width = width;
    m_height = height;
    m_oldPos = glm::vec2(width / 2.0f, height / 2.0f);
    m_newPos = glm::vec2(width / 2.0f, height / 2.0f);
    m_window = window;
    m_lastTime = glfwGetTime();
    glfwSetCursorPosCallback(m_window, &MouseCallback);
    glfwSetScrollCallback(m_window, &ScrollCallback);
    update();
}

void Camera::MouseCallback(GLFWwindow* window, double xpos, double ypos){
    Camera::m_xpos = (float) xpos;
    Camera::m_ypos = (float) ypos;
}

void Camera::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    Camera::m_radius -= (float) (yoffset * 0.1);
	Camera::m_radius = glm::max((float)Camera::m_radius, 0.000001f);
}

void Camera::update(){
    double time = glfwGetTime() - m_lastTime;
    m_lastTime = glfwGetTime();

    if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        m_newPos = glm::vec2(Camera::m_xpos, Camera::m_ypos);
        float xAngle = (m_newPos.x - m_oldPos.x) / m_width * 2 * 3.141592f;
        float yAngle = (m_newPos.y - m_oldPos.y) / m_height * 3.141592f;
        m_angle -= glm::vec2(xAngle, yAngle);
        m_angle.y = glm::max(m_angle.y, 0.000001f);
        m_angle.y = glm::min(m_angle.y, 3.141591f);
    }
    m_oldPos = glm::vec2(Camera::m_xpos, Camera::m_ypos);


    if (glfwGetKey(m_window, GLFW_KEY_C) == GLFW_PRESS && m_timeout <= 0.00001){
        if(m_type == TypeTrackBall)
            m_type = TypeFirstPerson;
        else
            m_type = TypeTrackBall;
        m_timeout = 0.3;
    }
    if(m_timeout >= 0.0)
        m_timeout -= time;

    if(m_type == TypeTrackBall){
        m_eye.x = Camera::m_radius * sin(m_angle.y) * sin(m_angle.x);
        m_eye.y = Camera::m_radius * cos(m_angle.y);
        m_eye.z = Camera::m_radius * sin(m_angle.y) * cos(m_angle.x);

        m_viewMatrix = glm::lookAt(m_eye, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
    }
    else if(m_type == TypeFirstPerson){
        glm::vec3 up = glm::vec3(0.0, 1.0, 0.0);
        glm::vec3 down = -up;
        glm::vec3 forward = glm::normalize(glm::vec3(m_forward.x, 0.0f, m_forward.z));
        glm::vec3 backwards = -1.f * forward;
        glm::vec3 left = glm::cross(up, forward);
        glm::vec3 right = -1.f * left;
        if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
            m_eye += forward * (float) time * 2.f;
        if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) 
            m_eye += left * (float) time * 2.f;
        if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
            m_eye += backwards * (float) time * 2.f;
	    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
            m_eye += right * (float) time * 2.f;
        if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            m_eye += down * (float) time * 2.f;
        if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS)
            m_eye += up * (float) time * 2.f;
        m_radius = glm::length(m_eye);

        m_forward.x = sin(3.141592f - m_angle.y) * sin(m_angle.x);
        m_forward.y = cos(3.141592f - m_angle.y);
        m_forward.z = sin(3.141592f - m_angle.y) * cos(m_angle.x);

        m_viewMatrix = glm::lookAt(m_eye, m_eye + m_forward, glm::vec3(0.0, 1.0, 0.0));
    }
}

glm::mat4 Camera::getView(){
    return glm::inverse(m_viewMatrix);
}