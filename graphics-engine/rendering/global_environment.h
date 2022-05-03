#ifndef GLOBAL_ENVIRONMENT_INCLUDED
#define GLOBAL_ENVIRONMENT_INCLUDED

#include <glm/glm.hpp>
#include <vector>

namespace dengine
{

	struct LightInfo{
		glm::vec4 Position;
		glm::vec4 Color;
	};

	struct GlobalEnvironment{
		glm::vec4 CameraPostion;
		glm::mat4 ProjectionMatrix;
		glm::mat4 ViewMatrix;
		std::pmr::vector<LightInfo> Lights;
		float AmbientStrength;
		float DiffuseStrength;
		float SpecularStrength;
	};
}

#endif
