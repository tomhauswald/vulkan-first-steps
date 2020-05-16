#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexColor;
layout(location = 2) in vec2 vertexUV;

layout(location = 0) out vec4 fragmentColor;
layout(location = 1) out vec2 fragmentUV;

layout(set = 0, binding = 0) uniform UniformData {
	mat4 cameraTransform;
} globals;

layout(push_constant) uniform PushConstantData {
	mat4 modelMatrix;
} self;

void main() {

	gl_Position = globals.cameraTransform 
			* self.modelMatrix 
	    		* vec4(vertexPosition, 1.0);
	
	fragmentColor = vec4(vertexColor, 1.0);
	fragmentUV = vertexUV;
}
