#include "Camera.h"

float Camera::m_radius;
float Camera::m_xpos;
float Camera::m_ypos;

Camera::Camera(){

}

Camera::Camera(GLFWwindow* window, uint32_t width, uint32_t height, glm::vec3 eye)
{
    Camera::m_xpos = 0.0f;
    Camera::m_ypos = 0.0f;
    Camera::m_radius = 5.0f;
    m_angle = glm::vec2(0.5f * 3.141592f, 0.4f * 3.141592f);
    m_width = width;
    m_height = height;
    m_oldPos = glm::vec2(width / 2.0f, height / 2.0f);
    m_newPos = glm::vec2(width / 2.0f, height / 2.0f);
    m_eye = eye;
    m_window = window;
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
    if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		m_newPos = glm::vec2(Camera::m_xpos, Camera::m_ypos);
		float xAngle = (m_newPos.x - m_oldPos.x) / m_width * 2 * 3.141592f;
		float yAngle = (m_newPos.y - m_oldPos.y) / m_height * 3.141592f;
		m_angle -= glm::vec2(xAngle, yAngle);
		m_angle.y = glm::max(m_angle.y, 0.000001f);
		m_angle.y = glm::min(m_angle.y, 3.141592f);

	}
	m_oldPos = glm::vec2(Camera::m_xpos, Camera::m_ypos);

    m_eye.x = Camera::m_radius * sin(m_angle.y) * sin(m_angle.x);
	m_eye.y = Camera::m_radius * cos(m_angle.y);
	m_eye.z = Camera::m_radius * sin(m_angle.y) * cos(m_angle.x);

	m_viewMatrix = glm::lookAt(m_eye, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
}

glm::mat4 Camera::getView(){
    return glm::inverse(m_viewMatrix);
}