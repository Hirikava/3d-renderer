#version 460

layout (binding = 0) uniform sampler2D diffuseMap;

layout (binding = 1) uniform RenderControl
{
	bool unUseAlbeidoTexture;
	vec4 unAlbeidoColor;
};

in VS_OUT {
	vec2 UV;
} fsIn;

out vec4 fragColor;

vec4 getColor()
{	
	if(unUseAlbeidoTexture)
       return texture(diffuseMap, fsIn.UV);
	return unAlbeidoColor;
}

void main()
{
	fragColor = getColor();
}