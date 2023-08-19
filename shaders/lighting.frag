#version 450

layout(binding = 0) uniform Transforms
{
    mat4 projInverse;
} transforms;

layout(binding = 1) uniform sampler2D albedoSampler;
layout(binding = 2) uniform sampler2D normalSampler;
layout(binding = 3) uniform sampler2D depthSampler;

layout(location = 0) in vec2 uvCoord;

layout(location = 0) out vec4 outColor;

void main()
{
	vec4 ndc = vec4(uvCoord * 2.0f - 1.0f, texture(depthSampler, uvCoord).r, 1);
	vec4 temp = transforms.projInverse * ndc;
	vec3 viewSpacePos = temp.xyz / temp.w;
	
	const vec3 L = normalize(vec3(0.4, 0.3, 1));
	vec3 N = normalize(texture(normalSampler, uvCoord).xyz);
	vec3 V = normalize(-viewSpacePos);
	vec3 H = normalize(L + V);
	
	const float ambient = 0.4f;
	float diffuse = max(dot(N, L), 0.0);
	float specular = 0;
	if (diffuse > 0)
	{
		specular = pow(max(dot(N, H), 0.0), 16.0f);
	}

    outColor = texture(albedoSampler, uvCoord) * (ambient + diffuse) + specular;
}