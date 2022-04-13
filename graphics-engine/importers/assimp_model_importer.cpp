#include <importers/assimp_model_importer.h>

#include <filesystem>
namespace fs = std::filesystem;

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

constexpr unsigned int ImportFlags = aiProcess_CalcTangentSpace | 
			aiProcess_Triangulate | 
			aiProcess_FlipUVs | 
			aiProcess_EmbedTextures;


dengine::Texture loadTextureFromFile(const char* filePath)
{
	//SBT CODE HERE!
	return dengine::Texture{};
}

dengine::Mesh processMesh(const aiMesh* mesh,const  aiScene* scene)
{
	std::pmr::vector<glm::vec3> positions(mesh->mNumVertices);
	std::pmr::vector<glm::vec3> normals(mesh->mNumVertices);
	std::pmr::vector<glm::vec2> uvs(mesh->mNumVertices);

	const unsigned cmpSize = mesh->mNumVertices * sizeof(glm::vec3);
	memcpy(&positions[0], mesh->mVertices, mesh->mNumVertices * sizeof(glm::vec3)); //copy positions
	memcpy(&normals[0], mesh->mNormals, cmpSize); //copy normals
	for(int i = 0; i < mesh->mNumVertices; i++) //copy UVs
		uvs[i] = glm::vec2(mesh->mTextureCoords[0]->x, mesh->mTextureCoords[0]->y);

	return dengine::Mesh{
		positions,
		normals,
		uvs,
		mesh->mMaterialIndex,
	};
}

void processNode(const aiNode* node, const aiScene* scene, std::pmr::vector<dengine::Mesh>& meshes)
{
	for (int i = 0; i < node->mNumMeshes; i++)
		meshes.push_back(processMesh(scene->mMeshes[node->mMeshes[i]], scene));
	for (int i = 0; i < node->mNumChildren; i++)
		processNode(node->mChildren[i], scene, meshes);
}


dengine::Material loadMaterial(const aiMaterial* material,const std::pmr::string& pathTofile)
{
	dengine::Texture diffuseTexute;
	if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0)
	{
		aiString textureName;
		material->GetTexture(aiTextureType_DIFFUSE, 0, &textureName);
		auto path = fs::path(pathTofile).parent_path() / fs::path(textureName.C_Str());
		diffuseTexute = loadTextureFromFile(reinterpret_cast<const char*>(path.c_str()));
	}
	dengine::Texture normalTexture;
	if (material->GetTextureCount(aiTextureType_NORMALS) != 0)
	{
		aiString textureName;
		material->GetTexture(aiTextureType_NORMALS, 0, &textureName);
		auto path = fs::path(pathTofile).parent_path() / fs::path(textureName.C_Str());
		diffuseTexute = loadTextureFromFile(reinterpret_cast<const char*>(path.c_str()));
	}

	return dengine::Material{
		diffuseTexute,
		normalTexture
	};
}

void loadMaterials(const aiScene* scene, std::pmr::vector<dengine::Material>& materials,const std::pmr::string& pathToFile)
{
	for (int i = 0; i < scene->mNumMaterials; i++)
	{
		const aiMaterial* material = scene->mMaterials[i];
		materials.push_back(loadMaterial(material, pathToFile));
	}
}


dengine::Model dengine::AssimpModelImporter::Import(std::pmr::string path)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path.c_str(), ImportFlags);
	std::pmr::vector<Mesh> meshes;
	std::pmr::vector<Material> materials;

	processNode(scene->mRootNode, scene, meshes);
	loadMaterials(scene, materials, path);

	importer.FreeScene();
	return Model{
		meshes,
		materials,
	};
}
