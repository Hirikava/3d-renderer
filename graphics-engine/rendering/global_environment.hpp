#ifndef GLOBAL_ENVIRONMENT_INCLUDED
#define GLOBAL_ENVIRONMENT_INCLUDED

#include <glm/glm.hpp>

namespace dengine
{
	struct GlobalEnvironment{
		glm::vec4 CameraPostion;
		glm::mat4 ProjectionMatrix;
		glm::mat4 ViewMatrix;
	};

	struct SimpleMaterial {
		int colorSelectorIndex;
		int pads[3];
		glm::vec4 baseColor;
	};

	struct RenderingUnit {
		unsigned int Vao;
		unsigned int DiffuseTexture;
		unsigned long long ElementsCount;
	};
}

#endif
