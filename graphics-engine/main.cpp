#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>

#include <importers/assimp_model_importer.h>
#include <rendering/rendering_tmp.hpp>
#include <rendering/camera.hpp>
#include <rendering/global_environment.hpp>




std::pmr::string loadShaderFromFile(const std::pmr::string& filePath)
{
	std::fstream fstream;
	fstream.open(filePath.c_str(), std::fstream::binary | std::fstream::in);
	std::stringstream ss;
	ss << fstream.rdbuf();
	fstream.close();
	return std::pmr::string(ss.str());
}

void GLAPIENTRY
MessageCallback(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
		(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
		type, severity, message);
}

float cameraSpeed = 7.5f;
float cameraRotationSpeed = 0.05f;
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




int main(char* argc, char* argv[])
{

	dengine::AssimpModelImporter modelImporter;
	auto model = modelImporter.Import("C:\\Users\\TheDAX\\Desktop\\models\\blossom_katana\\scene.gltf");

	//Init GLFW
	if (!glfwInit())
		return -1;

	//CreateWindow
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(1920, 1200, "Diplom", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// During init, enable debug output
	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
		std::cout << "Failed to initialize OpenGL context" << std::endl;
		return -1;
	}

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);

	//Load model from disk space

	auto openglModel = dengine::loadModelToGpu(model);
	model.Materials.clear();
	model.Meshes.clear();
	model.Textures.clear();


	//set up global environment
	dengine::GlobalEnvironment globalEnvironment;
	unsigned int globalEnvironmentUbo;
	glCreateBuffers(1, &globalEnvironmentUbo);
	glNamedBufferData(globalEnvironmentUbo, sizeof(dengine::GlobalEnvironment), nullptr, GL_STREAM_DRAW);

	//make instance buffer
	unsigned int instanceBuffer;
	glm::mat4 modelMatrix{ 1.0f };
	glCreateBuffers(1, &instanceBuffer);
	glNamedBufferData(instanceBuffer, sizeof(glm::mat4), glm::value_ptr(modelMatrix), GL_STREAM_DRAW);

	//make vaos
	struct RenderingUnit{
		unsigned int Vao;
		unsigned int DiffuseTexture;
		unsigned long long ElementsCount;
	};
	std::pmr::vector<RenderingUnit> renderingUnits;
	for(int i = 0; i < openglModel.Meshes.size(); i++)
	{
		auto& openglMesh = openglModel.Meshes[i];
		unsigned int vbo = openglMesh.Vbo;
		unsigned int vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		//Positions vertex layout
		auto positionVertexLayout = openglMesh.GetVertexAttributeLayout(dengine::Positions);
		glVertexArrayVertexBuffer(vao, 0, vbo, positionVertexLayout.Offset, positionVertexLayout.Stride);
		glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
		glVertexArrayAttribBinding(vao, 0, 0);
		glEnableVertexArrayAttrib(vao, 0);
		//Uvs binding
		auto uvVertexLayout = openglMesh.GetVertexAttributeLayout(dengine::UVs);
		glVertexArrayVertexBuffer(vao, 1, vbo, uvVertexLayout.Offset, uvVertexLayout.Stride);
		glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, 0);
		glVertexArrayAttribBinding(vao, 1, 1);
		glEnableVertexArrayAttrib(vao, 1);

		//Bind instance buffer
		glVertexArrayVertexBuffer(vao, 2, instanceBuffer, 0, 0);
		for(int i = 0; i < 4; i++)
		{
			glVertexArrayAttribFormat(vao, 2 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4) * i);
			glVertexArrayAttribBinding(vao, 2 + i, 2);
			glEnableVertexArrayAttrib(vao, 2 + i);
			glVertexArrayBindingDivisor(vao, 2 + i, 1);
		}


		glVertexArrayElementBuffer(vao, openglMesh.Ebo);
		glBindBufferRange(GL_UNIFORM_BUFFER, 0, globalEnvironmentUbo, 0, 144);
		glBindVertexArray(0);
		renderingUnits.push_back(RenderingUnit{ vao, openglModel.Materils[openglMesh.MaterialIndex].DiffuseTextureId, openglMesh.NumElements });
	}

	//load shader program and compile it
	auto vertexShaderSource = loadShaderFromFile("C:\\Users\\TheDAX\\Desktop\\diplom\\graphics-engine\\rendering\\shaders\\simple_3d.vert");
	auto fragmentShaderSource = loadShaderFromFile("C:\\Users\\TheDAX\\Desktop\\diplom\\graphics-engine\\rendering\\shaders\\simple_3d.frag");
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
	glUniformBlockBinding(program, 0, 0);
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

	GLenum drawBuffs[1] = { GL_COLOR_ATTACHMENT0 };
	glNamedFramebufferDrawBuffers(fbo,1,drawBuffs);





	//Init ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");

	dengine::Camera camera{glm::vec3(10.0f,10.0f,0.0f), glm::vec3(-1,-1,0), glm::vec3(0,1,0)};

	float color[3];
	float time = glfwGetTime();
	ImVec2 currentViewportSize(1920, 1080);
	ImVec2 tempViewPortSize(1920, 1080);
	while(!glfwWindowShouldClose(window))
	{
		float newTime = glfwGetTime();
		float dTime = newTime - time;
		time = newTime;
		
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if(tempViewPortSize.x != currentViewportSize.x || currentViewportSize.y != tempViewPortSize.y)
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
		glClearColor(color[0], color[1], color[2], 0.0f); 
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);


		UpdateCamera(camera, dTime);

		int width, height;
		glfwGetWindowSize(window, &width, &height);
		auto aspect = static_cast<float>(width) / static_cast<float>(height);
		globalEnvironment.CameraPostion = glm::vec4(camera.Position, 1.0f);
		globalEnvironment.ProjectionMatrix = glm::perspective(glm::degrees(45.0f), aspect, 0.01f, 100.0f);
		globalEnvironment.ViewMatrix = dengine::CameraControl::GetLookAtMatrix(camera);
		glNamedBufferSubData(globalEnvironmentUbo, 0, sizeof(dengine::GlobalEnvironment), &globalEnvironment);

		glUseProgram(program);
		for (auto renderingUnit : renderingUnits)
		{
			glBindVertexArray(renderingUnit.Vao);
			glBindTextureUnit(0, renderingUnit.DiffuseTexture);
			glDrawElementsInstanced(GL_TRIANGLES, renderingUnit.ElementsCount, GL_UNSIGNED_INT, nullptr, 1);
		}

		//swap to default framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);



		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		bool open = true;
		ImGui::Begin("Global", &open, window_flags);
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));
		ImGui::End();


		ImGui::Begin("New world!");

		//Start New ImGui frame
		ImGui::ColorPicker3("color", color);
		ImGui::DragFloat("CameraSpeed", &cameraSpeed,1.0f,0,50);
		ImGui::DragFloat("CameraRotationSpeed", &cameraRotationSpeed);
		ImGui::End();

		auto windowFlags = ImGuiWindowFlags_NoScrollbar;
		ImGui::Begin("Viewport", &open, windowFlags);
		tempViewPortSize = ImGui::GetWindowSize();
		std::cout << tempViewPortSize.x << " " << tempViewPortSize.y << std::endl;
		ImGui::Image((void*)(intptr_t)colorAttachmentTexture, ImVec2(currentViewportSize.x, currentViewportSize.y), 
			ImVec2(0, 1), ImVec2(1, 0));
		ImGui::End();
		//Render ImGui frame
		ImGui::Render();

		//Submit command to GPU(need bound opengl context)
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	//Terminate ImGui
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	//Terminate GLFW
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}