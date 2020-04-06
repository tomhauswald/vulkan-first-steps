#version 450

layout(location = 0) in vec3 fragmentColor;
layout(location = 1) in vec2 fragmentUV;

layout(location = 0) out vec4 displayColor;

layout(binding = 1) uniform sampler2D colorMap;

void main() {
	displayColor = vec4(fragmentColor, 1.0) * texture(colorMap, fragmentUV);
}
