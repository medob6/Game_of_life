# 🚀 Conway's Game of Life

**Infinite cellular automata in C++ with GPU-rendered grid/cells, interactive controls, and PNG-driven pattern loading.**

![Language](https://img.shields.io/badge/language-C%2B%2B-blue)
![Standard](https://img.shields.io/badge/standard-C%2B%2B17-00599C)
![Build](https://img.shields.io/badge/build-Makefile-6C4A3D)

---

## 🎬 Demo

![Demo placeholder](./docs/demo.gif)

_Demo placeholder: add a capture of pan/zoom, cell editing, HUD pattern selection, and rewind controls._

---

## ✨ Features

- 🌌 **Infinite sparse grid** with pan and zoom (mouse drag + scroll)
- 🖱️ **Click-to-toggle cells** alive/dead directly on the grid
- ⚡ **Instanced rendering** for alive cells (GPU efficient)
- 🧮 **Procedural grid in fragment shader** (no CPU line drawing)
- 🖼️ **PNG pattern loading** and auto-centering on the simulation grid
- 🧰 **Scrollable HUD panel** with shape thumbnails + keyboard navigation
- ⏪ **Generation history + rewind** (`B`) and reset (`R`)
- ⏱️ **Real-time speed control** (`=` faster, `-` slower)
- ⏯️ **Pause/resume + single-step** (`Space`, `N`)

---

## 🎮 Controls

| Key / Input | Action |
|---|---|
| `Space` | Pause / resume simulation |
| `N` | Advance one generation (single-step) |
| `R` | Reset to generation `0` |
| `B` | Rewind `10` generations |
| `=` | Increase simulation speed |
| `-` | Decrease simulation speed |
| `↑` / `↓` | Navigate shape list in HUD |
| `Enter` | Load selected HUD shape |
| `H` | Show / hide HUD panel |
| `Escape` | Quit application |
| `Left click` | Toggle one cell |
| `Left click + drag` | Pan grid |
| `Mouse scroll` | Zoom grid (or scroll HUD when cursor is over panel) |

---

## 📦 Dependencies

- C++ compiler (`clang++`/`c++`) with C++17 support
- `make`
- OpenGL 3.3 core
- GLFW
- POSIX headers (`dirent.h`)
- `stb_image` (bundled at `include/stb_image.h`)

---

## 🛠️ Build & Run

```bash
# from repository root
make
./GameOfLife
```

Run with a specific pattern:

```bash
./GameOfLife Gosper_glider_gun.png
```

Compiler flags used by the Makefile:

```bash
-Wall -Wextra -Werror -Iinclude
```

Useful targets:

```bash
make clean
make fclean
make re
```

---

## 📁 Project Structure

```text
.
├── main.cpp
├── Game.cpp
├── GameLoop.cpp
├── ShapeLoader.cpp
├── Makefile
├── include/
│   ├── Game.hpp
│   ├── GameLoop.hpp
│   ├── ShapeLoader.hpp
│   ├── PrecomputedShapes.hpp
│   └── stb_image.h
├── Shapes/
│   └── *.png
└── tools/
    └── generate_shapes.cpp
```

---

## ⚙️ How It Works

1. A fullscreen quad is rendered and a **fragment shader** procedurally draws the infinite grid.
2. Alive cells are stored sparsely and rendered as **instanced quads**, minimizing draw overhead.
3. A left-side **HUD overlay** is drawn in screen space with thumbnail textures for fast pattern loading.
4. Input callbacks drive panning, zoom, cell editing, playback controls, and generation history rewind.

---

## 🧭 Roadmap

- [ ] Add pattern search/filter in the HUD panel
- [ ] Save and load simulation snapshots
- [ ] Rule presets beyond Conway (`B/S` custom rules)
- [ ] Add optional FPS/generation timing overlay

---

**License:** No license file is currently defined in this repository.
