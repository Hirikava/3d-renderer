#include <iostream>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <importers/assimp_model_importer.h>


int main(char* argc, char* argv[])
{

	dengine::AssimpModelImporter modelImporter;
	auto model = modelImporter.Import("C:\\Users\\TheDAX\\Desktop\\models\\123\\Survival_BackPack_2.fbx");

	//Init GLFW
	if (!glfwInit())
		return -1;

	//CreateWindow
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	GLFWwindow* window = glfwCreateWindow(800, 600, "Diplom", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
		std::cout << "Failed to initialize OpenGL context" << std::endl;
		return -1;
	}

	//Init ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");

	float color[3];
	while(!glfwWindowShouldClose(window))
	{
		glClearColor(color[0], color[1], color[2], 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		//Start New ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::ColorPicker3("color", color);

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