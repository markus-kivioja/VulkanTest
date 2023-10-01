#version 450

layout(set = 0, binding = 0) uniform ModelTransforms
{
    mat4 model;
} modelTransforms;

layout(set = 1, binding = 0) uniform CameraTransforms
{
    mat4 view;
    mat4 projection;
} cameraTransform;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uvCoord;

layout(location = 0) out vec3 fragTexCoord;

void main()
{
	mat4 mv = cameraTransform.view * modelTransforms.model;
	mv[3][0] = 0.0f;
	mv[3][1] = 0.0f;
	mv[3][2] = 0.0f;
	mat4 mvp = cameraTransform.projection * mv;
    gl_Position = mvp * vec4(position, 1.0);
	gl_Position.z = 1.0f;

	fragTexCoord = position;
}