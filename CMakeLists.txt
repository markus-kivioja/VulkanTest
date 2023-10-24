cmake_minimum_required(VERSION 3.16)
project(VulkanTest)

set(CMAKE_CXX_STANDARD 17)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

file(GLOB SOURCES
    src/*.cpp
	external/imgui/*.cpp
	include/*.h
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE 
	include/
	external/
)

SET(GLM_TEST_ENABLE OFF CACHE BOOL "GLM Build unit tests")
add_subdirectory(external/glm EXCLUDE_FROM_ALL)

target_link_libraries(${PROJECT_NAME} glfw ${GLFW_LIBRARIES} Vulkan::Vulkan glm)

file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})
file(COPY shaders DESTINATION ${CMAKE_BINARY_DIR})
file(COPY imgui.ini DESTINATION ${CMAKE_BINARY_DIR})
