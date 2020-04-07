#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragmentColor;
layout(location = 1) in vec2 fragmentUV;

layout(location = 0) out vec4 displayColor;

layout(set = 1, binding = 0) uniform sampler2D lhs;
layout(set = 2, binding = 0) uniform sampler2D rhs;

void main() {
	displayColor = vec4(fragmentColor, 1.0) 
	             * texture(lhs, fragmentUV) 
				 * texture(rhs, fragmentUV);
}
