#ifndef RENDERING_TMP_INCLUDED
#define RENDERING_TMP_INCLUDED

#include <map>

#include <glad/glad.h>
#include <importers/model_importer.h>

namespace dengine
{
	enum VertexDataType{
		Positions = 0,
		Normals = 1,
		Tangents = 2,
		UVs = 3,
	};

	struct VertexLayout {
		unsigned int Stride;
		unsigned int Offset;
		unsigned int Size;
	};

	struct LoadedMaterial{
		unsigned int DiffuseTextureId;
	};


	struct BufferedMesh {
		unsigned int Vbo;
		unsigned int Ebo;
		unsigned int MaterialIndex;
	};


	struct OpenglModel{
		std::pmr::vector<BufferedMesh> Meshes;
		std::pmr::vector<LoadedMaterial> Materils;
	};

	unsigned int calculateBufferSize(const dengine::Mesh& mesh)
	{
		return  mesh.Positions.size() * sizeof(glm::vec3) +
			mesh.Normals.size() * sizeof(glm::vec3) +
			mesh.UVs.size() * sizeof(glm::vec2);
	}

	std::pmr::vector<LoadedMaterial> loadMaterialsToGpu(const dengine::Model& model)
	{
		std::pmr::vector<LoadedMaterial> materials;
		std::pmr::map<unsigned int, unsigned int> loadedTextures;

		for (auto& material : model.Materials)
		{
			auto& texture = model.Textures[material.DiffuseTextureIndex];

			unsigned int diffuseTexture;
			auto index = loadedTextures.find(material.DiffuseTextureIndex);
			if (index == loadedTextures.end())
			{
				auto& textureToLoad = model.Textures[material.DiffuseTextureIndex];
				glCreateTextures(GL_TEXTURE_2D, 1, &diffuseTexture);
				glTextureStorage2D(diffuseTexture, 1, GL_RGBA, textureToLoad.Width, textureToLoad.Height);
				glTextureSubImage2D(diffuseTexture, 0, 0, 0, textureToLoad.Height, textureToLoad.Height, GL_RGBA, GL_UNSIGNED_BYTE, &textureToLoad.Data[0]);

				glTextureParameteri(diffuseTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTextureParameteri(diffuseTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTextureParameteri(diffuseTexture, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTextureParameteri(diffuseTexture, GL_TEXTURE_WRAP_T, GL_REPEAT);
			}
			else
				diffuseTexture = index->second;

			materials.push_back(LoadedMaterial{
				diffuseTexture
				});
		}

		return materials;
	}

	OpenglModel loadModelToGpu(const dengine::Model& model)
	{

		std::pmr::vector<BufferedMesh> bufferedMeshes;
		for (const auto& mesh : model.Meshes)
		{
			unsigned int buffers[2];
			unsigned int* vboPtr = &buffers[0];
			unsigned int* eboPtr = &buffers[1];
			glGenBuffers(2, buffers);

			auto bufferSize = calculateBufferSize(mesh);
			glNamedBufferData(*vboPtr, bufferSize, nullptr, GL_STATIC_DRAW);
			//load postions
			unsigned int offset = 0;
			glNamedBufferSubData(*vboPtr, offset, mesh.Positions.size() * sizeof(glm::vec3), &mesh.Positions[0]);
			offset += mesh.Positions.size() * sizeof(glm::vec3);
			//load normals
			glNamedBufferSubData(*vboPtr, offset, mesh.Normals.size() * sizeof(glm::vec3), &mesh.Normals[0]);
			offset += mesh.Normals.size() * sizeof(glm::vec3);
			//load uvs
			glNamedBufferSubData(*vboPtr, offset, mesh.UVs.size() * sizeof(glm::vec2), &mesh.UVs[0]);
			offset += mesh.UVs.size() * sizeof(glm::vec2);
			//load elements
			glNamedBufferData(*eboPtr, mesh.Indecies.size() * sizeof(unsigned), &mesh.Indecies[0], GL_STATIC_DRAW);
			bufferedMeshes.push_back(BufferedMesh{
				*vboPtr,
				*eboPtr,
				mesh.MaterialIndex
				});
		}

		auto materials = loadMaterialsToGpu(model);
		return OpenglModel{
			bufferedMeshes,
			materials,
		};
	}

	
}

#endif

