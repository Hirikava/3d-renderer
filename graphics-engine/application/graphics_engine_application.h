#ifndef GRAPHICS_ENGINE_APPLICATION_INCLUDED
#define GRAPHICS_ENGINE_APPLICATION_INCLUDED

#include <glad/glad.h>
#include <importers/assimp_model_importer.h>
#include <entt/entt.hpp>
#include <GLFW/glfw3.h>

namespace dengine
{
	class IApplication
	{
	public:
		virtual ~IApplication() = default;
		int Run();
	protected:
		virtual bool Initialize() = 0;
		virtual bool Terminate() = 0;
		virtual int RunInternal() = 0;
	};


	class GraphicsEngineApplication : public IApplication
	{
	public:
		GraphicsEngineApplication() = default;
	protected:
		bool Terminate() override;
		int RunInternal() override;
		bool Initialize() override;
	private:
		GLFWwindow* window;
		AssimpModelImporter modelImporter;
		entt::registry registry;
	};
}


#endif

