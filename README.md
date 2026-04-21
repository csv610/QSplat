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

---

## Technical Discussion: QSplat vs. Triangle Rendering

### The Historic Context
In the late 1990s, when QSplat was developed, GPU memory was extremely limited, and the "triangle throughput" of hardware was a major bottleneck. Visualizing a model like Michelangelo’s David (which contains over 1 billion polygons) was impossible using standard triangle-based pipelines. QSplat bypassed this by using a **Bounding Sphere Hierarchy**, where the renderer only descends into the tree until the nodes appear small enough on screen (e.g., less than one pixel). At that point, it draws a single "splat" (a point or a small quad) instead of thousands of sub-triangles.

### QSplat vs. Modern Triangle Pipelines (e.g., Nanite)
Modern engines have introduced technologies like Unreal Engine’s *Nanite*, which uses highly optimized micro-triangle clusters. However, QSplat’s approach still offers unique advantages:
1.  **Memory Efficiency:** QSplat files are highly quantized. Positions, normals, and colors are packed into small bit-fields, making them significantly smaller than standard mesh formats.
2.  **Constant-Time Complexity:** The rendering time in QSplat is proportional to the **screen resolution**, not the model's total polygon count. No matter how large the dataset, the renderer only touches the data necessary to fill the pixels on screen.
3.  **Topology Independence:** Unlike triangles, which require maintaining connectivity (edge sharing, manifolds), points are independent. This makes it much easier to visualize "noisy" data from 3D scanners or LiDAR without the overhead of surface reconstruction.

### Is Splatting Still Useful Today?
Absolutely. We are currently seeing a massive resurgence in point-based rendering:

*   **3D Gaussian Splatting (3DGS):** The most significant breakthrough in neural rendering (2023-present) is based on the core concept of QSplat. Instead of using triangles to represent scenes, 3DGS uses millions of overlapping Gaussians. The "splatting" technique we use here is the direct ancestor of the algorithms currently powering the most advanced AI-generated 3D environments.
*   **Massive Scientific Visualization:** In fields like geology, archaeology, and aerospace, datasets frequently exceed the "triangle limit" of even the best GPUs. Multiresolution point hierarchies remain the gold standard for navigating these massive scans in real-time.
*   **Web and Mobile Rendering:** Because point-based formats can be streamed and rendered progressively (showing a blurry version and refining it as the user stays still), the QSplat philosophy is ideal for low-bandwidth environments.

In summary, while triangle rendering is the king of structured game assets, **Splatting** remains the most effective way to handle **massive, unstructured, and high-complexity 3D data.**

## License
This project is subject to the original Stanford University licensing terms. Please refer to the `README` file (original documentation) for detailed licensing and usage restrictions.
