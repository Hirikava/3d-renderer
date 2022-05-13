#ifndef GRAPHICS_ENGINE_APPLICATION_INCLUDED
#define GRAPHICS_ENGINE_APPLICATION_INCLUDED

#include <glad/glad.h>
#include <importers/assimp_model_importer.h>
#include <entt/entt.hpp>
#include <GLFW/glfw3.h>

namespace dengine
{

	template<typename TRunArguments>
	class IApplication
	{
	public:
		virtual ~IApplication() = default;
		int Run(TRunArguments& runArguments)
		{
			try
			{
				if (!Initialize())
				{
					Terminate();
					return -1;
				}
				return this->RunInternal(runArguments);
			}
			catch (std::exception exception)
			{
				Terminate();
			}
			Terminate();
			return -1;
			
		}
	protected:
		virtual bool Initialize() = 0;
		virtual bool Terminate() = 0;
		virtual int RunInternal(TRunArguments& runArguments) = 0;
	};

	struct GraphicsEngineRunArguments
	{
		std::pmr::string pathToModel;
	};


	class GraphicsEngineApplication : public IApplication<GraphicsEngineRunArguments>
	{
	public:
		GraphicsEngineApplication() = default;
	protected:
		bool Terminate() override;
		int RunInternal(GraphicsEngineRunArguments& arguments) override;
		bool Initialize() override;
	private:
		GLFWwindow* window;
		AssimpModelImporter modelImporter;
		entt::registry registry;
	};
}


#endif

