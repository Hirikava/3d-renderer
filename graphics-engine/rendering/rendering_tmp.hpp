#ifndef RENDERING_TMP_INCLUDED
#define RENDERING_TMP_INCLUDED

#include <map>
#include <array>

#include <glad/glad.h>
#include <importers/model_importer.h>


namespace dengine
{
	enum VertexDataType{
		Positions = 0,
		Normals = 1,
		UVs = 2,
	};

	struct VertexLayout {
		unsigned int Stride {0};
		unsigned int Offset {0};
		unsigned long long Size {0};
	};

	struct LoadedMaterial{
		unsigned int DiffuseTextureId;
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
				glTextureStorage2D(diffuseTexture, 1, GL_RGBA8, textureToLoad.Width, textureToLoad.Height);

				auto textureFormat = textureToLoad.TextureType == RGB ? GL_RGB : GL_RGBA;
				glTextureSubImage2D(diffuseTexture, 0, 0, 0, textureToLoad.Width, textureToLoad.Height, textureFormat, GL_UNSIGNED_BYTE, &textureToLoad.Data[0]);

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
		std::array<VertexLayout, 4> vertexLayouts;
		for (const auto& mesh : model.Meshes)
		{
			unsigned int buffers[2];
			unsigned int* vboPtr = &buffers[0];
			unsigned int* eboPtr = &buffers[1];
			glCreateBuffers(2, buffers);

			auto bufferSize = calculateBufferSize(mesh);
			glNamedBufferData(*vboPtr, bufferSize, nullptr, GL_STATIC_DRAW);
			//load postions
			unsigned int offset = 0;
			glNamedBufferSubData(*vboPtr, offset, mesh.Positions.size() * sizeof(glm::vec3), &mesh.Positions[0]);
			vertexLayouts[Positions] = VertexLayout{ sizeof(glm::vec3), offset, mesh.Positions.size() * sizeof(glm::vec3) };
			offset += mesh.Positions.size() * sizeof(glm::vec3);
			//load normals
			glNamedBufferSubData(*vboPtr, offset, mesh.Normals.size() * sizeof(glm::vec3), &mesh.Normals[0]);
			vertexLayouts[Normals] = VertexLayout{ sizeof(glm::vec3), offset, mesh.Normals.size() * sizeof(glm::vec3) };
			offset += mesh.Normals.size() * sizeof(glm::vec3);
			//load uvs
			glNamedBufferSubData(*vboPtr, offset, mesh.UVs.size() * sizeof(glm::vec2), &mesh.UVs[0]);
			vertexLayouts[UVs] = VertexLayout{ sizeof(glm::vec2), offset, mesh.UVs.size() * sizeof(glm::vec2) };
			offset += mesh.UVs.size() * sizeof(glm::vec2);
			//load elements
			glNamedBufferData(*eboPtr, mesh.Indecies.size() * sizeof(unsigned), &mesh.Indecies[0], GL_STATIC_DRAW);
			bufferedMeshes.push_back(BufferedMesh{
				*vboPtr,
				*eboPtr,
				mesh.MaterialIndex,
				mesh.Indecies.size(),
				vertexLayouts
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

