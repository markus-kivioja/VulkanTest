#version 450

layout(binding = 0) uniform Transforms
{
    mat4 model;
    mat4 view;
    mat4 projection;
} transforms;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uvCoord;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;

void main()
{
	mat4 mvMatrix = transforms.view * transforms.model;
    gl_Position = transforms.projection * mvMatrix * vec4(position, 1.0);
	
	mat3 normMatrix = transpose(inverse(mat3(mvMatrix)));
	fragNormal = normalize(normMatrix * normal);
    
	fragTexCoord = uvCoord;
}