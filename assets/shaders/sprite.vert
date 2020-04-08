#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexColor;
layout(location = 2) in vec2 vertexUV;

layout(location = 0) out vec3 fragmentColor;
layout(location = 1) out vec2 fragmentUV;

#define BATCH_SIZE 24
#define VERTS_PER_SPRITE 6

layout(set = 0, binding = 0) uniform sprite_batch_ {
	mat4 transforms[BATCH_SIZE];
	vec4 textureAreas[BATCH_SIZE];
	vec4 colors[BATCH_SIZE];
} batch;

void main() {

	int index = gl_VertexIndex / VERTS_PER_SPRITE;
	gl_Position = batch.transforms[index] * vec4(vertexPosition, 1.0);
	
	fragmentColor = vertexColor * vec3(batch.colors[index]);
	fragmentUV = batch.textureAreas[index].xy + vertexUV * (
		  batch.textureAreas[index].zw
		- batch.textureAreas[index].xy
	);
}
