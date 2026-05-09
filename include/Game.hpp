#pragma once

#include <iostream>
#include <stdexcept>

#include "glad/glad.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>


class Game
{
private:
    int width_;
    int height_;
    GLFWwindow *window_;

    void initGLFW();
    void createWindow();
    void initGLAD();

public:
    Game();
    ~Game();

    void start();
    GLFWwindow *getWindow() const;

    static void framebuffer_size_callback(GLFWwindow *window, int width, int height);
};
