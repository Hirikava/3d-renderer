#include <importers/assimp_model_importer.h>

#include <filesystem>
namespace fs = std::filesystem;

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


constexpr unsigned int ImportFlags = aiProcess_CalcTangentSpace |
	aiProcess_Triangulate |
	aiProcess_FlipUVs |
	aiProcess_EmbedTextures;



dengine::AssimpModelImporter::AssimpModelImporter(const std::shared_ptr<spdlog::logger> log) : log(log)
{
}

dengine::Model dengine::AssimpModelImporter::Import(std::pmr::string path)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path.c_str(), ImportFlags);
	if (scene == nullptr)
	{
		auto errorString = importer.GetErrorString();
		log->error("Failed to import model from {} with following error message:'{}'", path.c_str(), errorString);
		return Model{};
	}
	//load geometry
	std::pmr::vector<Mesh> meshes;
	processNode(scene->mRootNode, scene, meshes);
	//load maps and materials
	auto textures = loadEmbededTextures(scene);
	auto materials = loadMaterials(scene);

	importer.FreeScene();
	return Model{
		meshes,
		materials,
		textures
	};
}

std::pmr::vector<dengine::Texture> dengine::AssimpModelImporter::loadEmbededTextures(const aiScene* scene)
{
	std::pmr::vector<Texture> embededTextures;
	for (int i = 0; i < scene->mNumTextures; i++)
	{
		const aiTexture* aiTexture = scene->mTextures[i];
		if (aiTexture->mHeight == 0)
		{
			const auto zipDataPtr = reinterpret_cast<unsigned char*>(aiTexture->pcData);
			embededTextures.push_back(loadTextureFromMemmory(zipDataPtr, aiTexture->mWidth));
		}
		else
		{
			const unsigned int textureSizeInBytes = aiTexture->mWidth * aiTexture->mHeight * 4;
			std::pmr::vector<unsigned char> data(textureSizeInBytes);
			std::memcpy(&data[0], aiTexture->pcData, textureSizeInBytes);
			embededTextures.push_back(Texture{
				RGBA,
				static_cast<int>(aiTexture->mWidth),
				static_cast<int>(aiTexture->mHeight),
				data
			});
		}
	}
	return embededTextures;
}

dengine::Texture dengine::AssimpModelImporter::loadTextureFromMemmory(unsigned char* zipData, unsigned len)
{
	int width = 0, height = 0, numChannels = 0;
	unsigned char* data = stbi_load_from_memory(zipData, len, &width, &height, &numChannels, 4);
	const auto textureSizeInBytes = width * height * 4;
	std::pmr::vector<unsigned char> textureData(textureSizeInBytes);
	memcpy(&textureData[0], data, textureSizeInBytes);
	stbi_image_free(data);
	return Texture{
		RGBA,
		width,
		height,
		textureData,
	};
}

std::pmr::vector<dengine::Material> dengine::AssimpModelImporter::loadMaterials(const aiScene* scene)
{
	std::pmr::vector<Material> materials;
	for (int i = 0; i < scene->mNumMaterials; i++)
	{
		const aiMaterial* aiMaterial = scene->mMaterials[i];
		if (aiMaterial->GetTextureCount(aiTextureType_BASE_COLOR) > 0)
		{
			aiString textureName;
			//get diffuse texture
			aiMaterial->GetTexture(aiTextureType_BASE_COLOR, 0, &textureName);
			const auto diffuseTextureIndex = scene->GetEmbeddedTextureAndIndex(textureName.C_Str());

			//get normal texture
			aiMaterial->GetTexture(aiTextureType_NORMALS, 0, &textureName);
			const auto normalTextureIndex = scene->GetEmbeddedTextureAndIndex(textureName.C_Str());
			materials.push_back(Material{diffuseTextureIndex.second, normalTextureIndex.second});
		}
	}
	return materials;
}

void dengine::AssimpModelImporter::processNode(const aiNode* node, const aiScene* scene, std::pmr::vector<Mesh>& meshes)
{
	for (int i = 0; i < node->mNumMeshes; i++)
		meshes.push_back(processMesh(scene->mMeshes[node->mMeshes[i]], scene));
	for (int i = 0; i < node->mNumChildren; i++)
		processNode(node->mChildren[i], scene, meshes);
}

dengine::Mesh dengine::AssimpModelImporter::processMesh(const aiMesh* mesh, const aiScene* scene)
{
	std::pmr::vector<glm::vec3> positions(mesh->mNumVertices);
	std::pmr::vector<glm::vec3> tangents(mesh->mNumVertices);
	std::pmr::vector<glm::vec3> normals(mesh->mNumVertices);
	std::pmr::vector<glm::vec2> uvs(mesh->mNumVertices);


	const unsigned cmpSize = mesh->mNumVertices * sizeof(glm::vec3);
	memcpy(&positions[0], mesh->mVertices, mesh->mNumVertices * sizeof(glm::vec3)); //copy positions
	memcpy(&normals[0], mesh->mNormals, cmpSize); //copy normals
	memcpy(&tangents[0], mesh->mTangents, cmpSize); //copy normals
	for (int i = 0; i < mesh->mNumVertices; i++) //copy UVs
		uvs[i] = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);


	//copy indecies
	unsigned int indeciesSize = 0;
	for (int i = 0; i < mesh->mNumFaces; i++)
		indeciesSize += mesh->mFaces[i].mNumIndices;
	std::pmr::vector<unsigned int> indecies(indeciesSize);
	unsigned int offset = 0;
	for (int i = 0; i < mesh->mNumFaces; i++)
	{
		const aiFace face = mesh->mFaces[i];
		const unsigned copySize = face.mNumIndices * sizeof(unsigned);
		memcpy((offset + &indecies[0]), face.mIndices, copySize);
		offset += face.mNumIndices;
	}

	return Mesh{
		positions,
		normals,
		tangents,
		uvs,
		indecies,
		mesh->mMaterialIndex,
	};
}
