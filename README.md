# Game of Life

A C++/OpenGL implementation of Conway's Game of Life with an interactive grid and shape loading.

## Requirements

- C++ compiler (supports the current project sources)
- `make`
- OpenGL development libraries
- GLFW (`GLFW/glfw3.h`)

## Build

From the repository root:

```bash
make
```

This produces the executable:

- `./GameOfLife`

## Run

Default run (loads the default shape):

```bash
./GameOfLife
```

Run with a specific shape file from the `Shapes/` directory:

```bash
./GameOfLife Gosper_glider_gun.png
```

## Useful Make targets

- `make` — build
- `make clean` — remove object files
- `make fclean` — remove object files and executable
- `make re` — full rebuild
