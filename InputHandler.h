#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <glm/glm.hpp>

#include <cstdint>

class InputHandler
{
public:
	InputHandler(GLFWwindow* window);

	void update();

	glm::vec2 getDragVelocity();
private:
	void keyPressed(int key, int action);
	void mouseClicked(int button, int action);
	void mouseMoved(double xPos, double yPos);

	glm::vec2 m_prevMousePos;
	glm::vec2 m_currentMousePos;
	glm::vec2 m_dragVelocity;
	bool m_leftMouseDown{ false };
	bool m_rightMouseDown{ false };

	GLFWkeyfun m_prevKeyfun{ nullptr };
	GLFWcursorposfun m_prevCursorposfun{ nullptr };
	GLFWmousebuttonfun m_prevMousebuttonfun{ nullptr };
};

