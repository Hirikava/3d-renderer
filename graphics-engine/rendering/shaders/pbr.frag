#version 460
layout (binding = 0) uniform sampler2D sAlbeidoMap;
layout (binding = 1) uniform sampler2D sNormalMap;
layout (binding = 2) uniform sampler2D sMetalnessMap;

in VS_OUT {
	vec3 normal;
	vec2 uv;
	vec3 cameraPos;
	vec3 fragPos;
	mat3 TBN;
} fsIn;

out vec4 FragmentColor;
const float PI = 3.14f;

struct LightInfo{
	vec4 Position;	
	vec4 Color;	
};

layout (binding = 0) buffer LightsEnvironment
{
	int lightsCount;
	LightInfo lights[];
};

//PBR calculation functions
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

void main()
{
    vec3 albedo = texture(sAlbeidoMap, fsIn.uv).rgb; 
    vec3 norm = texture(sNormalMap, fsIn.uv).rgb;
    norm = normalize(norm * 2.0 - 1.0);  
	vec3 metalnessCfs = texture(sMetalnessMap, fsIn.uv).rgb;
    float metallic  = metalnessCfs.b;
    float roughness = metalnessCfs.g;


    vec3 viewDir  = fsIn.TBN * normalize(fsIn.cameraPos - fsIn.fragPos); 

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

	           
    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < lightsCount; ++i) 
    {
        LightInfo lightInfo = lights[i];
        // calculate per-light radiance
        vec3 lightDir = fsIn.TBN * normalize(lightInfo.Position.xyz - fsIn.fragPos);
        vec3 halfWayVL = normalize(viewDir + lightDir);
        float distance = length(lightInfo.Position.xyz - fsIn.fragPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightInfo.Color.xyz * attenuation;        
        
        // cook-torrance brdf
        float NDF = DistributionGGX(norm, halfWayVL, roughness);        
        float G   = GeometrySmith(norm, viewDir, lightDir, roughness);      
        vec3 F    = fresnelSchlick(max(dot(halfWayVL, viewDir), 0.0), F0);       
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;	  
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(norm, viewDir), 0.0) * max(dot(norm, lightDir), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;  
            
        // add to outgoing radiance Lo
        float NdotL = max(dot(norm, lightDir), 0.0);                
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; 
    }   

    vec3 ambient = vec3(0.03) * albedo;
    vec3 color = ambient + Lo;
    FragmentColor = vec4(color, 1.0);
}