#pragma once

#include <tuple>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class GLWindow {
public:
	GLWindow(const char* name, int width, int height, bool vsync);
	virtual ~GLWindow();

	void MainLoop();

	std::tuple<int, int> GetWindowSize();

	void ResizeWindow(int width, int height);

	void SetFullScreen(bool is_full_screen);

	void Close();

protected:
	GLFWwindow* window;

	// 全屏和窗口切换时临时保存信息
	int xpos_;
	int ypos_;
	int width_;
	int height_;

private:
	virtual void HandleDisplayEvent() = 0;

	virtual void HandleDrawGuiEvent() {}

	virtual void HandleReshapeEvent(int viewport_width, int viewport_height) {}

	virtual void HandleKeyboardEvent(int key) {}

	virtual void HandleMouseEvent(double mouse_x, double mouse_y) {}

	virtual void HandleMouseWheelEvent(double yoffset) {}

	virtual void HandleDropEvent(int count, const char** paths) {}
};

