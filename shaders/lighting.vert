#version 450

layout(location = 0) out vec2 uvCoord;

vec2 positions[4] = vec2[]
(
    vec2(1.0, -1.0),
    vec2(1.0, 1.0),
    vec2(-1.0, -1.0),
    vec2(-1.0, 1.0)
);

vec2 uvCoords[4] = vec2[]
(
    vec2(1.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 0.0),
    vec2(0.0, 1.0)
);

void main()
{
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    uvCoord = uvCoords[gl_VertexIndex];
}