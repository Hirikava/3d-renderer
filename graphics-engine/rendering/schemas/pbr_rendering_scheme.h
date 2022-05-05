#ifndef PBR_RENDERING_SCHEME_INCLUDED
#define PBR_RENDERING_SCHEME_INCLUDED


#include <vector>
#include <glm/glm.hpp>

#include <rendering/schemas/rendering_scheme.h>
#include <rendering/global_environment.h>
#include <rendering/rendering_tmp.h>
#include <unordered_map>

namespace dengine
{
	struct PbrLightsInfo {
		struct Info {
			int Count;
			int padding[3];
		} Info;
		LightInfo LightsInfos[512];
	};


	struct PbrInstacesData {
		glm::mat4 ModelMatrix;
	};

	struct PbrEnvironmentData {
		glm::vec4 CameraPosition;
		glm::mat4 ProjectionMatrix;
		glm::mat4 ViewMatrix;
	};


	struct PbrRenderingUnit {
		unsigned int Vao;
		unsigned long long IndeciesSize;
		unsigned int InstaciesBuffer;
		unsigned int EnvironmentBuffer;
		unsigned int LightsBuffer;
	};


	struct PbrSubmitInfo {
	public:
		PbrSubmitInfo()
		{
			InstanceDatas.reserve(16);
		}

		int DiffuseTexture{ -1 };
		int NormalTexture{ -1 };
		int MetalnessTexture{ -1 };
		std::pmr::vector<PbrInstacesData> InstanceDatas;
	};


	class PbrRenderingScheme : public IRenderingScheme {
	public:
		unsigned LoadShaderProgram() override;
		static PbrRenderingUnit CreateRenderingUnit(const BufferedMesh& mesh, OpenglSettings openglSettings);
	};


	class BlinFongRenderingSubmiter {
	public:
		explicit BlinFongRenderingSubmiter(OpenglSettings openglSettings);
		void Submit(PbrRenderingUnit renderingUnit, Material material, glm::mat4 modelMatrix);
		void DispatchDrawCall(unsigned programId, const GlobalEnvironment& environment) const;
		void Clear();
	private:
		std::unordered_map<std::pmr::string, std::pair<PbrRenderingUnit, PbrSubmitInfo>> instancedToDraw;
		OpenglSettings openglSettings;
	};
	
}

#endif

