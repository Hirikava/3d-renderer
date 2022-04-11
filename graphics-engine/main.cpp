#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

int main(char* argc, char* argv[])
{

	if (!glfwInit())
		return -1;

	GLFWwindow* window = glfwCreateWindow(800, 600, "Diplom", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
		std::cout << "Failed to initialize OpenGL context" << std::endl;
		return -1;
	}

	glClearColor(1.0f, 1.0f, 0.0f, 0.0f);
	while(!glfwWindowShouldClose(window))
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	std::cout << "Hello, world!" << std::endl;
	return 0;
}