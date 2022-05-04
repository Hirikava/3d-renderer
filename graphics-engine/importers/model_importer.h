#ifndef MODEL_IMPORTER_INCLUDED
#define MODEL_IMPORTER_INCLUDED

#include <vector>
#include <string>
#include <glm/glm.hpp>

namespace dengine
{
	enum TextureType{
		RGB,
		RGBA,
	};

	struct Texture{
		TextureType TextureType;
		int Width = 0;
		int Height = 0;
		std::pmr::vector<unsigned char> Data;
	};

	struct Mesh {
		std::pmr::vector<glm::vec3> Positions;
		std::pmr::vector<glm::vec3> Normals;
		std::pmr::vector<glm::vec3> Tangents;
		std::pmr::vector<glm::vec2> UVs;
		std::pmr::vector<unsigned int> Indecies;
		unsigned int MaterialIndex;
	};

	struct Material{
		int DiffuseTextureIndex {-1};
		int NormalTextureIndex {-1};
		int MetalnessTextureIndex{ -1 };
	};

	struct Model{
		std::pmr::vector<Mesh> Meshes;
		std::pmr::vector<Material> Materials;
		std::pmr::vector<Texture> Textures;
	};


	class IModelImporter{
	public:
		virtual ~IModelImporter() = default;
		virtual Model Import(std::pmr::string path) = 0;
	};
	
}

#endif

