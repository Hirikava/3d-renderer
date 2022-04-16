#version 460

layout (binding = 0) uniform sampler2D diffuseMap;

in VS_OUT {
	vec2 UV;
} fsIn;

out vec4 fragColor;

void main()
{
	fragColor = texture(diffuseMap, fsIn.UV);
}