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
constexpr unsigned int UboLightsBinding = 1;
//SHADER STORAGE BUFFER BINDINGS
constexpr unsigned int SsboLightsInfosBinding = 0;


unsigned dengine::BlinFongRenderingScheme::LoadShaderProgram()
{
	auto program = uploadAndCompileShaders("shaders/blin-fong.vert", "shaders/blin-fong.frag");
	glUniformBlockBinding(program, 0, UboEnvironmentsBinding);
	glUniformBlockBinding(program, 1, UboLightsBinding);
	glShaderStorageBlockBinding(program, 0, SsboLightsInfosBinding);
	return program;
}

std::pair<int, int> calculateSizeAndOffset(const dengine::OpenglSettings openglSettings)
{
	auto envSize = offsetof(dengine::BlinFongEnvironmentData, LightsSettings);
	auto lightSettingSize = sizeof(dengine::LightsSettings);
	auto envAlignCount = (envSize / openglSettings.uniformAlignment);
	envAlignCount += sizeof(dengine::BlinFongEnvironmentData) % openglSettings.uniformAlignment == 0 ? 0 : 1;
	return { envAlignCount * openglSettings.uniformAlignment + lightSettingSize,
		envAlignCount * openglSettings.uniformAlignment};
}

dengine::BlinFongRenderingUnit dengine::BlinFongRenderingScheme::CreateRenderingUnit(const BufferedMesh& mesh, OpenglSettings openglSettings)
{
	unsigned int vbo = mesh.Vbo;
	unsigned int vao;

	unsigned int buffers[3];
	glCreateBuffers(3, buffers);
	unsigned int instanceBuffer = buffers[0];
	unsigned int environmentBuffer = buffers[1];
	unsigned int lightsBuffer = buffers[2];


	auto sizeAndOffset = calculateSizeAndOffset(openglSettings);
	//InitializeBuffers
	glNamedBufferData(instanceBuffer, sizeof(BlinFongInstanceData) * 16, nullptr, GL_STREAM_DRAW);
	glNamedBufferData(environmentBuffer, openglSettings.uniformAlignment + sizeof(LightsSettings), nullptr, GL_STREAM_DRAW);
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
	glBindBufferRange(GL_UNIFORM_BUFFER, UboLightsBinding, environmentBuffer, openglSettings.uniformAlignment, sizeof(LightsSettings));
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightsBuffer);
	glBindVertexArray(0);

	return BlinFongRenderingUnit{vao, mesh.NumElements, instanceBuffer, environmentBuffer, lightsBuffer};
}


dengine::BlinFongRenderingSubmiter::BlinFongRenderingSubmiter(OpenglSettings openglSettings) : openglSettings(openglSettings) {}


std::pmr::string getBlinFongCacheId(unsigned int vaoId, const dengine::Material& material)
{
	std::stringstream ss;
	ss << "vao:{" << vaoId << "}" << "-" << "diffuseTexture:{"
		<< material.DiffuseTextureIndex << "}" << "-" << "normalTexture:{"
		<< material.NormalTextureIndex << "}";
	return std::pmr::string(ss.str());
}


void dengine::BlinFongRenderingSubmiter::Submit(BlinFongRenderingUnit renderingUnit, Material material,
                                                glm::mat4 modelMatrix)
{
	auto cacheId = getBlinFongCacheId(renderingUnit.Vao, material);
	auto findIter = instancedToDraw.find(cacheId);
	if (findIter == instancedToDraw.end())
	{
		BlinFongSubmitInfo submitInfo{};
		submitInfo.DiffuseTexture = material.DiffuseTextureIndex;
		submitInfo.NormalTexture = material.NormalTextureIndex;
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
	lightsInfo.Info.Count = environment.Lights.size();
	memcpy(lightsInfo.LightsInfos, &environment.Lights[0],
	       environment.Lights.size() * sizeof(LightInfo));

	BlinFongEnvironmentData blinFongEnvironmentData;
	blinFongEnvironmentData.CameraPosition = environment.CameraPostion;
	blinFongEnvironmentData.ProjectionMatrix = environment.ProjectionMatrix;
	blinFongEnvironmentData.ViewMatrix = environment.ViewMatrix;
	blinFongEnvironmentData.LightsSettings = LightsSettings{ environment.AmbientStrength, environment.DiffuseStrength, environment.SpecularStrength, environment.SpecularPower};

	//load data to gpu
	auto sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	auto offsetAndAlignment = calculateSizeAndOffset(openglSettings);
	for (auto& index : instancedToDraw)
	{
		auto& submitInfo = index.second.second;
		auto& renderingUnit = index.second.first;

		LightsSettings lightsSettings = blinFongEnvironmentData.LightsSettings;
		glNamedBufferSubData(renderingUnit.EnvironmentBuffer, 0, offsetof(BlinFongEnvironmentData, LightsSettings), &blinFongEnvironmentData);
		glNamedBufferSubData(renderingUnit.EnvironmentBuffer, openglSettings.uniformAlignment, sizeof(LightsSettings), &lightsSettings);
		glNamedBufferSubData(renderingUnit.InstaciesBuffer, 0,
		                     sizeof(BlinFongInstanceData) * submitInfo.InstanceDatas.size(),
		                     &submitInfo.InstanceDatas[0]); //Update model matricies
		glNamedBufferSubData(renderingUnit.LightsBuffer, 0, sizeof(BlinFongLightsInfo), &lightsInfo);//Update lights information
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
