#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 vertexPosition;
layout(location = 1) in vec3 vertexColor;

layout(location = 0) out vec3 fragmentColor;

layout(binding = 0) uniform Buffer {
	mat4 mvp;
} data;

void main() {
	gl_Position = data.mvp * vec4(vertexPosition, 0.0, 1.0);
	fragmentColor = vertexColor;
}
