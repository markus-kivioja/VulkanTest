#include "InputHandler.h"

#include <iostream>

InputHandler::InputHandler(GLFWwindow* window)
{
    auto keyCallback = [](GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        auto thisHandler = (InputHandler*)glfwGetWindowUserPointer(window);
        thisHandler->keyPressed(key, action);
        thisHandler->m_prevKeyfun(window, key, scancode, action, mods);
    };

    auto mouseCallback = [](GLFWwindow* window, double xPos, double yPos)
    {
        auto thisHandler = (InputHandler*)glfwGetWindowUserPointer(window);
        thisHandler->mouseMoved(xPos, yPos);
        thisHandler->m_prevCursorposfun(window, xPos, yPos);
    };

    auto mouseButtonCallback = [](GLFWwindow* window, int button, int action, int mods)
    {
        auto thisHandler = (InputHandler*)glfwGetWindowUserPointer(window);
        thisHandler->mouseClicked(button, action);
        thisHandler->m_prevMousebuttonfun(window, button, action, mods);
    };

    glfwSetWindowUserPointer(window, this);
    
    m_prevKeyfun = glfwSetKeyCallback(window, keyCallback);
    m_prevCursorposfun = glfwSetCursorPosCallback(window, mouseCallback);
    m_prevMousebuttonfun = glfwSetMouseButtonCallback(window, mouseButtonCallback);
}

void InputHandler::update()
{
    glfwPollEvents();

    if (m_leftMouseDown)
    {
        m_dragVelocity = m_currentMousePos - m_prevMousePos;
    }
    else
    {
        m_dragVelocity = glm::vec2(0);
    }
    m_prevMousePos = m_currentMousePos;
}

glm::vec2 InputHandler::getDragVelocity()
{
    return m_dragVelocity;
}

void InputHandler::keyPressed(int key, int action)
{
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
        case GLFW_KEY_W:
            std::cout << "Forward!" << std::endl;
            break;
        case GLFW_KEY_S:
            std::cout << "Forward!" << std::endl;
            break;
        case GLFW_KEY_A:
            std::cout << "Forward!" << std::endl;
            break;
        case GLFW_KEY_D:
            std::cout << "Forward!" << std::endl;
            break;
        default:
            break;
        }
    }
}

void InputHandler::mouseClicked(int button, int action)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            m_leftMouseDown = true;
        }
        else if (action == GLFW_RELEASE)
        {
            m_leftMouseDown = false;
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        if (action == GLFW_PRESS)
        {
            m_rightMouseDown = true;
        }
        else if (action == GLFW_RELEASE)
        {
            m_rightMouseDown = false;
        }
    }
}

void InputHandler::mouseMoved(double xPos, double yPos)
{
    m_currentMousePos = glm::vec2(xPos, yPos);
}