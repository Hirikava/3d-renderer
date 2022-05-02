#ifndef SIMPLE_RENDERING_SCHEME_INCLUDED
#define SIMPLE_RENDERING_SCHEME_INCLUDED

#include <glm/glm.hpp>
#include <rendering/schemas/rendering_scheme.h>
#include <rendering/rendering_tmp.h>
#include <rendering/global_environment.h>
#include <unordered_map>


namespace dengine
{
	struct SimpleMaterialData{
		struct ColorSelector{
			//16 byte alignment for GPU USE 0 INDEX
			int Index;
			int padding[3];
		} ColorSelector;


		glm::vec4 BaseColor;
	};


	struct SimpleInstanceData{
		glm::mat4 ModelMatrix;
	};


	struct SimpleEnvironmentData{
		glm::mat4 ProjectionMatrix;
		glm::mat4 ViewMatrix;
	};


	struct SimpleRenderingUnit{
		unsigned int Vao;
		unsigned long long IndeciesSize;
		unsigned int InstaciesBuffer;
		unsigned int MaterialsBuffer;
		unsigned int EnvironmentBuffer;
	};


	struct SimpleSubmitInfo{
	public:
		SimpleSubmitInfo()
		{
			SimpleInstanceData.reserve(16);
			SimpleMaterialData.reserve(16);
		}

		int DiffuseTexture {-1};
		std::pmr::vector<SimpleInstanceData> SimpleInstanceData;
		std::pmr::vector<SimpleMaterialData> SimpleMaterialData;
	};


	class SimpleRenderingScheme : public IRenderingScheme{
	public:
		unsigned LoadShaderProgram() override;
		static SimpleRenderingUnit CreateRenderingUnit(const BufferedMesh& mesh);
	};





	class SimpleRedneringSubmitter{
	public:
		void Submit(SimpleRenderingUnit renderingUnit, Material material, glm::mat4 modelMatrix);
		void DispatchDrawCall(unsigned programId, GlobalEnvironment environment) const;
		void Clear();
	private:
		std::unordered_map<std::pmr::string, std::pair<SimpleRenderingUnit, SimpleSubmitInfo>> instancedToDraw;
	};
}

#endif
