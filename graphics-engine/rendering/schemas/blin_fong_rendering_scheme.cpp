#include <rendering/schemas/blin_fong_rendering_scheme.h>

#include <sstream>

#include <utils/shader_load_utils.h>
#include <glad/glad.h>


//ATTRIBUTE BINDINGS
constexpr unsigned int AttributePositionLocation = 0;
constexpr unsigned int AttributeNormalLocation = 1;
constexpr unsigned int AttributeUVsLocation = 2;
constexpr unsigned int AttributeModelMatrixBaseLocation = 3;
//UNIFORM BUFFER BINDINGS
constexpr unsigned int UboEnvironmentsBinding = 0;
//SHADER STORAGE BUFFER BINDINGS
constexpr unsigned int SsboLightsInfosBinding = 0;


unsigned dengine::BlinFongRenderingScheme::LoadShaderProgram()
{
	auto program = uploadAndCompileShaders("shaders/blin-fong.vert", "shaders/blin-fong.frag");
	glUniformBlockBinding(program, 0, UboEnvironmentsBinding);
	glShaderStorageBlockBinding(program, 0, SsboLightsInfosBinding);
	return program;
}


dengine::BlinFongRenderingUnit dengine::BlinFongRenderingScheme::CreateRenderingUnit(const BufferedMesh& mesh)
{
	unsigned int vbo = mesh.Vbo;
	unsigned int vao;

	unsigned int buffers[3];
	glCreateBuffers(3, buffers);
	unsigned int instanceBuffer = buffers[0];
	unsigned int environmentBuffer = buffers[1];
	unsigned int lightsBuffer = buffers[2];

	//InitializeBuffers
	glNamedBufferData(instanceBuffer, sizeof(BlinFongInstanceData) * 16, nullptr, GL_STREAM_DRAW);
	glNamedBufferData(environmentBuffer, sizeof(BlinFongEnvironmentData), nullptr, GL_STREAM_DRAW);
	glNamedBufferData(lightsBuffer, sizeof(BlinFongLightsInfo), nullptr, GL_STREAM_DRAW);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	//Positions vertex layout
	auto positionVertexLayout = mesh.GetVertexAttributeLayout(Positions);
	glVertexArrayVertexBuffer(vao, 0, vbo, positionVertexLayout.Offset,
	                          positionVertexLayout.Stride);
	glVertexArrayAttribFormat(vao, AttributePositionLocation, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao, AttributePositionLocation, 0);
	glEnableVertexArrayAttrib(vao, AttributePositionLocation);
	//Normals binding
	auto normalsLayout = mesh.GetVertexAttributeLayout(Normals);
	glVertexArrayVertexBuffer(vao, AttributeNormalLocation, vbo, normalsLayout.Offset, normalsLayout.Stride);
	glVertexArrayAttribFormat(vao, AttributeNormalLocation, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao, AttributeNormalLocation, 1);
	glEnableVertexArrayAttrib(vao, AttributeNormalLocation);
	//uvs binding
	auto uvsLayot = mesh.GetVertexAttributeLayout(UVs);
	glVertexArrayVertexBuffer(vao, AttributeUVsLocation, vbo, uvsLayot.Offset, uvsLayot.Stride);
	glVertexArrayAttribFormat(vao, AttributeUVsLocation, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao, AttributeUVsLocation, 2);
	glEnableVertexArrayAttrib(vao, AttributeUVsLocation);


	//Bind instance buffer
	glVertexArrayVertexBuffer(vao, AttributeModelMatrixBaseLocation, instanceBuffer, 0, sizeof(glm::mat4));
	for (int i = 0; i < 4; i++)
	{
		glVertexArrayAttribFormat(vao, AttributeModelMatrixBaseLocation + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4) * i);
		glVertexArrayAttribBinding(vao, AttributeModelMatrixBaseLocation + i, AttributeModelMatrixBaseLocation);
		glEnableVertexArrayAttrib(vao, AttributeModelMatrixBaseLocation + i);
		glVertexArrayBindingDivisor(vao, AttributeModelMatrixBaseLocation + i, 1);
	}

	glVertexArrayElementBuffer(vao, mesh.Ebo);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, environmentBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightsBuffer);
	glBindVertexArray(0);

	return BlinFongRenderingUnit{vao, mesh.NumElements, instanceBuffer, environmentBuffer, lightsBuffer};
}


std::pmr::string getBlinFongCacheId(unsigned int vaoId, unsigned int diffuseTextureId)
{
	std::stringstream ss;
	ss << "vao:{" << vaoId << "}" << "-" << "diffuseTexture: {" << diffuseTextureId << "}";
	return std::pmr::string(ss.str());
}


void dengine::BlinFongRenderingSubmiter::Submit(BlinFongRenderingUnit renderingUnit, Material material,
                                                glm::mat4 modelMatrix)
{
	auto cacheId = getBlinFongCacheId(renderingUnit.Vao, material.DiffuseTextureIndex);
	auto findIter = instancedToDraw.find(cacheId);
	if (findIter == instancedToDraw.end())
	{
		BlinFongSubmitInfo submitInfo{};
		submitInfo.DiffuseTexture = material.DiffuseTextureIndex;
		instancedToDraw[cacheId] = {renderingUnit, submitInfo};
	}
	auto& drawInstance = instancedToDraw[cacheId];
	drawInstance.second.InstanceDatas.push_back(BlinFongInstanceData{modelMatrix});
}


void dengine::BlinFongRenderingSubmiter::DispatchDrawCall(unsigned programId,
                                                          const GlobalEnvironment& environment) const
{
	glUseProgram(programId);

	BlinFongLightsInfo lightsInfo;
	lightsInfo.Info.Count = environment.LightsPositions.size();
	memcpy(lightsInfo.LightsPositions, &environment.LightsPositions[0],
	       environment.LightsPositions.size() * sizeof(glm::vec4));

	for (auto& index : instancedToDraw)
	{
		auto& submitInfo = index.second.second;
		auto& renderingUnit = index.second.first;

		glBindTextureUnit(0, index.second.second.DiffuseTexture);
		//Update draw info
		BlinFongEnvironmentData blinFongEnvironmentData{
			environment.CameraPostion, environment.ProjectionMatrix, environment.ViewMatrix
		};
		glNamedBufferSubData(renderingUnit.EnvironmentBuffer, 0, sizeof(BlinFongEnvironmentData),
		                     &blinFongEnvironmentData);
		//Update global environment
		glNamedBufferSubData(renderingUnit.InstaciesBuffer, 0,
		                     sizeof(BlinFongInstanceData) * submitInfo.InstanceDatas.size(),
		                     &submitInfo.InstanceDatas[0]); //Update model matricies
		glNamedBufferSubData(renderingUnit.LightsBuffer, 0, sizeof(BlinFongLightsInfo), &lightsInfo);
		//Update lights information
		glBindVertexArray(renderingUnit.Vao);
		glDrawElementsInstanced(GL_TRIANGLES, renderingUnit.IndeciesSize, GL_UNSIGNED_INT, nullptr,
		                        submitInfo.InstanceDatas.size());
	}
}


void dengine::BlinFongRenderingSubmiter::Clear()
{
	for (auto& index : instancedToDraw)
	{
		auto& submitInfo = index.second.second;
		submitInfo.InstanceDatas.clear();
	}
}
