#ifndef SHADER_LOAD_UTIL_INCLUDED
#define SHADER_LOAD_UTIL_INCLUDED

#include <string>

namespace dengine
{
	std::pmr::string loadShaderFromFile(const std::pmr::string& filePath);
	unsigned int uploadAndCompileShaders(const char* vertexPath, const char* fragmentPath);
}
#endif
