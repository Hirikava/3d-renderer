#include <rendering/schemas/blin_fong_rendering_scheme.h>

#include <sstream>

#include <utils/shader_load_utils.h>
#include <glad/glad.h>


//ATTRIBUTE BINDINGS
#define ATTRIBUTE_POSITION_LOCATION 0
#define ATTRIBUTE_POSITION_BINDING 0
#define ATTRIBUTE_NORMAL_LOCATION 1
#define ATTRIBUTE_NORMAL_BINDING 1
#define ATTRIBUTE_UV_LOCATION 2
#define ATTRIBUTE_UV_BINDING 2

//UNIFORM BUFFER BINDINGS
#define UNIFORM_ENVIRONMENT_BLOCK_BINDING 0
#define UNIFROM_ENVIRONMENT_BLOCK_INDEX 0
//SHADER STORAGE BUFFER BINDINGS
#define SHADER_STORAGE_LIGHTS_BINDING 0
#define SHADER_STORAGE_lIGHTS_INDEX 0


unsigned dengine::BlinFongRenderingScheme::LoadShaderProgram()
{
	auto program = uploadAndCompileShaders("shaders/blin-fong.vert", "shaders/blin-fong.frag");
	glUniformBlockBinding(program, UNIFORM_ENVIRONMENT_BLOCK_BINDING, UNIFROM_ENVIRONMENT_BLOCK_INDEX);
	glShaderStorageBlockBinding(program, SHADER_STORAGE_LIGHTS_BINDING, SHADER_STORAGE_lIGHTS_INDEX);
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
	glVertexArrayVertexBuffer(vao, ATTRIBUTE_POSITION_BINDING, vbo, positionVertexLayout.Offset,
		positionVertexLayout.Stride);
	glVertexArrayAttribFormat(vao, ATTRIBUTE_POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao, ATTRIBUTE_POSITION_LOCATION, ATTRIBUTE_POSITION_BINDING);
	glEnableVertexArrayAttrib(vao, ATTRIBUTE_POSITION_LOCATION);
	//Uvs binding
	auto uvVertexLayout = mesh.GetVertexAttributeLayout(UVs);
	glVertexArrayVertexBuffer(vao, ATTRIBUTE_NORMAL_BINDING, vbo, uvVertexLayout.Offset, uvVertexLayout.Stride);
	glVertexArrayAttribFormat(vao, ATTRIBUTE_NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao, ATTRIBUTE_NORMAL_LOCATION, ATTRIBUTE_NORMAL_BINDING);
	glEnableVertexArrayAttrib(vao, ATTRIBUTE_NORMAL_LOCATION);

	//Bind instance buffer
	glVertexArrayVertexBuffer(vao, ATTRIBUTE_UV_BINDING, instanceBuffer, 0, sizeof(glm::mat4));
	for (int i = 0; i < 4; i++)
	{
		glVertexArrayAttribFormat(vao, ATTRIBUTE_UV_LOCATION + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4) * i);
		glVertexArrayAttribBinding(vao, ATTRIBUTE_UV_LOCATION + i, ATTRIBUTE_UV_BINDING);
		glEnableVertexArrayAttrib(vao, ATTRIBUTE_UV_LOCATION + i);
		glVertexArrayBindingDivisor(vao, ATTRIBUTE_UV_LOCATION + i, 1);
	}

	glVertexArrayElementBuffer(vao, mesh.Ebo);
	glBindBufferBase(GL_UNIFORM_BUFFER, UNIFORM_ENVIRONMENT_BLOCK_BINDING, environmentBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SHADER_STORAGE_LIGHTS_BINDING, environmentBuffer);
	glBindVertexArray(0);

	return BlinFongRenderingUnit{ vao, mesh.NumElements, instanceBuffer,  environmentBuffer };
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
		instancedToDraw[cacheId] = { renderingUnit, submitInfo };
	}
	auto& drawInstance = instancedToDraw[cacheId];
	drawInstance.second.InstanceDatas.push_back(BlinFongInstanceData{ modelMatrix });
}


void dengine::BlinFongRenderingSubmiter::DispatchDrawCall(unsigned programId, const GlobalEnvironment& environment) const
{
	glUseProgram(programId);

	BlinFongLightsInfo lightsInfo;
	lightsInfo.Info.Count = environment.LightsPositions.size();
	memcpy(lightsInfo.LightsPositions, &environment.LightsPositions[0], environment.LightsPositions.size() * sizeof(glm::vec4));

	for (auto& index : instancedToDraw)
	{
		auto& submitInfo = index.second.second;
		auto& renderingUnit = index.second.first;

		glBindTextureUnit(0, index.second.second.DiffuseTexture);
		//Update draw info
		BlinFongEnvironmentData blinFongEnvironmentData{ environment.CameraPostion, environment.ProjectionMatrix, environment.ViewMatrix };
		glNamedBufferSubData(renderingUnit.EnvironmentBuffer, 0, sizeof(BlinFongEnvironmentData), &blinFongEnvironmentData);
		//Update global environment
		glNamedBufferSubData(renderingUnit.InstaciesBuffer, 0,
			sizeof(BlinFongInstanceData) * submitInfo.InstanceDatas.size(),
			&submitInfo.InstanceDatas[0]); //Update model matricies
		glNamedBufferSubData(renderingUnit.LightsBuffer, 0,
			sizeof(BlinFongLightsInfo),
			&lightsInfo); //Update lights information
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
