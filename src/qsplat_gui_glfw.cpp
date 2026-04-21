#include "qsplat_gui_glfw.h"
#include "qsplat_gui_camera.h"
#include "qsplat_model.h"
#include <cstdio>
#include <cstring>
#include <sys/time.h>

#define INIT_WINDOW_WIDTH 623
#define INIT_WINDOW_HEIGHT 400

QSplatGLFWGUI* QSplatGLFWGUI::instance = nullptr;

QSplatGLFWGUI::QSplatGLFWGUI()
    : window(nullptr), winWidth(INIT_WINDOW_WIDTH), winHeight(INIT_WINDOW_HEIGHT),
      needRedraw(true), fullscreenMode(false), prevWidth(0), prevHeight(0), prevX(0), prevY(0),
      initialModel(nullptr)
{
}

QSplatGLFWGUI::~QSplatGLFWGUI()
{
    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
}

void QSplatGLFWGUI::ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void QSplatGLFWGUI::StaticErrorCallback(int error, const char* description)
{
    if (instance) instance->ErrorCallback(error, description);
}

void QSplatGLFWGUI::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            if (fullscreenMode) fullscreen(false);
            break;
        case GLFW_KEY_F:
            fullscreen(!fullscreenMode);
            break;
        case GLFW_KEY_R:
            if (mods & GLFW_MOD_CONTROL) {
                resetviewer();
            }
            break;
        case GLFW_KEY_O:
            if (mods & GLFW_MOD_CONTROL) {
                const char* filename = "/tmp/test.ply";
                OpenModel(filename);
            }
            break;
        case GLFW_KEY_Q:
            if (mods & GLFW_MOD_CONTROL) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            break;
        }
    }
    needRedraw = true;
}

void QSplatGLFWGUI::StaticKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (instance) instance->KeyCallback(window, key, scancode, action, mods);
}

void QSplatGLFWGUI::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    
    if (action == GLFW_PRESS) {
        mousebutton mb = NO_BUTTON;
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (mods & GLFW_MOD_CONTROL) mb = TRANSZ_BUTTON;
            else if (mods & GLFW_MOD_SHIFT) mb = TRANSXY_BUTTON;
            else mb = ROT_BUTTON;
        }
        else if (button == GLFW_MOUSE_BUTTON_RIGHT) mb = TRANSXY_BUTTON;
        else if (button == GLFW_MOUSE_BUTTON_MIDDLE) mb = TRANSZ_BUTTON;
        mouse((int)x, (int)y, mb);
    } else if (action == GLFW_RELEASE) {
        mouse((int)x, (int)y, NO_BUTTON);
    }
    needRedraw = true;
}

void QSplatGLFWGUI::StaticMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (instance) instance->MouseButtonCallback(window, button, action, mods);
}

void QSplatGLFWGUI::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    if (yoffset > 0) mouse((int)x, (int)y, UP_WHEEL);
    else if (yoffset < 0) mouse((int)x, (int)y, DOWN_WHEEL);
    needRedraw = true;
}

void QSplatGLFWGUI::StaticScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (instance) instance->ScrollCallback(window, xoffset, yoffset);
}

void QSplatGLFWGUI::MousePosCallback(GLFWwindow* window, double x, double y)
{
    if (dragging()) {
        mouse((int)x, (int)y, last_button);
        needRedraw = true;
    }
}

void QSplatGLFWGUI::StaticMousePosCallback(GLFWwindow* window, double x, double y)
{
    if (instance) instance->MousePosCallback(window, x, y);
}

void QSplatGLFWGUI::WindowSizeCallback(GLFWwindow* window, int width, int height)
{
    winWidth = width;
    winHeight = height;
    needRedraw = true;
}

void QSplatGLFWGUI::StaticWindowSizeCallback(GLFWwindow* window, int width, int height)
{
    if (instance) instance->WindowSizeCallback(window, width, height);
}

void QSplatGLFWGUI::Go()
{
    instance = this;

    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return;
    }

    glfwSetErrorCallback(StaticErrorCallback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);

    window = glfwCreateWindow(winWidth, winHeight, "QSplat", nullptr, nullptr);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    if (initialModel) {
        OpenModel(initialModel);
    }

    glfwSetKeyCallback(window, StaticKeyCallback);
    glfwSetMouseButtonCallback(window, StaticMouseButtonCallback);
    glfwSetCursorPosCallback(window, StaticMousePosCallback);
    glfwSetWindowSizeCallback(window, StaticWindowSizeCallback);
    glfwSetScrollCallback(window, StaticScrollCallback);

    glfwGetWindowSize(window, &winWidth, &winHeight);

    const char* renderer = (const char*)glGetString(GL_RENDERER);
    whichDriver = OPENGL_POLYS_CIRC;

    set_desiredrate(60.0f);
    set_shiny(true);
    set_backfacecull(true);
    set_showlight(false);
    set_showprogressbar(true);

    while (!glfwWindowShouldClose(window)) {
        if (needRedraw) {
            needRedraw = false;
            redraw();
        }

        if (!idle()) {
            needRedraw = true;
        }

        glfwPollEvents();
    }

    glfwTerminate();
}

void QSplatGLFWGUI::fullscreen(bool go_fullscreen)
{
    if (go_fullscreen && !fullscreenMode) {
        glfwGetWindowPos(window, &prevX, &prevY);
        glfwGetWindowSize(window, &prevWidth, &prevHeight);
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        fullscreenMode = true;
    } else if (!go_fullscreen && fullscreenMode) {
        glfwSetWindowMonitor(window, nullptr, prevX, prevY, prevWidth, prevHeight, 0);
        fullscreenMode = false;
    }
    needRedraw = true;
}

void QSplatGLFWGUI::swapbuffers()
{
    if (window) {
        glfwSwapBuffers(window);
    }
}

void QSplatGLFWGUI::updatestatus(const char* text)
{
}

void QSplatGLFWGUI::updaterate(const char* text)
{
}

void QSplatGLFWGUI::aboutmodel()
{
    if (theQSplat_Model) {
        display_dialog("About this Model", theQSplat_Model->comments.c_str());
    } else {
        display_dialog("", "No model loaded.");
    }
}

void QSplatGLFWGUI::aboutqsplat()
{
    display_dialog("About QSplat", "QSplat version 1.02\nby Szymon Rusinkiewicz\n\nCopyright (c) 1999-2000\nStanford University");
}

void QSplatGLFWGUI::display_dialog(const char* title, const char* message)
{
    printf("%s: %s\n", title, message);
}

bool QSplatGLFWGUI::abort_drawing(float time_elapsed)
{
    return time_elapsed >= 1.2f / rate() + 0.1f || glfwWindowShouldClose(window);
}

int main_gui_main(int argc, char** argv)
{
    QSplatGLFWGUI gui;
    theQSplatGUI = &gui;

    const char* filename = nullptr;
    if (argc > 1) {
        filename = argv[1];
        gui.OpenModel(filename);
    }

    gui.Go();
    return 0;
}