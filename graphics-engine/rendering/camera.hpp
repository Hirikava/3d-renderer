#ifndef CAMERA_INCLUDED
#define CAMERA_INCLUDED

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

namespace dengine
{
	struct Camera{
		glm::vec3 Position{-3, 0, 0};
		glm::vec3 Diraction{1, 0, 0};
		glm::vec3 Up{0, 1, 0};
	};


	class CameraControl{
	public:
		static glm::mat4 GetLookAtMatrix(const Camera& cam)
		{
			return lookAt(cam.Position, cam.Position + cam.Diraction, cam.Up);
		}
	};
}


#endif
