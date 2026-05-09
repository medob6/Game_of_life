# Conway's Game of Life

Infinite cellular automata in C++ with GPU-rendered grid/cells, interactive controls, and PNG-driven pattern loading.

---

## Demo

![Demo](./video/demo.gif)

---

## Features

- Infinite sparse grid with pan and zoom (mouse drag + scroll)
- Click to toggle cells alive or dead directly on the grid
- Instanced rendering for alive cells (GPU efficient)
- Procedural grid drawn via fragment shader (no CPU line drawing)
- PNG pattern loading and auto-centering on the simulation grid
- Scrollable HUD panel with shape thumbnails and keyboard navigation
- Generation history and rewind (`B`) and reset (`R`)
- Real-time speed control (`=` to speed up, `-` to slow down)
- Pause/resume and single-step control (`Space`, `N`)

---

## Controls

| Key / Input        | Action                                         |
|--------------------|------------------------------------------------|
| Space              | Pause / resume simulation                      |
| N                  | Advance one generation                         |
| R                  | Reset to generation 0                          |
| B                  | Rewind 10 generations                          |
| =                  | Increase simulation speed                      |
| -                  | Decrease simulation speed                      |
| ↑ / ↓              | Navigate shape list in HUD                     |
| Enter              | Load selected HUD shape                        |
| H                  | Show / hide HUD panel                          |
| Escape             | Quit application                               |
| Left click         | Toggle one cell                                |
| Left click + drag  | Pan grid                                       |
| Mouse scroll       | Zoom grid (or scroll HUD in panel)             |

---

## Dependencies

- C++ compiler (clang++ or c++)
- make
- OpenGL 3.3 core
- GLFW
- POSIX headers (dirent.h)
- stb_image (bundled at `include/stb_image.h`)

---

## Build & Run

```bash
# Build from repository root
make
./GameOfLife
```

To run with a specific pattern:

```bash
./GameOfLife Gosper_glider_gun.png
```

Compilation flags used:

```bash
clang++ -Wall -Wextra -Werror -std=c++17 -Iinclude
```

Useful Makefile targets:

```bash
make clean
make fclean
make re
```

---

## Project Structure

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

## How It Works

1. A fullscreen quad is rendered and a fragment shader procedurally draws the infinite grid.
2. Alive cells are stored sparsely and rendered as instanced quads for efficient drawing.
3. A left-side HUD overlay is drawn in screen space with thumbnail textures for patterns.
4. Input callbacks provide panning, zoom, cell editing, playback controls, and rewind.

---

## Roadmap

- Add pattern search/filter in HUD panel
- Save and load simulation snapshots
- Rule presets beyond Conway (custom rules)
- Optional FPS/generation timing overlay

---

## License

This project is released under an Open Source License. You are free to use, modify, and distribute it under the terms of your preferred open-source license (such as MIT or Apache 2.0). Please add your choice of license file (e.g., LICENSE) to the repository for more details.

---
