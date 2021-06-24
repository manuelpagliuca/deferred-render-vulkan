#include "pch.h"
#include "Window.h"

Window::Window()
{
	m_WindowWidth = 800;
	m_WindowHeight = 600;
	m_BufferWidth = 0;
	m_BufferHeight = 0;
	mainWindow = NULL;

	keys[0] = {};
	for (size_t i = 0; i < 1024; i++)
	{
		keys[i] = false;
	}
	mouseFirstMoved = true;

	lastX = 0.0f;
	lastY = 0.0f;
	xChange = 0.0f;
	yChange = 0.0f;
}

Window::Window(const int width, const int height, const char* title)
{
	m_WindowWidth	= width;
	m_WindowHeight	= height;
	m_WindowName	= title;
	m_BufferWidth	= 0;
	m_BufferHeight	= 0;
	mainWindow		= NULL;

	keys[0] = {};

	for (size_t i = 0; i < 1024; i++)
	{
		keys[i] = false;
	}

	mouseFirstMoved = true;
	lastX	= 0.0f;
	lastY	= 0.0f;
	xChange = 0.0f;
	yChange = 0.0f;
}

int Window::Initialise()
{
	const int res = glfwInit();

	if (!glfwVulkanSupported())
	{
		std::cerr << "GLFW: Vulkan not supported\n" << std::endl;
		return -1;
	}

	if (!res)
	{
		throw std::runtime_error("Unable to initialize GLFW.");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Impostare GLFW in modo che non lavori con OpenGL
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);	  // Disabilito la resize (va gestita in un suo modo specifico su Vulkan)

	mainWindow = glfwCreateWindow(m_WindowWidth, m_WindowHeight, m_WindowName, nullptr, nullptr);

	if (!mainWindow)
	{
		fprintf(stderr, "Error creating the window, Window.cpp line: %d\n", __LINE__);
		glfwTerminate();
		return -1;
	}

	glfwGetFramebufferSize(mainWindow, &m_BufferWidth, &m_BufferHeight);
	glfwMakeContextCurrent(mainWindow);

	createCallBacks();

	//glViewport(0, 0, m_BufferWidth, m_BufferHeight);

	glfwSetWindowUserPointer(mainWindow, this);

	centerMonitor(mainWindow, glfwGetPrimaryMonitor());
	return GLFW_NO_ERROR;
}

void Window::centerMonitor(GLFWwindow* mainWindow, GLFWmonitor* monitor)
{
	if (!monitor)
		return;
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);

	if (!mode)
		return;

	int monitorX, monitorY;

	glfwGetMonitorPos(monitor, &monitorX, &monitorY);

	glfwSetWindowPos(mainWindow, monitorX + (mode->width - m_WindowWidth) / 2, monitorY + (mode->height - m_WindowHeight) / 2);
}

void Window::createCallBacks()
{
	glfwSetKeyCallback(mainWindow, handleKeys);
	glfwSetCursorPosCallback(mainWindow, handleMouse);
}

void Window::handleKeys(GLFWwindow* window, int key, int code, int action, int mode)
{
	Window* theWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
		{
			theWindow->keys[key] = true;
			std::cout << "key: " << key << std::endl;
		}
		else if (action == GLFW_RELEASE)
		{
			theWindow->keys[key] = false;
		}
	}
}

void Window::handleMouse(GLFWwindow* window, double xPos, double yPos)
{
	Window* theWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));

	if (theWindow->mouseFirstMoved)
	{
		theWindow->lastX = static_cast<float>(xPos);
		theWindow->lastY = static_cast<float>(yPos);
		theWindow->mouseFirstMoved = false;
	}

	theWindow->xChange = static_cast<float>(xPos) - theWindow->lastX;
	theWindow->yChange = theWindow->lastY - static_cast<float>(yPos);

	theWindow->lastX = static_cast<float>(xPos);
	theWindow->lastY = static_cast<float>(yPos);
}

float Window::getXChange()
{
	const float theChange = xChange;
	xChange = 0.0f;
	return theChange;
}

float Window::getYChange()
{
	const float theChange = yChange;
	yChange = 0.0f;
	return theChange;
}

Window::~Window()
{
	glfwDestroyWindow(mainWindow);
	glfwTerminate();
}