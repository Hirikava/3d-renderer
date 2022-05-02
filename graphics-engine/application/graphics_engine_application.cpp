#include <graphics-engine/application/graphics_engine_application.h>

//stl
#include <exception>
#include <fstream>

//deps
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glm/gtc/type_ptr.hpp>
//logging
#include <spdlog/spdlog.h>

//rendering
#include <rendering/rendering_tmp.h>
#include <rendering/camera.hpp>
#include <rendering/global_environment.h>

#include <rendering/schemas/blin_fong_rendering_scheme.h>

const char* OpenGlLoggerName = "opengl_logger";
const char* AppLoggerName = "app_logger";


int dengine::IApplication::Run()
{
	try
	{
		if (!Initialize())
		{
			Terminate();
			return -1;
		}
		return this->RunInternal();
	}
	catch (std::exception exception)
	{
		Terminate();
	}
	Terminate();
	return -1;
}


bool dengine::GraphicsEngineApplication::Terminate()
{
	//Terminate ImGui
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	//Terminate GLFW
	if (window != nullptr)
		glfwDestroyWindow(window);
	glfwTerminate();
	return true;
}


struct TransformComponent{
	glm::mat4 ModelMatrix;
};


void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                const GLchar* message, const void* userParam)
{
	auto openglLogger = spdlog::get(OpenGlLoggerName);
	openglLogger->info("GL CALLBACK: {} type = {}, severity = {}, message = {}\n",
	                   (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
}


float cameraSpeed = 7.5f;
float cameraRotationSpeed = 0.0005f;


void UpdateCamera(dengine::Camera& cam, float dTime)
{
	dengine::Camera camera = cam;
	if (ImGui::IsKeyDown(ImGuiKey_A))
		dengine::CameraControl::MoveCamera(cam, dTime * cameraSpeed, dengine::MoveDiraction::Left);
	if (ImGui::IsKeyDown(ImGuiKey_D))
		dengine::CameraControl::MoveCamera(cam, dTime * cameraSpeed, dengine::MoveDiraction::Right);
	if (ImGui::IsKeyDown(ImGuiKey_W))
		dengine::CameraControl::MoveCamera(cam, dTime * cameraSpeed, dengine::MoveDiraction::Forward);
	if (ImGui::IsKeyDown(ImGuiKey_S))
		dengine::CameraControl::MoveCamera(cam, dTime * cameraSpeed, dengine::MoveDiraction::Backwards);
}


void UpdateRotateCamera(dengine::Camera& cam, float xPitch, float yYaw)
{
	xPitch *= cameraRotationSpeed / 2;
	yYaw *= -cameraRotationSpeed / 2;
	const glm::vec3 rotationVecRight = normalize(cross(cam.Diraction, glm::vec3(0, 1, 0)));
	const glm::vec3 rotationVecUp = normalize(cross(cam.Diraction, rotationVecRight));
	auto rotationQuatY = glm::quat(glm::cos(yYaw), rotationVecRight * glm::sin(yYaw));
	auto rotationQuatX = glm::quat(glm::cos(xPitch), rotationVecUp * glm::sin(xPitch));
	glm::quat rotationQuat = normalize(rotationQuatY * rotationQuatX);
	auto dirQuat = rotationQuat * cam.Diraction * conjugate(rotationQuat);
	cam.Diraction = normalize(glm::vec3(dirQuat.x, dirQuat.y, dirQuat.z));
}


int dengine::GraphicsEngineApplication::RunInternal()
{
	//load models
	auto model = modelImporter.Import("C:\\Users\\TheDAX\\Desktop\\models\\blossom_katana\\scene.gltf");
	auto openglModel = loadModelToGpu(model);
	model.Materials.clear();
	model.Meshes.clear();
	model.Textures.clear();

	//set up global environment
	GlobalEnvironment globalEnvironment;
	globalEnvironment.LightsPositions.push_back(glm::vec4(-10, -10, -10, 0));
	glm::mat4 modelMatrix{1.0f};

	//make vaos
	for (int i = 0; i < openglModel.Meshes.size(); i++)
	{
		auto simpleRenderinUnit = BlinFongRenderingScheme::CreateRenderingUnit(openglModel.Meshes[i]);
		auto entity = registry.create();
		registry.emplace<BlinFongRenderingUnit>(entity, simpleRenderinUnit);
		registry.emplace<TransformComponent>(entity, modelMatrix);
		registry.emplace<Material>(entity, Material{ openglModel.Materils[openglModel.Meshes[i].MaterialIndex].DiffuseTextureId });
	}

	//load shader program and compile it
	BlinFongRenderingScheme simpleRenderingScheme;
	auto program = simpleRenderingScheme.LoadShaderProgram();


	glEnable(GL_DEPTH_TEST);

	//CreateRenderBuffer
	unsigned int fbo, depthTexture, colorAttachmentTexture;
	glCreateFramebuffers(1, &fbo);
	glCreateTextures(GL_TEXTURE_2D, 1, &depthTexture);
	glCreateTextures(GL_TEXTURE_2D, 1, &colorAttachmentTexture);
	glTextureStorage2D(depthTexture, 1, GL_DEPTH24_STENCIL8, 1920, 1080);
	glTextureStorage2D(colorAttachmentTexture, 1, GL_RGBA8, 1920, 1080);
	glNamedFramebufferTexture(fbo, GL_DEPTH_ATTACHMENT, depthTexture, 0);
	glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, colorAttachmentTexture, 0);

	GLenum drawBuffs[1] = {GL_COLOR_ATTACHMENT0};
	glNamedFramebufferDrawBuffers(fbo, 1, drawBuffs);

	Camera camera{glm::vec3(10.0f, 10.0f, 0.0f), glm::vec3(-1, -1, 0), glm::vec3(0, 1, 0)};

	float color[3] = {0, 0, 0};
	float time = glfwGetTime();
	ImVec2 currentViewportSize(1920, 1080);
	ImVec2 tempViewPortSize(1920, 1080);


	BlinFongRenderingSubmiter renderingSubmitter;

	bool useDiffuseTexture = true;
	while (!glfwWindowShouldClose(window))
	{
		float newTime = glfwGetTime();
		float dTime = newTime - time;
		time = newTime;

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (tempViewPortSize.x != currentViewportSize.x || currentViewportSize.y != tempViewPortSize.y)
		{
			glDeleteTextures(1, &colorAttachmentTexture);
			glDeleteTextures(1, &depthTexture);
			glCreateTextures(GL_TEXTURE_2D, 1, &depthTexture);
			glCreateTextures(GL_TEXTURE_2D, 1, &colorAttachmentTexture);
			glTextureStorage2D(depthTexture, 1, GL_DEPTH24_STENCIL8, tempViewPortSize.x, tempViewPortSize.y);
			glTextureStorage2D(colorAttachmentTexture, 1, GL_RGBA8, tempViewPortSize.x, tempViewPortSize.y);
			glNamedFramebufferTexture(fbo, GL_DEPTH_ATTACHMENT, depthTexture, 0);
			glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, colorAttachmentTexture, 0);
			currentViewportSize = tempViewPortSize;
		}


		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glClearColor(color[0], color[1], color[2], 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		auto delta = ImGui::GetIO().MouseDelta;

		int width, height;
		glfwGetWindowSize(window, &width, &height);
		auto aspect = currentViewportSize.x / currentViewportSize.y;
		globalEnvironment.CameraPostion = glm::vec4(camera.Position, 1.0f);
		globalEnvironment.ProjectionMatrix = glm::perspective(glm::degrees(45.0f), aspect, 0.01f, 100.0f);
		globalEnvironment.ViewMatrix = CameraControl::GetLookAtMatrix(camera);

		auto view = registry.view<BlinFongRenderingUnit, TransformComponent, Material>();
		for (auto entity : view)
		{
			auto renderingUnit = view.get<BlinFongRenderingUnit>(entity);
			auto material = view.get<Material>(entity);
			auto modelMatrix1 = glm::mat4(1.0f);
			auto modelMatrix2 = translate(glm::mat4(1.0f), glm::vec3(4.0f, 4.0f, 0));
			renderingSubmitter.Submit(renderingUnit, material, modelMatrix1);
			renderingSubmitter.Submit(renderingUnit, material, modelMatrix2);
		}

		renderingSubmitter.DispatchDrawCall(program, globalEnvironment);
		renderingSubmitter.Clear();

		//swap to default framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		bool open = true;
		ImGui::Begin("Global", &open, window_flags);
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));
		ImGui::End();


		ImGui::Begin("New world!");

		//Start New ImGui frame
		ImGui::ColorPicker3("color", color);
		ImGui::DragFloat("CameraSpeed", &cameraSpeed, 1.0f, 0, 50);
		ImGui::DragFloat("CameraRotationSpeed", &cameraRotationSpeed, 0.0001f, 0, 1);

		auto windowFlags = ImGuiWindowFlags_NoScrollbar;
		ImGui::Begin("Viewport", &open, windowFlags);
		tempViewPortSize = ImGui::GetWindowSize();
		if (ImGui::IsWindowFocused())
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			UpdateCamera(camera, dTime);
			UpdateRotateCamera(camera, delta.x, delta.y);
		}
		else
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
		ImGui::Image((void*)static_cast<intptr_t>(colorAttachmentTexture),
		             ImVec2(currentViewportSize.x, currentViewportSize.y), ImVec2(0, 1), ImVec2(1, 0));
		ImGui::End();
		//Render ImGui frame
		ImGui::Render();

		//Submit command to GPU(need bound opengl context)
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	return 0;
}


void glfwErrorCallback(int code, const char* errorMessgae)
{
	auto logger = spdlog::get(AppLoggerName);
	logger->critical(errorMessgae);
}


bool dengine::GraphicsEngineApplication::Initialize()
{
	//Enable logger
	auto logger = spdlog::basic_logger_mt(OpenGlLoggerName, "opengl-logs.txt", true);
	auto applicationLogger = spdlog::basic_logger_mt(AppLoggerName, "app-logs.txt", true);
	//CreateWindow
	glfwSetErrorCallback(glfwErrorCallback);
	if (!glfwInit())
	{
		applicationLogger->critical("Failed to initialize GLFW, check above log for more information");
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(1920, 1200, "graphics-engine", nullptr, nullptr);
	if (window == nullptr)
	{
		applicationLogger->critical("Failed to create a window, check above log for more information");
		return false;
	}
	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
	{
		applicationLogger->critical("Failed to initialize OpenGL context");
		return false;
	}
	// During init, enable debug output
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, nullptr);

	//Init ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");
	return true;
}
