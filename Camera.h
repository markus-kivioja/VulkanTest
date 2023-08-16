#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

class Camera
{
public:
	void moveForward() { std::cout << "Forward!" << std::endl; };
	void moveBackward() { std::cout << "Backward!" << std::endl; };
	void moveLeft() { std::cout << "Left!" << std::endl; };
	void moveRight() { std::cout << "Right!" << std::endl; };
private:
	glm::vec3 m_position;
	glm::vec3 m_orieantation;
};

