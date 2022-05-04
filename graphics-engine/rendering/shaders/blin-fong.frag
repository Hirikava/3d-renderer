#version 460

in VS_OUT {
	vec3 normal;
	vec2 uv;
	vec3 cameraPos;
	vec3 fragPos;
	mat3 TBN;
} fsIn;

layout (binding = 0) uniform sampler2D sDiffuseMap;
layout (binding = 1) uniform sampler2D sNormalMap;
layout (binding = 2) uniform sampler2D sMetalnessMap;

struct LightSettings
{	
	float AmbientStrength;
	float DiffuseStrength;
	float SpecularStrength;
	int SpecularPower;
};


layout (std140, binding = 1) uniform LightsInfo
{
	LightSettings settings;
} lightsInfo;


struct LightInfo{
	vec4 Position;	
	vec4 Color;	
};

layout (binding = 0) buffer LightsEnvironment
{
	int lightsCount;
	LightInfo lights[];
};

out vec4 outputColor;

void main()
{
	//ambient component
    vec3 norm = texture(sNormalMap, fsIn.uv).rgb;
    norm = normalize(norm * 2.0 - 1.0);   
   
    vec3 viewDir  = fsIn.TBN * normalize(fsIn.cameraPos - fsIn.fragPos);  

	vec3 tmpDiffuseImpact = vec3(0,0,0);
	vec3 tmpSpecularImpact = vec3(0,0,0);
	for (int i = 0; i < lightsCount; i++)
	{
		LightInfo lightInfo = lights[i];
		vec3 lightDir = fsIn.TBN * normalize(lightInfo.Position.xyz - fsIn.fragPos);
		//add duffisue component
		float diffuseImpact = max(dot(norm, lightDir), 0);
		tmpDiffuseImpact += diffuseImpact * lightInfo.Color.xyz;
		//add specular component
		vec3 reflectDir = reflect(-lightDir, norm);
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), lightsInfo.settings.SpecularPower);
		tmpSpecularImpact = spec * lightInfo.Color.xyz * texture(sMetalnessMap, fsIn.uv).b;
	}

	vec4 baseColor = texture(sDiffuseMap, fsIn.uv);
	vec4 resultAmbientImpact = vec4(lightsInfo.settings.AmbientStrength * vec3(1,1,1), 1);
	vec4 resultDiffuseImpact = vec4(lightsInfo.settings.DiffuseStrength * tmpDiffuseImpact, 1);
	vec4 resultSpecularImpact = vec4(lightsInfo.settings.SpecularStrength * tmpSpecularImpact, 1);
	outputColor = (resultAmbientImpact + resultDiffuseImpact + resultSpecularImpact) * baseColor;
}