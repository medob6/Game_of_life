#include "include/Game.hpp"

Game::Game() : width_(1600), height_(800), window_(NULL)
{
}

Game::~Game()
{
    if (window_ != NULL)
    {
        glfwDestroyWindow(window_);
        window_ = NULL;
    }
    glfwTerminate();
}

void Game::initGLFW()
{
    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
}

void Game::createWindow()
{
    window_ = glfwCreateWindow(width_, height_, "Conway’s Game of Life", NULL, NULL);
    if (window_ == NULL)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window_);
    glfwSetFramebufferSizeCallback(window_, framebuffer_size_callback);
}

void Game::initGLAD()
{
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    glViewport(0, 0, width_, height_);
}

void Game::start()
{
    initGLFW();
    createWindow();
    initGLAD();
}

GLFWwindow *Game::getWindow() const
{
    return window_;
}


void Game::framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    (void)window;
    glViewport(0, 0, width, height);
}