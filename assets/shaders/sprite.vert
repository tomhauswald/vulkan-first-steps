#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexColor;
layout(location = 2) in vec2 vertexUV;

layout(location = 0) out vec3 fragmentColor;
layout(location = 1) out vec2 fragmentUV;

#define BATCH_SIZE 312
#define VERTS_PER_SPRITE 6

layout(set = 0, binding = 0) uniform USpriteBatch {
	vec4  bounds       [BATCH_SIZE];
	vec4  textureAreas [BATCH_SIZE];
	vec4  colors       [BATCH_SIZE];
	vec4  rotations    [BATCH_SIZE / 4];
} batch;

void main() {

	int index = gl_VertexIndex / VERTS_PER_SPRITE;

	float rad = batch.rotations[index / 4][index % 4] / 180.0 * 3.1416;
	float cosrad = cos(rad);
	float sinrad = sin(rad);

	vec2 pos = batch.bounds[index].xy + vertexPosition.xy * batch.bounds[index].zw;
	vec2 mid = batch.bounds[index].xy + 0.5 * batch.bounds[index].zw;

	gl_Position = vec4(
		mid.x + cosrad * (pos.x - mid.x) + sinrad * (pos.y - mid.y),
		mid.y + cosrad * (pos.y - mid.y) - sinrad * (pos.x - mid.x), 
		0.0, 1.0
	);

	fragmentColor = vertexColor * vec3(batch.colors[index]);
	fragmentUV = batch.textureAreas[index].xy + vertexUV * batch.textureAreas[index].zw;
}
