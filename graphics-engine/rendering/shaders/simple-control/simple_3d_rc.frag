#version 460

//in data
in VS_OUT {
	vec2 UV;
} fsIn;

//textures
layout (binding = 0) uniform sampler2D diffuseMap;

//subroutines
subroutine vec4 BaseColorSelector(vec2 uv);
layout (index = 0) subroutine(BaseColorSelector) vec4 selectColorFromTexture(vec2 uv)
{
	return texture(diffuseMap, fsIn.UV);
}
layout (index = 1) subroutine(BaseColorSelector) vec4 selectColorFromBaseMaterial(vec2 uv)
{
	return vec4(1.0f,1.0f,0.0f,1.0f);
}

subroutine vec4 BaseColorSelector2();

layout (index = 2) subroutine(BaseColorSelector2) vec4 Red()
{
	return vec4(1,0,0,1);
}

layout(index = 3) subroutine(BaseColorSelector2) vec4 Blue()
{
	return vec4(0,1,0,1);
}


layout (location = 0) subroutine uniform BaseColorSelector SelectBaseColor[2];
layout (location = 2) subroutine uniform BaseColorSelector2 ColorFilter[2];

out vec4 fragColor;

void main()
{
	vec4 baseColor = SelectBaseColor[0](fsIn.UV);
	vec4 filter123 = ColorFilter[1](); 
	fragColor = baseColor * filter123;
}