#ifndef GLOBAL_ENVIRONMENT_INCLUDED
#define GLOBAL_ENVIRONMENT_INCLUDED

#include <glm/glm.hpp>
#include <vector>

namespace dengine
{
	struct GlobalEnvironment{
		glm::vec4 CameraPostion;
		glm::mat4 ProjectionMatrix;
		glm::mat4 ViewMatrix;
		std::pmr::vector<glm::vec4> LightsPositions;
	};
}

#endif
