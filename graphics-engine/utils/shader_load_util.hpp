#ifndef SHADER_LOAD_UTIL_INCLUDED
#define SHADER_LOAD_UTIL_INCLUDED

#include <string>
#include <fstream>
#include <sstream>

namespace dengine
{
	std::pmr::string loadShaderFromFile(const std::pmr::string& filePath)
	{
		std::fstream fstream;
		fstream.open(filePath.c_str(), std::fstream::binary | std::fstream::in);
		std::stringstream ss;
		ss << fstream.rdbuf();
		fstream.close();
		return std::pmr::string(ss.str());
	}
}
#endif
