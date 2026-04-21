cmake_minimum_required(VERSION 3.16)
project(QSplat VERSION 1.02 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)

include_directories(${GLFW_INCLUDE_DIRS})
link_directories(${GLFW_LIBRARY_DIRS})

set(COMMON_SOURCES
    src/qsplat_colorquant.cpp
    src/qsplat_normquant.cpp
    src/qsplat_spherequant.cpp
)

set(QSPLAT_VIEWER_SOURCES
    src/main_glfw.cpp
    src/qsplat_gui.cpp
    src/qsplat_gui_camera.cpp
    src/qsplat_model.cpp
    src/qsplat_draw_gl.cpp
    src/qsplat_draw_gl_ellip.cpp
    src/qsplat_draw_gaussian.cpp
    src/qsplat_draw_spheres.cpp
    src/qsplat_draw_software.cpp
    src/qsplat_draw_software_tiles.cpp
    src/qsplat_gui_glfw.cpp
)

set(QSPLAT_MAKE_SOURCES
    src/qsplat_make_main.cpp
    src/qsplat_make_from_mesh.cpp
    src/qsplat_make_qtree_v11.cpp
)

add_executable(qsplat_make
    ${QSPLAT_MAKE_SOURCES}
    ${COMMON_SOURCES}
)
target_compile_definitions(qsplat_make PRIVATE DARWIN)
target_link_libraries(qsplat_make PRIVATE OpenGL::GL)

add_executable(qsplat
    ${QSPLAT_VIEWER_SOURCES}
    ${COMMON_SOURCES}
)
target_compile_definitions(qsplat PRIVATE DARWIN)
target_link_libraries(qsplat PRIVATE OpenGL::GL ${GLFW_LIBRARIES} "-framework Carbon")

install(TARGETS qsplat qsplat_make DESTINATION bin)
