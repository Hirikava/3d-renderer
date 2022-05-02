#include <utils/shader_load_utils.h>
#include <glad/glad.h>

#include <fstream>
#include <sstream>


std::pmr::string dengine::loadShaderFromFile(const std::pmr::string& filePath)
{
	std::fstream fstream;
	fstream.open(filePath.c_str(), std::fstream::binary | std::fstream::in);
	std::stringstream ss;
	ss << fstream.rdbuf();
	fstream.close();
	return std::pmr::string(ss.str());
}


unsigned int dengine::uploadAndCompileShaders(const char* vertexPath, const char* fragmentPath)
{
	auto vertexShaderSource = loadShaderFromFile(vertexPath);
	auto fragmentShaderSource = loadShaderFromFile(fragmentPath);

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

	return program;
}
