#pragma once

#include <GLFW/glfw3.h>

class Window
{
public:
	// Constructors
	Window();
	Window(const int width, const int height, const char* title);

	// Methods
	int Initialise();
	void centerMonitor(GLFWwindow* mainWindow, GLFWmonitor* monitor);
	int getShouldClose() { return glfwWindowShouldClose(mainWindow); }
	GLFWwindow* getWindow() { return mainWindow; }
	bool* getsKeys() { return keys; }
	float getXChange();
	float getYChange();
	void swapBuffers() { return glfwSwapBuffers(mainWindow); }

	// Getters
	int getBufferWidth() { return m_BufferWidth; }
	int getBufferHeight() { return m_BufferHeight; }
	float getFrameAspectRatio() { return static_cast<float>(m_BufferWidth) / static_cast<float>(m_BufferHeight); }

	// Destructor
	~Window();

private:
	int m_WindowWidth, m_WindowHeight;
	const char* m_WindowName;
	int m_BufferWidth, m_BufferHeight;
	bool keys[1024], mouseFirstMoved;
	float lastX, lastY, xChange, yChange;

	static void handleKeys(GLFWwindow* window, int key, int code, int action, int mode);
	static void handleMouse(GLFWwindow* window, double xPos, double yPos);
	void createCallBacks();
	GLFWwindow* mainWindow;
};