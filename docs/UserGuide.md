# QSplat: A Guide to Multiresolution Point Rendering

Welcome to the **QSplat User Guide**. This document is designed as a pedagogical resource for undergraduate students in computer graphics, visualization, and high-performance computing. It explores the theory, architecture, and practical application of one of the most influential point-based rendering systems in graphics history.

---

## Table of Contents
1. [Introduction to Point-Based Rendering](#1-introduction-to-point-based-rendering)
2. [The Core Innovation: Bounding Sphere Hierarchies](#2-the-core-innovation-bounding-sphere-hierarchies)
3. [The Multiresolution Pipeline](#3-the-multiresolution-pipeline)
4. [Getting Started with QSplat](#4-getting-started-with-qsplat)
5. [The Modernized Architecture](#5-the-modernized-architecture)
6. [Interactive Controls and Navigation](#6-interactive-controls-and-navigation)
7. [Advanced Concepts: Quantization and Rate Control](#7-advanced-concepts-quantization-and-rate-control)

---

## 1. Introduction to Point-Based Rendering

In traditional computer graphics, surfaces are typically represented as meshes of triangles. While triangles are the "gold standard" for most applications, they encounter significant limitations when dealing with **massive datasets**. Imagine trying to render a model of a cathedral scanned at sub-millimeter precision, resulting in billions of polygons. At that scale, a single triangle might be smaller than a pixel on your screen, making the overhead of processing triangle connectivity and rasterization extremely inefficient.

**Splatting** is an alternative approach. Instead of triangles, we represent the geometry as a dense collection of points (or "splats"). Each point has a position, a color, and a radius. When rendered, it creates a small, circular or elliptical "blob" on the screen. By overlapping millions of these blobs, we can reconstruct a smooth, continuous surface.

---

## 2. The Core Innovation: Bounding Sphere Hierarchies

QSplat's efficiency comes from its data structure: the **Bounding Sphere Hierarchy**. 

### 2.1 What is a Bounding Sphere?
A bounding sphere is the smallest sphere that completely encloses a set of points. In QSplat, every node in the hierarchy represents a bounding sphere.

### 2.2 The Tree Structure
The entire 3D model is organized into a tree (typically a 1-to-4 or 1-to-8 branching structure):
*   **Root Node:** A single sphere enclosing the entire model.
*   **Internal Nodes:** Spheres that enclose a subset of the model's points. These nodes provide a "coarse" approximation of that section.
*   **Leaf Nodes:** The smallest spheres at the bottom of the tree, representing individual points or very small clusters.

### 2.3 The Traversal Algorithm
When rendering, the system performs a recursive traversal of the tree:
1.  **Culling:** If a sphere is completely outside the camera's view (the "view frustum"), we skip it and all its children.
2.  **LOD Decision:** We project the sphere onto the screen. If the projected size (in pixels) is smaller than a user-defined threshold (e.g., 1 pixel), we stop descending and simply draw that sphere as a single point.
3.  **Recursion:** If the sphere is larger than the threshold, we open it and repeat the process for its children.

This algorithm ensures **Constant-Time Complexity** relative to the screen resolution. No matter how many billions of points are in the model, we only process the nodes necessary to fill the pixels on your screen.

---

## 3. The Multiresolution Pipeline

Using QSplat typically involves two main stages: **Preprocessing** and **Visualization**.

### 3.1 Preprocessing (The Converter)
Before a standard mesh (like a `.ply` or `.obj` file) can be rendered efficiently, it must be converted into a `.qs` file. The converter (`qsplat_make`):
1.  Reads the input geometry.
2.  Recursively builds the bounding sphere hierarchy.
3.  **Quantizes** the data (compressing positions and normals) to minimize file size.

### 3.2 Visualization (The Viewer)
The viewer (`qsplat`) loads the `.qs` file and performs the real-time hierarchy traversal. Because the data is already organized for multiresolution access, the viewer can start rendering immediately, even for files that are gigabytes in size.

---

## 4. Getting Started with QSplat

### 4.1 Installation
QSplat requires a modern C++ environment. On macOS, you can set up the environment using Homebrew:
```bash
brew install cmake glfw glm assimp
```

### 4.2 Building the Project
We use CMake for a cross-platform build process:
```bash
mkdir build && cd build
cmake ..
make -j
```

### 4.3 Running the Viewer
You can open a mesh directly:
```bash
./qsplat my_model.obj
```
*Note: If you provide a raw mesh file, the system will automatically convert it to the optimized `.qs` format in the background.*

---

## 5. The Modernized Architecture

While the original QSplat was written in the late 1990s using legacy OpenGL "Immediate Mode," this version has been completely modernized for today's hardware:

*   **Vertex Buffer Objects (VBOs):** Instead of sending points to the GPU one by one, we batch them into large buffers, significantly reducing the overhead of the CPU-to-GPU communication.
*   **GLSL Shaders:** We use programmable shaders to handle the "splatting" logic. This allows for high-quality lighting (Phong shading) and smooth, anti-aliased edges on each point.
*   **Asynchronous Loading:** The system can stream data from the disk while you navigate, ensuring that the interface remains responsive even when loading massive files.

---

## 6. Interactive Controls and Navigation

Navigating a massive 3D scene requires precise control. QSplat uses a "Virtual Trackball" metaphor.

| Action | Input | Description |
| :--- | :--- | :--- |
| **Rotate** | Left Drag | Rotates the model around its center. |
| **Zoom** | Scroll / `CTRL` + Left Drag | Moves the camera closer or further away. |
| **Pan** | Right Drag / `SHIFT` + Left Drag | Slides the model horizontally or vertically. |
| **Reset View** | `H` | Centers the model and resets the zoom level. |
| **Toggle Box** | `B` | Displays the bounding boxes for the hierarchy nodes. |
| **LOD Detail** | `[` and `]` | Manually increase or decrease the rendering detail. |

---

## 7. Advanced Concepts: Quantization and Rate Control

### 7.1 Quantization: Doing More with Less
To handle massive datasets, QSplat uses **quantization**. Instead of using standard 32-bit floating point numbers for everything, it packs data into small bit-fields:
*   **Position:** Relative to the parent sphere, stored in just a few bits.
*   **Normals:** Mapped to a pre-computed table of directions, stored in 13 bits.
*   **Color:** Quantized to 16 bits (RGB 565).
This allows the system to store millions of points in a fraction of the memory required by traditional formats.

### 7.2 Rate Control
QSplat features a "Target Frame Rate" system. If the computer is struggling to maintain 60 frames per second, the renderer will automatically increase the "pixel size threshold," showing a slightly coarser version of the model to keep the movement smooth. When you stop moving, the system "refines" the image, filling in the missing details.

---

## 8. Modern Context: Gaussian Splatting

While QSplat was a pioneer in the late 90s, the principles of point-based rendering have seen a massive resurgence recently with **3D Gaussian Splatting (3DGS)**. This technique, introduced in 2023, has become a cornerstone of neural rendering.

### 8.1 What is a 3D Gaussian?
In QSplat, a point is a "hard" sphere with a fixed radius. In 3DGS, each point is a "soft" **3D Gaussian distribution**. Mathematically, a Gaussian is defined by:
*   **Mean ($\mu$):** The center position of the point in 3D space (just like QSplat).
*   **Covariance ($\Sigma$):** This defines the shape and orientation. Unlike a sphere, a Gaussian can be stretched into an ellipsoid. This is stored using **Scaling** and **Rotation** parameters.
*   **Opacity ($\alpha$):** How transparent the Gaussian is.
*   **Spherical Harmonics (SH):** Instead of a single color, Gaussians store SH coefficients. This allows the color of the point to change depending on the angle you are looking at it from (simulating reflections and glossiness).

### 8.2 The Differentiable Rendering Pipeline
One of the biggest differences is how the data is created. 
*   **QSplat:** We take a known 3D mesh and convert it into spheres.
*   **3DGS:** We take a set of **2D photos** of a scene. The system starts with a sparse cloud of Gaussians and uses a process called **Differentiable Rendering**. It renders the Gaussians from the same angles as the photos and calculates the "loss" (the difference between the render and the photo). It then uses **Gradient Descent** to move, stretch, and recolor the Gaussians until the rendered scene matches the photos perfectly.

### 8.3 Rasterization and Alpha Blending
The rendering process (the "Splatting") is highly optimized:
1.  **Projection:** The 3D Gaussians are projected onto the 2D screen coordinate system.
2.  **Tiling:** The screen is divided into tiles (e.g., 16x16 pixels). Gaussians are sorted by depth within each tile.
3.  **Alpha Compositing:** For every pixel, the system blends the overlapping Gaussians from front to back using the standard volume rendering formula:
    $$C = \sum_{i \in N} c_i \alpha_i \prod_{j=1}^{i-1} (1 - \alpha_j)$$
    This "back-to-front" or "front-to-back" blending is what creates the high-fidelity, photo-realistic results.

### 8.4 Why Study QSplat First?
Understanding QSplat provides the foundation for 3DGS:
1.  **Spatial Data Structures:** Both systems rely on organizing points in space to render millions of them efficiently.
2.  **Point Primitives:** The concept of moving away from triangles to "splats" is identical.
3.  **Real-Time Performance:** Both systems push the limits of GPU throughput to achieve interactive frame rates.

By mastering the hierarchy and traversal in QSplat, you are learning the core architectural principles that modern AI-driven rendering systems use to reconstruct the world from images.

---

## Summary for Students
QSplat represents a fundamental shift in thinking about 3D data. By moving away from triangles and toward a **hierarchical point representation**, it allows us to visualize the largest 3D scans ever created. As you explore this codebase, pay close attention to the `qsplat_traverse_v11.h` file—it is the heart of the system where the magic of multiresolution rendering happens.

*Happy Coding and Rendering!*
