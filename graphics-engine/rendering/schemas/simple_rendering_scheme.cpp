#include <rendering/schemas/simple_rendering_scheme.h>

#include <glad/glad.h>
#include <utils/shader_load_utils.h>
#include <sstream>

//ATTRIBUTE BINDINGS
constexpr unsigned int  AttributePositionLocation = 0;
constexpr unsigned int AttributeUVsLocation = 1;
constexpr unsigned int AttributeModelMatrixBaseLocation = 2;

//UNIFORM BUFFER BINDINGS
constexpr unsigned int UboEnvironmentBinding = 0;
//SHADER STORAGE BUFFER BINDINGS
constexpr unsigned int SsboMaterialBinding = 0;


unsigned dengine::SimpleRenderingScheme::LoadShaderProgram()
{
	auto program = uploadAndCompileShaders("shaders/simple.vert", "shaders/simple.frag");
	glUniformBlockBinding(program, UboEnvironmentBinding, 0);
	glShaderStorageBlockBinding(program, SsboMaterialBinding, 0);
	return program;
}


dengine::SimpleRenderingUnit dengine::SimpleRenderingScheme::CreateRenderingUnit(const BufferedMesh& mesh)
{
	unsigned int vbo = mesh.Vbo;
	unsigned int vao;

	unsigned int buffers[3];
	glCreateBuffers(3, buffers);
	unsigned int instanceBuffer = buffers[0];
	unsigned int materialsBuffer = buffers[1];
	unsigned int environmentBuffer = buffers[2];

	//InitializeBuffers
	glNamedBufferData(instanceBuffer, sizeof(SimpleInstanceData) * 16, nullptr, GL_STREAM_DRAW);
	glNamedBufferData(materialsBuffer, sizeof(SimpleMaterialData) * 16, nullptr, GL_STREAM_DRAW);
	glNamedBufferData(environmentBuffer, sizeof(SimpleEnvironmentData), nullptr, GL_STREAM_DRAW);


	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	//Positions vertex layout
	auto positionVertexLayout = mesh.GetVertexAttributeLayout(Positions);
	glVertexArrayVertexBuffer(vao, AttributePositionLocation, vbo, positionVertexLayout.Offset,
	                          positionVertexLayout.Stride);
	glVertexArrayAttribFormat(vao, AttributePositionLocation, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao, AttributePositionLocation, 0);
	glEnableVertexArrayAttrib(vao, AttributePositionLocation);
	//Uvs binding
	auto uvVertexLayout = mesh.GetVertexAttributeLayout(UVs);
	glVertexArrayVertexBuffer(vao, 1, vbo, uvVertexLayout.Offset, uvVertexLayout.Stride);
	glVertexArrayAttribFormat(vao, AttributeUVsLocation, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao, AttributeUVsLocation, 1);
	glEnableVertexArrayAttrib(vao, AttributeUVsLocation);

	//Bind instance buffer
	glVertexArrayVertexBuffer(vao, 2, instanceBuffer, 0, sizeof(glm::mat4));
	for (int i = 0; i < 4; i++)
	{
		glVertexArrayAttribFormat(vao, AttributeModelMatrixBaseLocation + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4) * i);
		glVertexArrayAttribBinding(vao, AttributeModelMatrixBaseLocation + i, 2);
		glEnableVertexArrayAttrib(vao, AttributeModelMatrixBaseLocation + i);
		glVertexArrayBindingDivisor(vao, AttributeModelMatrixBaseLocation + i, 1);
	}

	glVertexArrayElementBuffer(vao, mesh.Ebo);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, environmentBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, materialsBuffer);
	glBindVertexArray(0);

	return SimpleRenderingUnit{vao, mesh.NumElements, instanceBuffer, materialsBuffer, environmentBuffer};
}


std::pmr::string getCacheId(unsigned int vaoId, unsigned int diffuseTextureId)
{
	std::stringstream ss;
	ss << "vao:{" << vaoId << "}" << "-" << "diffuseTexture: {" << diffuseTextureId << "}";
	return std::pmr::string(ss.str());
}


void dengine::SimpleRedneringSubmitter::Submit(SimpleRenderingUnit renderingUnit, Material material,
                                               glm::mat4 modelMatrix)
{
	auto cacheId = getCacheId(renderingUnit.Vao, material.DiffuseTextureIndex);
	auto findIter = instancedToDraw.find(cacheId);
	if (findIter == instancedToDraw.end())
	{
		SimpleSubmitInfo submitInfo{};
		submitInfo.DiffuseTexture = material.DiffuseTextureIndex;
		instancedToDraw[cacheId] = { renderingUnit, submitInfo };
	}
	auto& drawInstance = instancedToDraw[cacheId];
	drawInstance.second.SimpleInstanceData.push_back(SimpleInstanceData{modelMatrix});
	SimpleMaterialData simpleMaterialData{};
	simpleMaterialData.BaseColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
	simpleMaterialData.ColorSelector.Index = material.DiffuseTextureIndex == -1 ? 1 : 0;
	drawInstance.second.SimpleMaterialData.push_back(simpleMaterialData);
}


void dengine::SimpleRedneringSubmitter::DispatchDrawCall(unsigned programId, GlobalEnvironment environment) const
{
	glUseProgram(programId);
	GLuint indices[2] = {0, 1};
	glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 2, indices);

	for (auto& index : instancedToDraw)
	{
		auto& submitInfo = index.second.second;
		auto& renderingUnit = index.second.first;

		glBindTextureUnit(0, index.second.second.DiffuseTexture);
		//Update draw info
		SimpleEnvironmentData simpleEnvironmentData{environment.ProjectionMatrix, environment.ViewMatrix};
		glNamedBufferSubData(renderingUnit.EnvironmentBuffer, 0, sizeof(SimpleEnvironmentData), &simpleEnvironmentData);
		//Update global environment
		glNamedBufferSubData(renderingUnit.InstaciesBuffer, 0,
		                     sizeof(SimpleInstanceData) * submitInfo.SimpleInstanceData.size(),
		                     &submitInfo.SimpleInstanceData[0]); //Update model matricies
		glNamedBufferSubData(renderingUnit.MaterialsBuffer, 0,
		                     sizeof(SimpleMaterialData) * submitInfo.SimpleMaterialData.size(),
		                     &submitInfo.SimpleMaterialData[0]); //Update model materials
		glBindVertexArray(renderingUnit.Vao);
		glDrawElementsInstanced(GL_TRIANGLES, renderingUnit.IndeciesSize, GL_UNSIGNED_INT, nullptr,
		                        submitInfo.SimpleInstanceData.size());
	}
}


void dengine::SimpleRedneringSubmitter::Clear()
{
	for (auto& index : instancedToDraw)
	{
		auto& submitInfo = index.second.second;
		submitInfo.SimpleInstanceData.clear();
		submitInfo.SimpleMaterialData.clear();
	}
}
