#ifndef RENDERING_TMP_INCLUDED
#define RENDERING_TMP_INCLUDED

#include <map>
#include <array>
#include <importers/model_importer.h>


namespace dengine
{
	enum VertexDataType{
		Positions = 0,
		Normals = 1,
		UVs = 2,
		Tangents = 3,
	};

	struct VertexLayout {
		unsigned int Stride {0};
		unsigned int Offset {0};
		unsigned long long Size {0};
	};

	struct LoadedMaterial{
		int DiffuseTextureId;
		int NormalTextureId;
	};

	struct OpenglSettings{
		int uniformAlignment;
	};


	class BufferedMesh {
	public:
		BufferedMesh(unsigned Vbo, unsigned Ebo, unsigned MaterialIndex, unsigned long long numElemtns, const std::array<VertexLayout, 4>& vertexLayouts) :
			Vbo(Vbo), Ebo(Ebo), MaterialIndex(MaterialIndex), vertexLayouts(vertexLayouts), NumElements(numElemtns)
		{}

		unsigned int Vbo;
		unsigned int Ebo;
		unsigned int MaterialIndex;
		unsigned long long NumElements;

		VertexLayout GetVertexAttributeLayout(VertexDataType vertexDataType) const
		{
			return vertexLayouts[vertexDataType];
		}
	private:
		std::array<VertexLayout,4> vertexLayouts;
	};

	struct OpenglModel{
		std::pmr::vector<BufferedMesh> Meshes;
		std::pmr::vector<LoadedMaterial> Materils;
	};

	unsigned int calculateBufferSize(const dengine::Mesh& mesh);
	std::pmr::vector<LoadedMaterial> loadMaterialsToGpu(const dengine::Model& model);
	OpenglModel loadModelToGpu(const dengine::Model& model);
}

#endif

