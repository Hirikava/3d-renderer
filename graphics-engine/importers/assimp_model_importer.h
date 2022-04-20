#ifndef ASSIMP_MODEL_IMPORTER_INCLUDED
#define ASSIMP_MODEL_IMPORTER_INCLUDED

#include <spdlog/spdlog.h>
#include <importers/model_importer.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

namespace dengine{
	class AssimpModelImporter : public IModelImporter {
	public:
		AssimpModelImporter(std::shared_ptr<spdlog::logger> log);
		Model Import(std::pmr::string path) override;

	private:
		std::pmr::vector<dengine::Texture> loadEmbededTextures(const aiScene* scene);
		dengine::Texture loadTextureFromMemmory(unsigned char* zipData, unsigned len);
		std::pmr::vector<dengine::Material> loadMaterials(const aiScene* scene);
		void processNode(const aiNode* node, const aiScene* scene, std::pmr::vector<dengine::Mesh>& meshes);
		dengine::Mesh processMesh(const aiMesh* mesh, const  aiScene* scene);

		Assimp::Importer importer;
		std::shared_ptr<spdlog::logger> log;
	};
	
}


#endif
