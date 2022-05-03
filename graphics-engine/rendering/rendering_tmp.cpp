#include <rendering/rendering_tmp.h>
#include <glad/glad.h>


unsigned dengine::calculateBufferSize(const Mesh& mesh)
{
	return mesh.Positions.size() * sizeof(glm::vec3) + mesh.Normals.size() * sizeof(glm::vec3) + mesh.UVs.size() *
		sizeof(glm::vec2) + mesh.Tangents.size() * sizeof(glm::vec3);
}


std::pmr::vector<dengine::LoadedMaterial> dengine::loadMaterialsToGpu(const Model& model)
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

			auto textureFormat = GL_RGBA;
			glTextureSubImage2D(diffuseTexture, 0, 0, 0, textureToLoad.Width, textureToLoad.Height, textureFormat,
			                    GL_UNSIGNED_BYTE, &textureToLoad.Data[0]);

			glTextureParameteri(diffuseTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTextureParameteri(diffuseTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTextureParameteri(diffuseTexture, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTextureParameteri(diffuseTexture, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}
		else
			diffuseTexture = index->second;

		materials.push_back(LoadedMaterial{static_cast<int>(diffuseTexture)});
	}
	return materials;
}


dengine::OpenglModel dengine::loadModelToGpu(const Model& model)
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
		vertexLayouts[Positions] = VertexLayout{sizeof(glm::vec3), offset, mesh.Positions.size() * sizeof(glm::vec3)};
		offset += mesh.Positions.size() * sizeof(glm::vec3);
		//load normals
		glNamedBufferSubData(*vboPtr, offset, mesh.Normals.size() * sizeof(glm::vec3), &mesh.Normals[0]);
		vertexLayouts[Normals] = VertexLayout{sizeof(glm::vec3), offset, mesh.Normals.size() * sizeof(glm::vec3)};
		offset += mesh.Normals.size() * sizeof(glm::vec3);
		//load uvs
		glNamedBufferSubData(*vboPtr, offset, mesh.UVs.size() * sizeof(glm::vec2), &mesh.UVs[0]);
		vertexLayouts[UVs] = VertexLayout{sizeof(glm::vec2), offset, mesh.UVs.size() * sizeof(glm::vec2)};
		offset += mesh.UVs.size() * sizeof(glm::vec2);
		//load tangents
		glNamedBufferSubData(*vboPtr, offset, mesh.UVs.size() * sizeof(glm::vec2), &mesh.UVs[0]);
		vertexLayouts[UVs] = VertexLayout{ sizeof(glm::vec3), offset, mesh.Tangents.size() * sizeof(glm::vec3) };
		offset += mesh.Tangents.size() * sizeof(glm::vec3);
		//load elements
		glNamedBufferData(*eboPtr, mesh.Indecies.size() * sizeof(unsigned), &mesh.Indecies[0], GL_STATIC_DRAW);
		bufferedMeshes.push_back(BufferedMesh{
			*vboPtr, *eboPtr, mesh.MaterialIndex, mesh.Indecies.size(), vertexLayouts
		});
	}

	auto materials = loadMaterialsToGpu(model);
	return OpenglModel{bufferedMeshes, materials,};
}
