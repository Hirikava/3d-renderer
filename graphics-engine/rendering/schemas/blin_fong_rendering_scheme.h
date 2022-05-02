#ifndef BLIN_FONG_RENDERING_SCHEME_INCLUDED
#define BLIN_FONG_RENDERING_SCHEME_INCLUDED

#include <vector>

#include <glm/glm.hpp>

#include <rendering/schemas/rendering_scheme.h>
#include <rendering/global_environment.h>
#include <rendering/rendering_tmp.h>
#include <unordered_map>

namespace dengine
{
	struct BlinFongLightsInfo{
		struct Info{
			int Count;
			int padding[3];
		} Info;
		glm::vec4 LightsPositions[512];
	};


	struct BlinFongInstanceData {
		glm::mat4 ModelMatrix;
	};

	struct BlinFongEnvironmentData {
		glm::vec4 CameraPosition;
		glm::mat4 ProjectionMatrix;
		glm::mat4 ViewMatrix;
	};


	struct BlinFongRenderingUnit {
		unsigned int Vao;
		unsigned long long IndeciesSize;
		unsigned int InstaciesBuffer;
		unsigned int EnvironmentBuffer;
		unsigned int LightsBuffer;
	};


	struct BlinFongSubmitInfo {
	public:
		BlinFongSubmitInfo()
		{
			InstanceDatas.reserve(16);
		}

		int DiffuseTexture{ -1 };
		int NormalTexture{ -1 };
		std::pmr::vector<BlinFongInstanceData> InstanceDatas;
	};


	class BlinFongRenderingScheme : public IRenderingScheme{
	public:
		unsigned LoadShaderProgram() override;
		static BlinFongRenderingUnit CreateRenderingUnit(const BufferedMesh& mesh);
	};


	class BlinFongRenderingSubmiter {
	public:
		void Submit(BlinFongRenderingUnit renderingUnit, Material material, glm::mat4 modelMatrix);
		void DispatchDrawCall(unsigned programId, const GlobalEnvironment& environment) const;
		void Clear();
	private:
		std::unordered_map<std::pmr::string, std::pair<BlinFongRenderingUnit, BlinFongSubmitInfo>> instancedToDraw;
	};
}

#endif

