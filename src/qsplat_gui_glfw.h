#ifndef QSPLAT_GUI_GLFW_H
#define QSPLAT_GUI_GLFW_H

#define GUI ((QSplatGLFWGUI *) theQSplatGUI)

#include "qsplat_gui.h"
#include <GLFW/glfw3.h>

class QSplatGLFWGUI : public QSplatGUI {
private:
    GLFWwindow* window;
    int winWidth, winHeight;
    bool needRedraw;
    bool fullscreenMode;
    int prevWidth, prevHeight;
    int prevX, prevY;
    const char* initialModel;

    static QSplatGLFWGUI* instance;

    void ErrorCallback(int error, const char* description);
    static void StaticErrorCallback(int error, const char* description);
    void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void StaticKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void StaticMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    void MousePosCallback(GLFWwindow* window, double x, double y);
    static void StaticMousePosCallback(GLFWwindow* window, double x, double y);
    void WindowSizeCallback(GLFWwindow* window, int width, int height);
    static void StaticWindowSizeCallback(GLFWwindow* window, int width, int height);
    void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void StaticScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

public:
    QSplatGLFWGUI();
    ~QSplatGLFWGUI();

    void SetInitialModel(const char* filename) { initialModel = filename; }
    void Go() override;
    int screenx() override { return winWidth; }
    int screeny() override { return winHeight; }
    int viewportx() override { return 0; }
    int viewporty() override { return 0; }
    void fullscreen(bool go_fullscreen) override;
    void need_redraw() override { needRedraw = true; }
    void swapbuffers() override;
    void updatestatus(const char* text) override;
    void updaterate(const char* text) override;
    void updatemenus() override {}
    void aboutmodel() override;
    void aboutqsplat() override;
    void display_dialog(const char* title, const char* message) override;
    bool abort_drawing(float time_elapsed) override;
};

#endif
