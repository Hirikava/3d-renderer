#version 460

layout (location = 0) in vec3 aPostion;
layout (location = 1) in vec2 aUV;
layout (location = 2) in mat4 aModelMatrix; //Instanced


layout (binding = 0) uniform GlobalEnv
{
	vec4 uCameraPostion;
	mat4 uProjectionMatrix;
	mat4 uViewMatrix;
};


out VS_OUT {
	vec2 UV;
} vsOut;


void main()
{
	gl_Position = uProjectionMatrix * uViewMatrix * aModelMatrix * vec4(aPostion, 1.0f);
	vsOut.UV = aUV;
}


