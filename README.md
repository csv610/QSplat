# QSplat (Modernized Port)

This is a modernized port of **QSplat**, a multiresolution point-cloud renderer originally developed at Stanford University by Szymon Rusinkiewicz and Marc Levoy.

## Original Credits
QSplat was originally developed (1999-2000) by:
- **Szymon Rusinkiewicz** (Stanford University)
- **Marc Levoy** (Stanford University)

The original project provided a pioneering multiresolution structure for real-time visualization of massive meshes, such as the Michelangelo's David.

## Modernization Contributions
This fork (2026) aims to bring the original codebase into the modern era while preserving the carefully tuned interaction logic of the original. Key improvements include:

- **M1/M2/M3 Silicon Optimization:** Transitioned from CPU-heavy "Immediate Mode" rendering to a modern **GPU-accelerated pipeline**.
- **Vertex Buffer Objects (VBOs):** Implemented batch rendering using VAOs/VBOs to leverage the massive parallelism of modern Mac GPUs.
- **Programmable Shaders:** Integrated a custom GLSL shader pipeline to handle point rendering and lighting.
- **GLM Integration:** Incorporated the industry-standard **OpenGL Mathematics (GLM)** library for efficient GPU data alignment and shader communication.
- **Modern Build System:** Replaced legacy Makefiles with a clean **CMake** configuration compatible with latest compilers (C++17).
- **Enhanced Interaction:** 
    - Added **Scroll Wheel Zoom** support.
    - Implemented laptop-friendly shortcuts (**SHIFT+Left** for Panning, **CTRL+Left** for Zooming).
    - Fixed legacy event-loop bugs to ensure 60+ FPS smoothness on high-resolution displays.
- **Stability:** Fixed "scene recovery" bugs where models would disappear or become unresponsive during extreme zooming.

## Technical Note on Math
While the rendering pipeline now uses modern GLM-backed buffers, the core camera interaction math (Quaternions/Translations) has been intentionally kept in its original form. This ensures that the highly-tuned "feel" of the original viewer is preserved exactly as the authors intended.

## License
This software is provided "as is" and remains subject to the original Stanford University licensing terms found in the `README` file (original documentation).
