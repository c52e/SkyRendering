#include "GLWindow.h"

#include <stdexcept>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "Utils.h"

GLWindow::GLWindow(const char* name, int width, int height, bool vsync) {
    SetCurrentDirToExe(); // 切换到exe所在目录

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, name, NULL, NULL);
    if (window == NULL)
    {
        throw std::runtime_error("Failed to create GLFW window");
        glfwTerminate();
    }
    glfwMakeContextCurrent(window);
    if (!vsync)
        glfwSwapInterval(0);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window,
        [](GLFWwindow* window, int width, int height) {
            auto w = static_cast<GLWindow*>(glfwGetWindowUserPointer(window));
            w->HandleReshapeEvent(width, height);
        });
    glfwSetKeyCallback(window,
        [](GLFWwindow* window, int key, int scancode, int action, int mods) {
            if (ImGui::GetIO().WantCaptureKeyboard)
                return;
            auto w = static_cast<GLWindow*>(glfwGetWindowUserPointer(window));
            if (action == GLFW_PRESS)
                w->HandleKeyboardEvent(key);
        });
    glfwSetCursorPosCallback(window,
        [](GLFWwindow* window, double x, double y) {
            if (ImGui::GetIO().WantCaptureMouse)
                return;
            auto w = static_cast<GLWindow*>(glfwGetWindowUserPointer(window));
            w->HandleMouseEvent(x, y);
        });
    glfwSetScrollCallback(window,
        [](GLFWwindow* window, double xoffset, double yoffset) {
            auto w = static_cast<GLWindow*>(glfwGetWindowUserPointer(window));
            w->HandleMouseWheelEvent(yoffset);
        });
    glfwSetDropCallback(window,
        [](GLFWwindow* window, int count, const char** paths) {
            auto w = static_cast<GLWindow*>(glfwGetWindowUserPointer(window));
            w->HandleDropEvent(count, paths);
        });

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");
}

GLWindow::~GLWindow() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
}

void GLWindow::MainLoop() {
    while (!glfwWindowShouldClose(window)) {
        HandleDisplayEvent();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        HandleDrawGuiEvent();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

std::tuple<int, int>  GLWindow::GetWindowSize() {
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    return { width, height };
}

void GLWindow::ResizeWindow(int width, int height) {
    glfwSetWindowSize(window, width, height);
}

void GLWindow::SetFullScreen(bool is_full_screen) {
    if (is_full_screen) {
        glfwGetWindowPos(window, &xpos_, &ypos_);
        glfwGetWindowSize(window, &width_, &height_);
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, 0);
    }
    else {
        // restore last window size and position
        glfwSetWindowMonitor(window, nullptr, xpos_, ypos_, width_, height_, 0);
    }
}

void GLWindow::Close() {
    glfwSetWindowShouldClose(window, true);
}