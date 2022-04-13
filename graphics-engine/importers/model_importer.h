#ifndef MODEL_IMPORTER_INCLUDED
#define MODEL_IMPORTER_INCLUDED

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace dengine
{
	enum TextureType{
		RGB,
		RGBA,
	};

	struct Texture{
		TextureType TextureType;
		unsigned int Width = 0;
		unsigned int Height = 0;
		std::pmr::vector<unsigned> Data;
	};

	struct Mesh {
		std::pmr::vector<glm::vec3> Positions;
		std::pmr::vector<glm::vec3> Normals;
		std::pmr::vector<glm::vec2> UVs;
		unsigned int textureIndex;
	};

	struct Material{
		Texture DiffuseTexture;
		Texture NormalTexture;
	};

	struct Model{
		std::pmr::vector<Mesh> Meshes;
		std::pmr::vector<Material> Materials;
	};


	class IModelImporter{
	public:
		virtual ~IModelImporter() = default;
		virtual Model Import(std::pmr::string path) = 0;
	};
	
}

#endif

