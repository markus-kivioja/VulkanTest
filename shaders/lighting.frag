#version 450

layout(set = 0, binding = 0) uniform Transforms
{
    mat4 projInverse;
    mat4 viewInverse;
	vec3 lightDir;
} transforms;

layout(set = 1, binding = 0) uniform LightTransforms
{
    mat4 view;
    mat4 projection;
} lightTransform;

layout(set = 0, binding = 1) uniform sampler2D albedoSampler;
layout(set = 0, binding = 2) uniform sampler2D normalSampler;
layout(set = 0, binding = 3) uniform sampler2D depthSampler;
layout(set = 0, binding = 4) uniform sampler2D shadowSampler;

layout(location = 0) in vec2 uvCoord;

layout(location = 0) out vec4 outColor;

void main()
{
	vec4 ndc = vec4(uvCoord * 2.0f - 1.0f, texture(depthSampler, uvCoord).r, 1);
	vec4 temp = transforms.projInverse * ndc;
	temp /= temp.w;
	vec3 viewSpacePos = temp.xyz;
	vec4 worldSpacePos = transforms.viewInverse * temp;
	
	vec3 L = -transforms.lightDir;
	vec3 N = normalize(texture(normalSampler, uvCoord).xyz);
	vec3 V = normalize(-viewSpacePos);
	vec3 H = normalize(L + V);
	
	// Shadows
	float shadow = 1.0f;
	vec4 lightNDC = lightTransform.projection * lightTransform.view * worldSpacePos;
	lightNDC /= lightNDC.w;
	vec2 shadowMapUV = lightNDC.xy * 0.5 + 0.5;
	const float SHADOW_BIAS = 0.001f;
	if (lightNDC.z > texture(shadowSampler, shadowMapUV).r + SHADOW_BIAS)
	{
		shadow = 0.4f;
	}
	
	const float ambient = 0.4f;
	float diffuse = max(dot(N, L), 0.0);
	float specular = 0;
	if (diffuse > 0 && shadow == 1.0f)
	{
		specular = pow(max(dot(N, H), 0.0), 16.0f);
	}

    outColor = texture(albedoSampler, uvCoord) * (ambient + diffuse) * shadow + specular;
}