#ifndef ASSIMP_MODEL_IMPORTER_INCLUDED
#define ASSIMP_MODEL_IMPORTER_INCLUDED

#include <spdlog/spdlog.h>
#include <importers/model_importer.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

namespace dengine{
	class AssimpModelImporter : public IModelImporter {
	public:
		explicit AssimpModelImporter(const std::shared_ptr<spdlog::logger> log = spdlog::get("app_logger"));
		Model Import(std::pmr::string path) override;

	private:
		static std::pmr::vector<dengine::Texture> loadEmbededTextures(const aiScene* scene);
		static dengine::Texture loadTextureFromMemmory(unsigned char* zipData, unsigned len);
		static std::pmr::vector<dengine::Material> loadMaterials(const aiScene* scene);
		void processNode(const aiNode* node, const aiScene* scene, std::pmr::vector<dengine::Mesh>& meshes);
		static dengine::Mesh processMesh(const aiMesh* mesh, const  aiScene* scene);

		Assimp::Importer importer;
		std::shared_ptr<spdlog::logger> log;
	};
	
}


#endif
