#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 vertexPosition;
layout(location = 1) in vec3 vertexColor;

layout(location = 0) out vec3 fragmentColor;

void main() {
	gl_Position = vec4(vertexPosition, 0.0, 1.0);
	fragmentColor = vertexColor;
}
