#version 460

//textures
layout (binding = 0) uniform sampler2D diffuseMap;

//in data
in VS_OUT {
	vec2 UV;
	flat int baseColorSelectorIndex;
	vec4 baseColor;
} fsIn;

//subroutines
subroutine vec4 BaseColorSelector();
layout (index = 0) subroutine(BaseColorSelector) vec4 selectColorFromTexture()
{
	return texture(diffuseMap, fsIn.UV);
}
layout(index = 1) subroutine(BaseColorSelector) vec4 selectColorFromMaterial()
{
	return vec4(fsIn.baseColor.xyz,1.0f);
}
layout (location = 0) subroutine uniform BaseColorSelector BaseColorSelectors[2];


out vec4 fragColor;

void main()
{
	vec4 baseColor = BaseColorSelectors[fsIn.baseColorSelectorIndex]();
	fragColor = baseColor;
}