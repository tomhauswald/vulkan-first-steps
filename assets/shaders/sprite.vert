#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexColor;
layout(location = 2) in vec2 vertexUV;

layout(location = 0) out vec4 fragmentColor;
layout(location = 1) out vec2 fragmentUV;

#define BATCH_SIZE 292
#define VERTS_PER_SPRITE 6

layout(set = 0, binding = 0) uniform USpriteBatch {
	vec4 bounds       [BATCH_SIZE];
	vec4 textureAreas [BATCH_SIZE];
	vec4 colors       [BATCH_SIZE];
	vec4 trigonometry [BATCH_SIZE / 2];
} batch;

void main() {

	int index = gl_VertexIndex / VERTS_PER_SPRITE;

	float sine   = batch.trigonometry[index / 2][2 * (index % 2) + 0];
	float cosine = batch.trigonometry[index / 2][2 * (index % 2) + 1];

	vec2 rot = vec2( 
		cosine * (vertexPosition.x - 0.5) 
		+ sine * (vertexPosition.y - 0.5),
		cosine * (vertexPosition.y - 0.5) 
		- sine * (vertexPosition.x - 0.5)
	);

	gl_Position = vec4(
		batch.bounds[index].xy + (rot + 0.5) * batch.bounds[index].zw,
		0.0, 1.0
	);

	fragmentColor = vec4(vertexColor, 1.0) * batch.colors[index];
	fragmentUV = batch.textureAreas[index].xy + vertexUV * batch.textureAreas[index].zw;
}
