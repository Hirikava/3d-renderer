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

struct Material
{	
	int colorSelectorIndex;
	vec4 baseColor;
};

layout (binding = 1) buffer materials {
	Material bMaterials[]; 
};


out VS_OUT {
	vec2 UV;
	flat int baseColorSelectorIndex;
	vec4 baseColor;
} vsOut;


void main()
{
	Material mat = bMaterials[gl_InstanceID];
	gl_Position = uProjectionMatrix * uViewMatrix * aModelMatrix * vec4(aPostion, 1.0f);
	
	vsOut.UV = aUV;
	vsOut.baseColorSelectorIndex = bMaterials[gl_InstanceID].colorSelectorIndex;
	vsOut.baseColor = bMaterials[gl_InstanceID].baseColor;
}


