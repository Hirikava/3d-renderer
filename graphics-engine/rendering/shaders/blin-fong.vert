#version 460

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;
//instanced
layout (location = 3) in mat4 aModel;

layout (binding = 0) uniform GlobalEnv
{
	vec4 uCameraPostion;
	mat4 uProjectionMatrix;
	mat4 uViewMatrix;
};

out VS_OUT {
	vec3 normal;
	vec2 uv;
	vec3 cameraPos;
	vec3 fragPos;
} vsOut;


void main()
{
	gl_Position = uProjectionMatrix * uViewMatrix * aModel * vec4(aPosition, 1.0f);
	vsOut.normal = aNormal;
	vsOut.uv = aUV;
	vsOut.cameraPos = uCameraPostion.xyz;
	vsOut.fragPos = aPosition;
}