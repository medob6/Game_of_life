#pragma once

#include "Game.hpp"
#include <vector>
#include <unordered_set>
#include <utility>
#include <cstdint>
#include <string>

// the state is updated by the input handler;
// there can be multipel states at the same time .

// enum
enum State
{
    NONE = 0,
    RUNNING = 1 << 0,
    PAUSED = 1 << 1,
    // add more states as needed
};

class GameLoop
{

private:
    State state_;
    Game *game_;
    unsigned int VBO;           // The data container 📦
    unsigned int VAO;           // The layout manager 🗂️
    unsigned int shaderProgram; // The "Painter" program ⚡
    // Grid rendering
    unsigned int gridVBO;
    unsigned int gridVAO;
    unsigned int gridShaderProgram;
    int gridVertexCount;
    // Cell storage (sparse infinite grid)
    std::unordered_set<long long> aliveCells;
    std::vector<std::unordered_set<long long>> generationHistory;
    // Instance rendering for alive cells
    unsigned int cellVAO;
    unsigned int cellVBO;     // unit quad
    unsigned int instanceVBO; // per-instance offsets
    unsigned int cellShaderProgram;
    int instanceCount;
    // Pan & zoom state for the grid
    float cellSize; // world cell size (in NDC Y units)
    float offsetX;  // world offset X
    float offsetY;  // world offset Y
    bool dragging;
    bool leftMouseDown;
    bool clickCandidate;
    double lastMouseX;
    double lastMouseY;
    double pressMouseX;
    double pressMouseY;
    // debug cursor cell coordinates
    int cursorCellX;
    int cursorCellY;

    // GLFW callbacks (registered per-window, retrieve instance via user pointer)
    static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
    static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow *window, double xpos, double ypos);
    // Game update / input helpers
    bool stepRequested;
    double lastUpdateTime;
    double updateInterval; // seconds per generation
    int generationCount;
    bool spacePrev;
    bool nPrev;
    bool speedUpPrev;
    bool speedDownPrev;

    // Title state for flicker prevention
    int lastTitleGeneration;
    int lastTitleCursorX;
    int lastTitleCursorY;
    double lastTitleInterval;
    std::string lastTitleText;

    void initRendering();
    void setupGeometry();
    void setupShaderProgram();
    void setupGrid(int rows = 50, int cols = 50);
    unsigned int compileShader(unsigned int shaderType, const char *source);
    void cleanupRendering();
    void processInput();
    void update();
    void updateGeneration();
    // cell management
    long long cellKey(int x, int y) const;
    std::pair<int, int> screenPosToCell(double xpos, double ypos) const;
    void toggleCellAtScreenPos(double xpos, double ypos);
    void updateInstanceBuffer();
    void updateCursorCell(double xpos, double ypos);
    void syncGenerationZeroSnapshot();
    void restoreGenerationState(int targetGeneration);
    void rewindGenerations(int steps);
    bool isCellAlive(int x, int y) const;
    int countAliveNeighbors(int x, int y) const;
    void setCellAlive(int x, int y, bool alive);
    void render();
    void updateCellsForNextGeneration();
    void updateTitleIfChanged();
    void applyLoadedShape(const std::vector<std::pair<int, int>> &coords);

    // --- HUD (left-side vertical panel) ---
    static const int HUD_WIDTH = 180;  // panel width in pixels
    static const int HUD_ITEM_H = 100; // thumbnail height
    static const int HUD_PADDING = 10;

    unsigned int hudVAO, hudVBO;
    unsigned int hudShaderProgram;

    struct HudShape
    {
        std::string name;
        unsigned int textureID;
    };
    std::vector<HudShape> hudShapes;
    int hudScrollOffset = 0;
    int selectedHudShape = -1;
    bool hudVisible = false;

    // helpers
    void initHUD();
    void renderHUD();
    void loadHUDTextures();
    unsigned int loadTextureFromFile(const std::string &path);
    void drawHUDQuad(float x, float y, float w, float h,
                     unsigned int texID,
                     float r, float g, float b, float a);
    bool mouseInHUD() const; // is cursor currently over the panel?

public:
    GameLoop();
    GameLoop(Game &other);
    ~GameLoop();

    void run();
    void loadShapeByName(const std::string &imageName);
    void loadShapeFromImage(const std::string &imagePath);
};
