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

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;

void main()
{
	mat4 mvMatrix = cameraTransform.view * modelTransforms.model;
    gl_Position = cameraTransform.projection * mvMatrix * vec4(position, 1.0);
	
	mat3 normMatrix = transpose(inverse(mat3(mvMatrix)));
	fragNormal = normalize(normMatrix * normal);
    
	fragTexCoord = uvCoord;
}