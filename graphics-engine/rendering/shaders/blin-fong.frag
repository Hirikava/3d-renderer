#version 460

in VS_OUT {
	vec3 normal;
	vec2 uv;
	vec3 cameraPos;
	vec3 fragPos;
} fsIn;

layout (binding = 0) uniform sampler2D sDiffuseMap;


struct LightInfo{
	vec4 lightPosition;	
};

layout (binding = 0) buffer LightsEnvironment
{
	int lightsCount;
	LightInfo lights[];
};

out vec4 outputColor;

void main()
{
	vec3 ambientImpact = vec3(0.1, 0.1, 0.1);

	vec3 resultDiffuseImpact = vec3(0,0,0);
	vec3 norm = normalize(fsIn.normal);

	for (int i = 0; i < lightsCount; i++)
	{
		LightInfo lightInfo = lights[i];
		vec3 lightDir = normalize(lightInfo.lightPosition.xyz - fsIn.fragPos);
		float diffuseImpact = max(dot(norm, lightDir), 0);
		resultDiffuseImpact += vec3(diffuseImpact, diffuseImpact, diffuseImpact);
	}

	vec4 baseColor = texture(sDiffuseMap, fsIn.uv);
	outputColor = vec4((ambientImpact + resultDiffuseImpact), 1.0f) * baseColor;
}