#include <graphics-engine/application/graphics_engine_application.h>

int main(char* argc, char* argv[])
{
	dengine::GraphicsEngineRunArguments arguments{
	argv[1]
	};

	dengine::GraphicsEngineApplication application;
	return application.Run(arguments);
}