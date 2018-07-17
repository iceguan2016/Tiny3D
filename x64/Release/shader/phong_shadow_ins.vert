#version 330

uniform mat4 viewProjectMatrix;

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 texcoord;
layout (location = 2) in mat3x4 modelMatrix;

out vec3 vTexcoord;
out vec2 projDepth;

mat4 convertMat(mat3x4 srcMat) {
	mat4x3 transMat = transpose(srcMat);
	return mat4(transMat[0], 0.0, 
				transMat[1], 0.0, 
				transMat[2], 0.0, 
				transMat[3], 1.0);
}

void main() {
	vTexcoord = texcoord;
	vec4 worldVertex = convertMat(modelMatrix) * vec4(vertex, 1.0);
	gl_Position = viewProjectMatrix * worldVertex;
	projDepth = gl_Position.zw;
}