#include "qsplat_gui_glfw.h"

int main(int argc, char** argv)
{
    QSplatGLFWGUI gui;
    theQSplatGUI = &gui;

    if (argc > 1) {
        gui.SetInitialModel(argv[1]);
    }

    gui.Go();
    return 0;
}
