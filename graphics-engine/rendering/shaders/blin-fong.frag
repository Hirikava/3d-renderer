#version 460

in VS_OUT {
	vec3 normal;
	vec2 uv;
	vec3 cameraPos;
	vec3 fragPos;
} fsIn;

layout (binding = 0) uniform sampler2D sDiffuseMap;

layout (binding = 1) uniform LightsSettings
{
	float uAmbientStrength;
	float uDiffuseStrength;
	float uSpecularStrength;
} lightsSettings;


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
	vec3 ambientImpact = lightsSettings.uAmbientStrength * vec3(1, 1, 1);
	vec3 norm = normalize(fsIn.normal);
	vec3 viewDir = normalize(fsIn.cameraPos - fsIn.fragPos);

	vec3 tmpDiffuseImpact = vec3(0,0,0);
	vec3 tmpSpecularImpact = vec3(0,0,0);
	for (int i = 0; i < lightsCount; i++)
	{
		LightInfo lightInfo = lights[i];
		vec3 lightDir = normalize(lightInfo.Position.xyz - fsIn.fragPos);
		//add duffisue component
		float diffuseImpact = max(dot(norm, lightDir), 0);
		tmpDiffuseImpact += diffuseImpact * lightInfo.Color.xyz;
		//add specular component
		vec3 reflectDir = reflect(-lightDir, norm);
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);
		tmpSpecularImpact = spec * lightInfo.Color.xyz;
	}

	vec4 baseColor = texture(sDiffuseMap, fsIn.uv);
	vec4 resultAmbientImpact = vec4(lightsSettings.uAmbientStrength * vec3(1,1,1), 1);
	vec4 resultDiffuseImpact = vec4(lightsSettings.uDiffuseStrength * tmpDiffuseImpact, 1);
	vec4 resultSpecularImpact = vec4(lightsSettings.uSpecularStrength * tmpSpecularImpact, 1);
	outputColor = (resultAmbientImpact + resultDiffuseImpact + resultSpecularImpact) * baseColor;
}