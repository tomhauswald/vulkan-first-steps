#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexColor;
layout(location = 2) in vec2 vertexUV;

layout(location = 0) out vec3 fragmentColor;
layout(location = 1) out vec2 fragmentUV;

#define BATCH_SIZE 1024
#define VERTS_PER_SPRITE 6

struct sprite_t {
	vec4 bounds;
	vec4 textureArea;
	vec4 color;
};

layout(set = 0, binding = 0) uniform sprite_batch_t {
	sprite_t sprites[BATCH_SIZE];
} batch;

void main() {

	int index = gl_VertexIndex / VERTS_PER_SPRITE;
	
	gl_Position = vec4(
		batch.sprites[index].bounds.xy 
		+ vertexPosition.xy * batch.sprites[index].bounds.zw,
		0.0, 
		1.0
	);

	fragmentColor = vertexColor * vec3(batch.sprites[index].color);
	
	fragmentUV = batch.sprites[index].textureArea.xy 
	           + vertexUV * batch.sprites[index].textureArea.zw;
}
