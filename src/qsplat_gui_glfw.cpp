#define GL_SILENCE_DEPRECATION
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
{
    instance = this;
    window = nullptr;
    fullscreenMode = false;
    winWidth = INIT_WINDOW_WIDTH;
    winHeight = INIT_WINDOW_HEIGHT;
}

QSplatGLFWGUI::~QSplatGLFWGUI()
{
    if (window) glfwDestroyWindow(window);
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
        case GLFW_KEY_H:
            resetviewer();
            break;
        case GLFW_KEY_B:
            showbbox = !showbbox;
            break;
        case GLFW_KEY_O:
            if (mods & GLFW_MOD_CONTROL) {
                // This is a placeholder, actual file dialog would be better
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
        needRedraw = true;
    }
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
    if (!fullscreenMode) {
        winWidth = width;
        winHeight = height;
    }
    glViewport(0, 0, width, height);
    needRedraw = true;
}

void QSplatGLFWGUI::StaticWindowSizeCallback(GLFWwindow* window, int width, int height)
{
    if (instance) instance->WindowSizeCallback(window, width, height);
}

void QSplatGLFWGUI::Go()
{
    if (!glfwInit()) return;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    
    window = glfwCreateWindow(INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT, "QSplat", NULL, NULL);
    if (!window) {
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
    if (go_fullscreen == fullscreenMode) return;

    if (go_fullscreen) {
        glfwGetWindowPos(window, &prevX, &prevY);
        glfwGetWindowSize(window, &prevWidth, &prevHeight);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        winWidth = mode->width;
        winHeight = mode->height;
    } else {
        glfwSetWindowMonitor(window, NULL, prevX, prevY, prevWidth, prevHeight, 0);
        winWidth = prevWidth;
        winHeight = prevHeight;
    }
    fullscreenMode = go_fullscreen;
    needRedraw = true;
}

void QSplatGLFWGUI::swapbuffers()
{
    glfwSwapBuffers(window);
}

void QSplatGLFWGUI::updatestatus(const char* text)
{
    char title[256];
    snprintf(title, sizeof(title), "QSplat - %s", text);
    glfwSetWindowTitle(window, title);
}

void QSplatGLFWGUI::updaterate(const char* text)
{
    // For now, we can append to the title or just ignore
}

void QSplatGLFWGUI::aboutmodel()
{
    if (theQSplat_Model)
        display_dialog("About Model", theQSplat_Model->comments.c_str());
    else
        display_dialog("About Model", "No model loaded.");
}

void QSplatGLFWGUI::aboutqsplat()
{
    display_dialog("About QSplat", "Modernized QSplat Port\nOriginal by Szymon Rusinkiewicz and Marc Levoy");
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

    if (argc > 1) {
        gui.SetInitialModel(argv[1]);
    }

    gui.Go();
    return 0;
}
