#version 460

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;
layout (location = 3) in vec3 aTangent;
//instanced
layout (location = 4) in mat4 aModel;

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
	mat3 TBN;
} vsOut;


void main()
{
	gl_Position = uProjectionMatrix * uViewMatrix * aModel * vec4(aPosition, 1.0f);

	vec3 bitangent = normalize(cross(aNormal, aTangent));
	
	vec3 T = normalize(vec3(aModel * vec4(aTangent,   0.0)));
	vec3 B = normalize(vec3(aModel * vec4(bitangent, 0.0)));
	vec3 N = normalize(vec3(aModel * vec4(aNormal,    0.0)));
	
	vsOut.TBN = mat3(T, B, N);
	vsOut.normal = aNormal;
	vsOut.uv = aUV;
	vsOut.cameraPos = uCameraPostion.xyz;
	vsOut.fragPos = aPosition;
}