#include <rendering/schemas/simple_rendering_scheme.h>

#include <glad/glad.h>
#include <utils/shader_load_util.hpp>

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
#define SHADER_STORAGE_MATERIAL_BINDING 0
#define SHADER_STORAGE_MATERIAL_INDEX 0

unsigned dengine::SimpleRenderingScheme::LoadShaderProgram()
{
	auto vertexShaderSource = loadShaderFromFile("shaders/simple.vert");
	auto fragmentShaderSource = loadShaderFromFile("shaders/simple.frag");

	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	unsigned int program = glCreateProgram();
	const char* vertexShaderSourcePtr = vertexShaderSource.c_str();
	const char* fragmentShaderSourcePtr = fragmentShaderSource.c_str();
	glShaderSource(vertexShader, 1, &vertexShaderSourcePtr, nullptr);
	glShaderSource(fragmentShader, 1, &fragmentShaderSourcePtr, nullptr);
	glCompileShader(vertexShader);
	glCompileShader(fragmentShader);

	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	glDetachShader(program, vertexShader);
	glDetachShader(program, fragmentShader);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);


	glUniformBlockBinding(program, UNIFORM_ENVIRONMENT_BLOCK_BINDING, UNIFROM_ENVIRONMENT_BLOCK_INDEX);
	glShaderStorageBlockBinding(program, SHADER_STORAGE_MATERIAL_INDEX, SHADER_STORAGE_MATERIAL_BINDING);
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
	auto positionVertexLayout = mesh.GetVertexAttributeLayout(dengine::Positions);
	glVertexArrayVertexBuffer(vao, ATTRIBUTE_POSITION_BINDING, vbo, positionVertexLayout.Offset, positionVertexLayout.Stride);
	glVertexArrayAttribFormat(vao, ATTRIBUTE_POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao, ATTRIBUTE_POSITION_LOCATION, ATTRIBUTE_POSITION_BINDING);
	glEnableVertexArrayAttrib(vao, ATTRIBUTE_POSITION_LOCATION);
	//Uvs binding
	auto uvVertexLayout = mesh.GetVertexAttributeLayout(dengine::UVs);
	glVertexArrayVertexBuffer(vao, ATTRIBUTE_NORMAL_BINDING, vbo, uvVertexLayout.Offset, uvVertexLayout.Stride);
	glVertexArrayAttribFormat(vao, ATTRIBUTE_NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao, ATTRIBUTE_NORMAL_LOCATION, ATTRIBUTE_NORMAL_BINDING);
	glEnableVertexArrayAttrib(vao, ATTRIBUTE_NORMAL_LOCATION);

	//Bind instance buffer
	glVertexArrayVertexBuffer(vao, ATTRIBUTE_NORMAL_BINDING, instanceBuffer, 0, sizeof(glm::mat4));
	for (int i = 0; i < 4; i++)
	{
		glVertexArrayAttribFormat(vao, ATTRIBUTE_UV_LOCATION + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4) * i);
		glVertexArrayAttribBinding(vao, ATTRIBUTE_UV_LOCATION + i, ATTRIBUTE_UV_BINDING);
		glEnableVertexArrayAttrib(vao, ATTRIBUTE_UV_LOCATION + i);
		glVertexArrayBindingDivisor(vao, ATTRIBUTE_UV_LOCATION + i, 1);
	}

	glVertexArrayElementBuffer(vao, mesh.Ebo);
	glBindBufferBase(GL_UNIFORM_BUFFER, UNIFORM_ENVIRONMENT_BLOCK_BINDING, environmentBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SHADER_STORAGE_MATERIAL_BINDING, materialsBuffer);
	glBindVertexArray(0);

	return SimpleRenderingUnit{
		vao,
		mesh.NumElements,
		instanceBuffer,
		materialsBuffer,
		environmentBuffer
	};
}


void dengine::SimpleRedneringSubmitter::Submit(SimpleRenderingUnit renderingUnit, glm::mat4 modelMatrxi)
{
	auto findIter = instancedToDraw.find(renderingUnit.Vao);
	if (findIter == instancedToDraw.end())
		instancedToDraw[renderingUnit.Vao] ={renderingUnit,SimpleSubmitInfo{}};
	auto& drawInstance = instancedToDraw[renderingUnit.Vao];
	drawInstance.second.SimpleInstanceData.push_back(SimpleInstanceData{ modelMatrxi });
	SimpleMaterialData simpleMaterialData{};
	simpleMaterialData.BaseColor = glm::vec4(1.0f, 0.0f,0.0f,1.0f);
	simpleMaterialData.ColorSelector.Index = 1;
	drawInstance.second.SimpleMaterialData.push_back(simpleMaterialData);
}


void dengine::SimpleRedneringSubmitter::DispatchDrawCall(unsigned programId, GlobalEnvironment environment) const
{
	glUseProgram(programId);
	GLuint indices[2] = { 0,1 };
	glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 2, indices);

	for (auto& index : instancedToDraw)
	{
		auto& submitInfo = index.second.second;
		auto& renderingUnit = index.second.first;

		//Update draw info
		SimpleEnvironmentData simpleEnvironmentData{environment.ProjectionMatrix, environment.ViewMatrix};
		glNamedBufferSubData(renderingUnit.EnvironmentBuffer, 0, sizeof(SimpleEnvironmentData), &simpleEnvironmentData); 	//Update global environment
		glNamedBufferSubData(renderingUnit.InstaciesBuffer, 0, sizeof(SimpleInstanceData) * submitInfo.SimpleInstanceData.size(), &submitInfo.SimpleInstanceData[0]);//Update model matricies
		glNamedBufferSubData(renderingUnit.MaterialsBuffer, 0, sizeof(SimpleMaterialData) * submitInfo.SimpleMaterialData.size(), &submitInfo.SimpleMaterialData[0]);//Update model materials
		glDrawElementsInstanced(GL_TRIANGLES, renderingUnit.IndeciesSize, GL_UNSIGNED_INT, nullptr, submitInfo.SimpleInstanceData.size());
	}
}


void dengine::SimpleRedneringSubmitter::Clear()
{
	for (auto& index : instancedToDraw)
	{
		auto& submitInfo = index.second.second;
		submitInfo.SimpleInstanceData.clear();
		submitInfo.SimpleInstanceData.clear();
	}
}

