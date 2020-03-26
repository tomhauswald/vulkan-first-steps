#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexColor;

layout(location = 0) out vec3 fragmentColor;

layout(binding = 0) uniform Global {
	mat4 viewMatrix;
	mat4 projectionMatrix;
} globals;

layout(binding = 1) uniform PerInstance {
	mat4 modelMatrix;
} self;

void main() {

	gl_Position = globals.projectionMatrix 
		    * globals.viewMatrix 
		    * self.modelMatrix 
                    * vec4(vertexPosition, 1.0);
	
	fragmentColor = vertexColor;
}
