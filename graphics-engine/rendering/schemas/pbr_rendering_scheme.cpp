#include <rendering/schemas/pbr_rendering_scheme.h>
#include <rendering/schemas/blin_fong_rendering_scheme.h>
#include <sstream>
#include <utils/shader_load_utils.h>
#include <glad/glad.h>


//ATTRIBUTE BINDINGS
constexpr unsigned int AttributePositionLocation = 0;
constexpr unsigned int AttributeNormalLocation = 1;
constexpr unsigned int AttributeUVsLocation = 2;
constexpr unsigned int AttributeTangentLocation = 3;
constexpr unsigned int AttributeModelMatrixBaseLocation = 4;
//UNIFORM BUFFER BINDINGS
constexpr unsigned int UboEnvironmentsBinding = 0;
//SHADER STORAGE BUFFER BINDINGS
constexpr unsigned int SsboLightsInfosBinding = 0;


unsigned dengine::PbrRenderingScheme::LoadShaderProgram()
{
	auto program = uploadAndCompileShaders("shaders/pbr.vert", "shaders/pbr.frag");
	glUniformBlockBinding(program, 0, UboEnvironmentsBinding);
	glShaderStorageBlockBinding(program, 0, SsboLightsInfosBinding);
	return program;
}


dengine::PbrRenderingUnit dengine::PbrRenderingScheme::CreateRenderingUnit(const BufferedMesh& mesh, OpenglSettings openglSettings)
{
	unsigned int vbo = mesh.Vbo;
	unsigned int vao;

	unsigned int buffers[3];
	glCreateBuffers(3, buffers);
	unsigned int instanceBuffer = buffers[0];
	unsigned int environmentBuffer = buffers[1];
	unsigned int lightsBuffer = buffers[2];


	//InitializeBuffers
	glNamedBufferData(instanceBuffer, sizeof(PbrInstancesData) * 16, nullptr, GL_STREAM_DRAW);
	glNamedBufferData(environmentBuffer, sizeof(PbrEnvironmentData), nullptr, GL_STREAM_DRAW);
	glNamedBufferData(lightsBuffer, sizeof(PbrLightsInfo), nullptr, GL_STREAM_DRAW);

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
	//tangents binding
	auto tangentLayout = mesh.GetVertexAttributeLayout(Tangents);
	glVertexArrayVertexBuffer(vao, AttributeTangentLocation, vbo, tangentLayout.Offset, tangentLayout.Stride);
	glVertexArrayAttribFormat(vao, AttributeTangentLocation, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao, AttributeTangentLocation, 3);
	glEnableVertexArrayAttrib(vao, AttributeTangentLocation);


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


	glBindBufferRange(GL_UNIFORM_BUFFER, UboEnvironmentsBinding, environmentBuffer, 0, openglSettings.uniformAlignment);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightsBuffer);
	glBindVertexArray(0);

	return PbrRenderingUnit{ vao, mesh.NumElements, instanceBuffer, environmentBuffer, lightsBuffer };
}


dengine::PbrRenderingSubmitter::PbrRenderingSubmitter(OpenglSettings openglSettings) : openglSettings(openglSettings) {}


std::pmr::string getPbrCacheId(unsigned int vaoId, const dengine::Material& material)
{
	std::stringstream ss;
	ss << "vao:{" << vaoId << "}" << "-" << "diffuseTexture:{"
		<< material.DiffuseTextureIndex << "}" << "-" << "normalTexture:{"
		<< material.NormalTextureIndex << "}" << "-" << "metalinessTexture:{"
		<< material.MetalnessTextureIndex << "}";
	return std::pmr::string(ss.str());
}


void dengine::PbrRenderingSubmitter::Submit(PbrRenderingUnit renderingUnit, Material material,
	glm::mat4 modelMatrix)
{
	auto cacheId = getPbrCacheId(renderingUnit.Vao, material);
	auto findIter = instancedToDraw.find(cacheId);
	if (findIter == instancedToDraw.end())
	{
		PbrSubmitInfo submitInfo{};
		submitInfo.DiffuseTexture = material.DiffuseTextureIndex;
		submitInfo.NormalTexture = material.NormalTextureIndex;
		submitInfo.MetalnessTexture = material.MetalnessTextureIndex;
		instancedToDraw[cacheId] = { renderingUnit, submitInfo };
	}
	auto& drawInstance = instancedToDraw[cacheId];
	drawInstance.second.InstanceDatas.push_back(PbrInstancesData{ modelMatrix });
}


void dengine::PbrRenderingSubmitter::DispatchDrawCall(unsigned programId,
	const GlobalEnvironment& environment) const
{
	glUseProgram(programId);

	PbrLightsInfo lightsInfo;
	lightsInfo.Info.Count = environment.Lights.size();
	memcpy(lightsInfo.LightsInfos, &environment.Lights[0],
		environment.Lights.size() * sizeof(LightInfo));

	PbrEnvironmentData environmentData;
	environmentData.CameraPosition = environment.CameraPostion;
	environmentData.ProjectionMatrix = environment.ProjectionMatrix;
	environmentData.ViewMatrix = environment.ViewMatrix;

	//load data to gpu
	auto sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	for (auto& index : instancedToDraw)
	{
		auto& submitInfo = index.second.second;
		auto& renderingUnit = index.second.first;
		glNamedBufferSubData(renderingUnit.EnvironmentBuffer, 0, sizeof(PbrEnvironmentData), &environmentData);
		glNamedBufferSubData(renderingUnit.InstaciesBuffer, 0,
			sizeof(PbrInstancesData) * submitInfo.InstanceDatas.size(),
			&submitInfo.InstanceDatas[0]); //Update model matricies
		glNamedBufferSubData(renderingUnit.LightsBuffer, 0, sizeof(PbrLightsInfo), &lightsInfo);//Update lights information
	}
	glWaitSync(sync, 0, GL_TIMEOUT_IGNORED);
	glDeleteSync(sync);

	//render all
	for (auto& index : instancedToDraw)
	{
		auto& submitInfo = index.second.second;
		auto& renderingUnit = index.second.first;

		glBindTextureUnit(0, index.second.second.DiffuseTexture);
		glBindTextureUnit(1, index.second.second.NormalTexture);
		glBindTextureUnit(2, index.second.second.MetalnessTexture);
		glBindVertexArray(renderingUnit.Vao);
		glDrawElementsInstanced(GL_TRIANGLES, renderingUnit.IndeciesSize, GL_UNSIGNED_INT, nullptr,
			submitInfo.InstanceDatas.size());
	}
}


void dengine::PbrRenderingSubmitter::Clear()
{
	for (auto& index : instancedToDraw)
	{
		auto& submitInfo = index.second.second;
		submitInfo.InstanceDatas.clear();
	}
}
