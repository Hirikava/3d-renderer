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
}

#endif
