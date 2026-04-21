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
*   **Assimp Integration:** Expanded file format support. The converter now accepts **OBJ, STL, and OFF** in addition to the original PLY format, leveraging the Open Asset Import Library.
*   **GLM Integration:** Utilizes the industry-standard **OpenGL Mathematics (GLM)** library for robust, shader-compatible data structures.
*   **CMake Build System:** Replaced legacy Makefiles with a cross-platform CMake configuration, ensuring compatibility with the latest C++17 compilers and IDEs.
*   **Smooth Motion:** Re-engineered the event loop to ensure consistent 60+ FPS performance even on Retina and 4K displays.

### 🖱 Enhanced Interaction Design
*   **One-Step Workflow:** Seamlessly open standard meshes (**OBJ, PLY, STL, OFF**) directly in the viewer. The system automatically performs multiresolution conversion in the background, eliminating the need for a separate preprocessing step.
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
- **Assimp** (Open Asset Import Library)
- **OpenGL**

On macOS, these can be installed via Homebrew:
```bash
brew install cmake glfw glm assimp
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

### 🚀 Direct Visualization
You can now open standard 3D meshes (PLY, OBJ, STL, OFF) directly in the high-performance viewer. The system automatically converts them to the optimized `.qs` format in the background:
```bash
./qsplat model.obj
```

### 🛠 Manual Data Preparation (Optional)
For very large datasets, you may still want to pre-convert meshes into the optimized `.qs` multiresolution format:
```bash
./qsplat_make large_input.ply output.qs
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

### QSplat vs. Instance Rendering
Hardware Instance Rendering (`glDrawArraysInstanced`) is a modern technique designed to render thousands of copies of the *same* complex mesh (like a forest of trees or a crowd of characters) with a single draw call. While both QSplat and Instancing aim to reduce draw call overhead, they solve different problems:

*   **Geometry Complexity:** Instance rendering excels when the repeated object is complex (e.g., a 1,000-triangle tree). QSplat, conversely, focuses on "atomic" geometry—points or simple quads. In QSplat, the "complexity" lies in the **billions of unique points**, not the complexity of a single instance.
*   **Data Uniqueness:** In a standard instanced scene, most data (vertex positions, UVs) is shared across instances. In QSplat, every visible node has **unique attributes** (specific quantized color, unique normal, and a precise position in the hierarchy). Because every splat is different, the benefits of pure instancing are reduced, making **VBO Batching** (our current modernized approach) more efficient than Instancing for simple point clouds.
*   **The Hybrid Path:** Modern high-fidelity splatters (like 3D Gaussian Splatting) sometimes use a hybrid approach: using Instance Rendering to draw a high-quality "proxy" shape (like a 3D sphere or a Gaussian ellipsoid) for every point in the cloud. This allows the GPU to handle the "roundness" of the splats while the CPU/Compute shader handles the hierarchy traversal.

### Is Splatting Still Useful Today?
Absolutely. We are currently seeing a massive resurgence in point-based rendering:

*   **3D Gaussian Splatting (3DGS):** The most significant breakthrough in neural rendering (2023-present) is based on the core concept of QSplat. Instead of using triangles to represent scenes, 3DGS uses millions of overlapping Gaussians. The "splatting" technique we use here is the direct ancestor of the algorithms currently powering the most advanced AI-generated 3D environments.
*   **Massive Scientific Visualization:** In fields like geology, archaeology, and aerospace, datasets frequently exceed the "triangle limit" of even the best GPUs. Multiresolution point hierarchies remain the gold standard for navigating these massive scans in real-time.
*   **Web and Mobile Rendering:** Because point-based formats can be streamed and rendered progressively (showing a blurry version and refining it as the user stays still), the QSplat philosophy is ideal for low-bandwidth environments.

In summary, while triangle rendering is the king of structured game assets, **Splatting** remains the most effective way to handle **massive, unstructured, and high-complexity 3D data.**

---

## Future Work & Contributing

Contributions are welcome to further modernize this landmark project. Current areas of interest include:
*   **Compute Shader Traversal:** Offloading the bounding sphere hierarchy traversal from the CPU to GPU Compute Shaders.
*   **Metal/Vulkan Backend:** Native support for modern graphics APIs beyond OpenGL.
*   **Gaussian Splatting Integration:** Implementing modern Gaussian kernels for high-fidelity surface reconstruction.
*   **WebGPU Port:** Bringing high-performance multiresolution rendering to the browser.

Please feel free to submit pull requests or open issues for feature requests and bug reports.

## License
This project is subject to the original Stanford University licensing terms. Please refer to the `README` file (original documentation) for detailed licensing and usage restrictions.
