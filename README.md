# QSplat: High-Performance Multiresolution Point-Cloud Renderer

A modernized, high-performance port of the pioneering **QSplat** multiresolution point-rendering system. This version is specifically optimized for modern hardware, including Apple Silicon (M1/M2/M3) and high-resolution displays, while maintaining the legendary interaction model of the original Stanford implementation.

---

## Project Overview

QSplat is a multiresolution visualization system based on a bounding sphere hierarchy. Originally designed to handle massive meshes (such as those from the Digital Michelangelo Project), it allows for real-time navigation of hundreds of millions of points by dynamically adjusting the level of detail based on view parameters and performance targets.

This port maintains the core data structure and file format integrity while replacing the legacy, CPU-bound rendering pipeline with a modern, GPU-accelerated architecture.

## Key Modernization Features

### 🚀 Hardware Optimization
*   **Silicon-Native Performance:** Optimized for Apple Silicon integrated GPUs and modern discrete graphics.
*   **GPU-Accelerated Pipeline:** Transitioned from legacy "Immediate Mode" to **Vertex Buffer Objects (VBOs)** and **Vertex Array Objects (VAOs)**, offloading the rendering workload from the CPU to the GPU.
*   **Programmable Shading:** Implemented a GLSL-based shader pipeline for efficient point rasterization and real-time lighting.

### 🛠 Modern Engineering
*   **GLM Integration:** Utilizes the industry-standard **OpenGL Mathematics (GLM)** library for robust, shader-compatible data structures.
*   **CMake Build System:** Replaced legacy Makefiles with a cross-platform CMake configuration, ensuring compatibility with the latest C++17 compilers and IDEs.
*   **Smooth Motion:** Re-engineered the event loop to ensure consistent 60+ FPS performance even on Retina and 4K displays.

### 🖱 Enhanced Interaction Design
*   **Standardized Navigation:** Intuitive scroll-wheel zooming and improved panning sensitivity.
*   **Laptop & Trackpad Support:** Integrated modifier-key shortcuts (`SHIFT` and `CTRL`) for seamless navigation on mobile workstations.
*   **Stability Enhancements:** Resolved legacy bugs related to near-plane clipping and scene recovery during extreme zooming.

---

## Installation

### Prerequisites
The project requires a modern C++ compiler and the following libraries:
- **CMake** (3.16+)
- **GLFW 3**
- **GLM**
- **OpenGL**

On macOS, these can be installed via Homebrew:
```bash
brew install cmake glfw glm
```

### Build Instructions
```bash
# Clone the repository
git clone git@github.com:csv610/QSplat.git
cd QSplat

# Create and enter build directory
mkdir build && cd build

# Configure and compile
cmake ..
make -j$(sysctl -n hw.ncpu)
```

---

## Usage

### 1. Data Preparation
Convert standard `.ply` meshes into the optimized `.qs` multiresolution format:
```bash
./qsplat_make input.ply output.qs
```

### 2. Visualization
Launch the high-performance viewer:
```bash
./qsplat model.qs
```

### Controls Reference
The interaction model preserves the original "feel" while adding modern conveniences:

| Action | Input |
| :--- | :--- |
| **Rotate** | Left Mouse Button (Drag) |
| **Zoom** | Scroll Wheel / `CTRL` + Left Mouse Button |
| **Translate (Pan)** | Right Mouse Button / `SHIFT` + Left Mouse Button |
| **Reset View** | `CTRL` + `R` |
| **Toggle Fullscreen** | `F` |
| **Exit** | `ESC` / `CTRL` + `Q` |

---

## Credits & History

**Original Software (1999-2000):**
QSplat was originally designed and implemented by **Szymon Rusinkiewicz** and **Marc Levoy** at the Stanford University Computer Graphics Laboratory. It remains a landmark project in the history of computer graphics and massive data visualization.

**Modernization Port (2026):**
This version focuses on architectural modernization, GPU optimization, and cross-platform build stability for contemporary research and professional workflows.

## License
This project is subject to the original Stanford University licensing terms. Please refer to the `README` file (original documentation) for detailed licensing and usage restrictions.
