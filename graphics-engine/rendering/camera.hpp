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
		
		Camera& operator=(const Camera& other)
		{
			this->Position = other.Position;
			this->Diraction = other.Diraction;
			this->Up = other.Up;
		};
	};

	enum MoveDiraction
	{
		Forward,
		Backwards,
		Left,
		Right,
	};




	class CameraControl{
	private:
		static glm::vec3 GetNormalizedMoveVec(const Camera& cam, MoveDiraction diraction)
		{
			switch (diraction)
			{
			case Forward: return glm::normalize(cam.Diraction);
			case Backwards: return -glm::normalize(cam.Diraction);
			case Right: return glm::normalize(glm::cross(cam.Diraction, cam.Up));
			case Left: return -glm::normalize(glm::cross(cam.Diraction, cam.Up));
			default: return glm::vec3(0,0,0);
			}
		}
	
	public:
		static glm::mat4 GetLookAtMatrix(const Camera& cam)
		{
			return lookAt(cam.Position, cam.Position + cam.Diraction, cam.Up);
		}

		static void MoveCamera(Camera& cam, float distance, MoveDiraction diraction)
		{
			glm::vec3 normalizedMoveDiraction = GetNormalizedMoveVec(cam, diraction);
			cam.Position = cam.Position + normalizedMoveDiraction * distance;
		}
	};
}


#endif
