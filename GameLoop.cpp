#include "include/GameLoop.hpp"
#include "include/ShapeLoader.hpp"
#include <cmath>
#include <climits>

const char *vertexShaderSource = "#version 330 core\n"
                                 "layout (location = 0) in vec3 aPos;\n"
                                 "uniform float scale;\n"
                                 "uniform vec2 offset;\n"
                                 "uniform float aspect;\n"
                                 "void main()\n"
                                 "{\n"
                                 "   // Scale first, then move!\n"
                                 " gl_Position = vec4(((aPos.x * scale) / aspect) + offset.x, (aPos.y * scale) + offset.y, aPos.z, 1.0);\n"
                                 "}\0";
static const char *fragmentShaderSource = "#version 330 core\n"
                                          "out vec4 FragColor;\n"
                                          "void main()\n"
                                          "{\n"
                                          "   FragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);\n"
                                          "}\n\0";

// Simple passthrough shader for grid lines (in NDC)
static const char *gridVertexSource = "#version 330 core\n"
                                      "layout (location = 0) in vec3 aPos;\n"
                                      "void main()\n"
                                      "{\n"
                                      "   gl_Position = vec4(aPos, 1.0);\n"
                                      "}\0";

static const char *gridFragmentSource = "#version 330 core\n"
                                        "out vec4 FragColor;\n"
                                        "uniform vec2 u_resolution;\n"
                                        "uniform float u_cellSize;\n"
                                        "uniform vec2 u_offset;\n"
                                        "uniform float u_aspect;\n"
                                        "uniform vec3 u_gridColor;\n"
                                        "uniform vec3 u_bgColor;\n"
                                        "void main()\n"
                                        "{\n"
                                        "    vec2 uv = gl_FragCoord.xy / u_resolution;\n"
                                        "    // Map to world coords: centered, y in [-1,1], x in [-aspect,aspect] \n"
                                        "    vec2 ndc = (uv - 0.5) * vec2(u_aspect * 2.0, 2.0);\n"
                                        "    vec2 world = ndc - u_offset;\n"
                                        "    vec2 gridPos = world / u_cellSize;\n"
                                        "    vec2 f = abs(fract(gridPos) - 0.5);\n"
                                        "    float line = min(f.x, f.y);\n"
                                        "    float fw = fwidth(gridPos.x) * 1.5;\n"
                                        "    float mask = 1.0 - smoothstep(0.0, max(0.0001, fw), line);\n"
                                        "    vec3 color = mix(u_bgColor, u_gridColor, mask);\n"
                                        "    FragColor = vec4(color, 1.0);\n"
                                        "}\n\0";

// Cell instanced shader (renders alive cells as filled quads slightly smaller than cell size)
static const char *cellVertexSource = "#version 330 core\n"
                                      "layout (location = 0) in vec2 aPos;\n"
                                      "layout (location = 1) in vec2 instancePos;\n"
                                      "uniform float u_cellSize;\n"
                                      "uniform vec2 u_offset;\n"
                                      "uniform float u_aspect;\n"
                                      "uniform float u_inner;\n"
                                      "void main()\n"
                                      "{\n"
                                      "    vec2 center = instancePos * u_cellSize + u_offset;\n"
                                      "    vec2 local = aPos * u_cellSize * u_inner;\n"
                                      "    float x = (center.x + local.x) / u_aspect;\n"
                                      "    float y = center.y + local.y;\n"
                                      "    gl_Position = vec4(x, y, 0.0, 1.0);\n"
                                      "}\0";

static const char *cellFragmentSource = "#version 330 core\n"
                                        "out vec4 FragColor;\n"
                                        "uniform vec3 u_cellColor;\n"
                                        "void main()\n"
                                        "{\n"
                                        "    FragColor = vec4(u_cellColor, 1.0);\n"
                                        "}\n\0";

// Draws 2D pixel-space quads. position = pixels (top-left origin), optional texture.
static const char *hudVertexSource =
    "#version 330 core\n"
    "layout(location = 0) in vec2 aPos;\n"
    "layout(location = 1) in vec2 aUV;\n"
    "uniform vec2 u_screen;\n" // (fbw, fbh)
    "out vec2 vUV;\n"
    "void main() {\n"
    "    vec2 ndc = (aPos / u_screen) * 2.0 - 1.0;\n"
    "    ndc.y = -ndc.y;\n" // flip Y: pixel (0,0) = top-left
    "    gl_Position = vec4(ndc, 0.0, 1.0);\n"
    "    vUV = aUV;\n"
    "}\0";

static const char *hudFragmentSource =
    "#version 330 core\n"
    "in vec2 vUV;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D u_tex;\n"
    "uniform bool  u_useTexture;\n"
    "uniform vec4  u_color;\n"
    "void main() {\n"
    "    FragColor = u_useTexture ? texture(u_tex, vUV) : u_color;\n"
    "}\0";

void GameLoop::initHUD()
{
    // --- VAO/VBO (6 vertices × (2 pos + 2 uv) floats, updated each quad) ---
    glGenVertexArrays(1, &hudVAO);
    glGenBuffers(1, &hudVBO);
    glBindVertexArray(hudVAO);
    glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // --- Compile HUD shader ---
    unsigned int v = compileShader(GL_VERTEX_SHADER, hudVertexSource);
    unsigned int f = compileShader(GL_FRAGMENT_SHADER, hudFragmentSource);
    hudShaderProgram = glCreateProgram();
    glAttachShader(hudShaderProgram, v);
    glAttachShader(hudShaderProgram, f);
    glLinkProgram(hudShaderProgram);
    glDeleteShader(v);
    glDeleteShader(f);

    // --- Load shape thumbnails ---
    loadHUDTextures();
}

GameLoop::GameLoop() : state_(NONE), game_(nullptr), VBO(0), VAO(0), shaderProgram(0)
{
    stepRequested = false;
    lastUpdateTime = 0.0;
    updateInterval = 0.2;
    generationCount = 0;
    spacePrev = false;
    nPrev = false;
    speedUpPrev = false;
    speedDownPrev = false;
    gridVBO = 0;
    gridVAO = 0;
    gridShaderProgram = 0;
    gridVertexCount = 0;
    cellSize = 0.1f;
    offsetX = 0.0f;
    offsetY = 0.0f;
    dragging = false;
    leftMouseDown = false;
    clickCandidate = false;
    lastMouseX = lastMouseY = 0.0;
    pressMouseX = pressMouseY = 0.0;
    cursorCellX = cursorCellY = 0;
    // Title state
    lastTitleGeneration = -1;
    lastTitleCursorX = INT_MIN;
    lastTitleCursorY = INT_MIN;
    lastTitleInterval = -1.0;
    lastTitleText = "";
    generationHistory.push_back(aliveCells);
}

// Replace the filesystem include and loadHUDTextures with this:
#include <dirent.h> // POSIX, no C++17 needed
#include <string.h> // strrchr
#include "stb_image.h"

void GameLoop::loadHUDTextures()
{
    const std::string shapesDir = "Shapes";

    DIR *dir = opendir(shapesDir.c_str());
    if (!dir)
    {
        std::cerr << "HUD: could not open shapes dir: " << shapesDir << std::endl;
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        std::string name = entry->d_name;

        // Skip hidden / . / ..
        if (name.empty() || name[0] == '.')
            continue;

        // Check extension
        const char *dot = strrchr(entry->d_name, '.');
        if (!dot)
            continue;
        std::string ext = dot; // e.g. ".png"
        if (ext != ".png" && ext != ".jpg" && ext != ".jpeg")
            continue;

        std::string fullPath = shapesDir + "/" + name;
        unsigned int tex = loadTextureFromFile(fullPath);
        if (tex != 0)
            hudShapes.push_back({name, tex});
    }

    closedir(dir);
}

bool GameLoop::mouseInHUD() const
{
    // only consider HUD region when HUD is visible
    if (!hudVisible)
        return false;
    // lastMouseX/lastMouseY use GLFW cursor coords with (0,0) top-left
    return (lastMouseX >= 0 && lastMouseY >= 0 && lastMouseX < HUD_WIDTH);
}

unsigned int GameLoop::loadTextureFromFile(const std::string &path)
{
    int w, h, ch;
    stbi_set_flip_vertically_on_load(false); // pixel (0,0) = top-left for HUD
    unsigned char *data = stbi_load(path.c_str(), &w, &h, &ch, 4);
    if (!data)
        return 0;
    unsigned int texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

#ifdef GL_EXT_texture_filter_anisotropic
    GLfloat maxAniso = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
    if (maxAniso > 0.0f)
    {
        GLfloat aniso = std::min(maxAniso, 8.0f);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
    }
#endif
    stbi_image_free(data);
    return texID;
}

void GameLoop::drawHUDQuad(float x, float y, float w, float h,
                           unsigned int texID,
                           float r, float g, float b, float a)
{
    // Two triangles, each vertex = (px, py, u, v)
    float verts[6][4] = {
        {x, y, 0, 0},
        {x + w, y, 1, 0},
        {x + w, y + h, 1, 1},
        {x, y, 0, 0},
        {x + w, y + h, 1, 1},
        {x, y + h, 0, 1},
    };

    glBindVertexArray(hudVAO);
    glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

    bool useTexture = (texID != 0);
    glUniform1i(glGetUniformLocation(hudShaderProgram, "u_useTexture"), useTexture ? 1 : 0);
    glUniform4f(glGetUniformLocation(hudShaderProgram, "u_color"), r, g, b, a);

    if (useTexture)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texID);
        glUniform1i(glGetUniformLocation(hudShaderProgram, "u_tex"), 0);
    }

    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void GameLoop::renderHUD()
{
    if (!game_ || hudShaderProgram == 0 || !hudVisible)
        return;

    int fbw, fbh;
    glfwGetFramebufferSize(game_->getWindow(), &fbw, &fbh);

    // restrict rendering to left HUD panel so it cannot overwrite pixels outside the panel

    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, HUD_WIDTH, fbh);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glUseProgram(hudShaderProgram);
    glUniform2f(glGetUniformLocation(hudShaderProgram, "u_screen"),
                (float)fbw, (float)fbh);

    // --- Background panel: left vertical panel ---
    drawHUDQuad(0, 0, (float)HUD_WIDTH, (float)fbh,
                0, 0.08f, 0.08f, 0.08f, 0.95f);

    // --- Vertical scrollable thumbnails list ---
    const int thumbW = HUD_WIDTH - HUD_PADDING * 2;
    const int thumbH = HUD_ITEM_H - HUD_PADDING;
    int y = HUD_PADDING - hudScrollOffset;

    for (int i = 0; i < (int)hudShapes.size(); i++)
    {
        // Skip items fully outside view
        if (y + HUD_ITEM_H < 0)
        {
            y += HUD_ITEM_H;
            continue;
        }
        if (y > fbh)
            break;

        // Highlight selected shape
        if (i == selectedHudShape)
            drawHUDQuad(2, y - 2, HUD_WIDTH - 4, thumbH + 4 + HUD_PADDING,
                        0, 0.2f, 0.6f, 0.2f, 0.9f);

        // Thumbnail
        drawHUDQuad(HUD_PADDING, y, thumbW, thumbH,
                    hudShapes[i].textureID,
                    1, 1, 1, 1);

        y += HUD_ITEM_H;
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
}

void GameLoop::scrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    (void)xoffset;
    GameLoop *inst = static_cast<GameLoop *>(glfwGetWindowUserPointer(window));
    if (!inst)
        return;

    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    // If cursor is over the left HUD panel, scroll thumbnails vertically
    if (inst->hudVisible && mx < GameLoop::HUD_WIDTH)
    {
        inst->hudScrollOffset -= (int)(yoffset * 20); // scroll speed
        int fbw = 1, fbh = 1;
        glfwGetFramebufferSize(window, &fbw, &fbh);
        int totalHeight = (int)inst->hudShapes.size() * HUD_ITEM_H;
        int maxScroll = std::max(0, totalHeight - fbh + HUD_PADDING);
        if (inst->hudScrollOffset < 0)
            inst->hudScrollOffset = 0;
        if (inst->hudScrollOffset > maxScroll)
            inst->hudScrollOffset = maxScroll;
        return;
    }
    else
    {
        float factor = (float)std::pow(1.15f, (float)yoffset);
        inst->cellSize *= factor;
        if (inst->cellSize < 0.0005f)
            inst->cellSize = 0.0005f;
        if (inst->cellSize > 10.0f)
            inst->cellSize = 10.0f;
    }
}

GameLoop::GameLoop(Game &other) : state_(PAUSED), game_(&other), VBO(0), VAO(0), shaderProgram(0)
{
    stepRequested = false;
    lastUpdateTime = 0.0;
    updateInterval = 0.2;
    generationCount = 0;
    spacePrev = false;
    nPrev = false;
    speedUpPrev = false;
    speedDownPrev = false;
    gridVBO = 0;
    gridVAO = 0;
    gridShaderProgram = 0;
    gridVertexCount = 0;
    // pan/zoom defaults
    cellSize = 0.1f;
    offsetX = 0.0f;
    offsetY = 0.0f;
    dragging = false;
    leftMouseDown = false;
    clickCandidate = false;
    lastMouseX = lastMouseY = 0.0;
    pressMouseX = pressMouseY = 0.0;
    cursorCellX = cursorCellY = 0;
    // Title state
    lastTitleGeneration = -1;
    lastTitleCursorX = INT_MIN;
    lastTitleCursorY = INT_MIN;
    lastTitleInterval = -1.0;
    lastTitleText = "";
    generationHistory.push_back(aliveCells);
}

GameLoop::~GameLoop()
{
    cleanupRendering();
}

void GameLoop::run()
{
    if (game_ == nullptr)
        return;

    GLFWwindow *window = game_->getWindow();
    initRendering();
    while (!glfwWindowShouldClose(window))
    {
        processInput();
        update();
        render();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void GameLoop::processInput()
{
    if (game_ == nullptr)
        return;

    GLFWwindow *window = game_->getWindow();

    // Escape: close
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Space: toggle running
    int spaceState = glfwGetKey(window, GLFW_KEY_SPACE);
    bool spaceDown = (spaceState == GLFW_PRESS || spaceState == GLFW_REPEAT);
    if (spaceDown && !spacePrev)
        state_ = (state_ & RUNNING) ? (State)(state_ & ~RUNNING) : (State)(state_ | RUNNING);
    spacePrev = spaceDown;

    // N: single step
    int nState = glfwGetKey(window, GLFW_KEY_N);
    bool nDown = (nState == GLFW_PRESS || nState == GLFW_REPEAT);
    if (nDown && !nPrev)
        stepRequested = true;
    nPrev = nDown;

    // R: reset to generation 0
    int rState = glfwGetKey(window, GLFW_KEY_R);
    static bool rPrev = false;
    bool rDown = (rState == GLFW_PRESS || rState == GLFW_REPEAT);
    if (rDown && !rPrev)
    {
        restoreGenerationState(0);
        lastUpdateTime = glfwGetTime();
        stepRequested = false;
    }
    rPrev = rDown;

    // B: rewind 10 generations
    int rewindState = glfwGetKey(window, GLFW_KEY_B);
    static bool rewindPrev = false;
    bool rewindDown = (rewindState == GLFW_PRESS || rewindState == GLFW_REPEAT);
    if (rewindDown && !rewindPrev)
    {
        rewindGenerations(10);
        lastUpdateTime = glfwGetTime();
        stepRequested = false;
    }
    rewindPrev = rewindDown;

    // =: speed up
    int speedUpState = glfwGetKey(window, GLFW_KEY_EQUAL);
    bool speedUpDown = (speedUpState == GLFW_PRESS || speedUpState == GLFW_REPEAT);
    if (speedUpDown && !speedUpPrev)
        updateInterval = std::max(0.01, updateInterval * 0.8);
    speedUpPrev = speedUpDown;

    // -: slow down
    int speedDownState = glfwGetKey(window, GLFW_KEY_MINUS);
    bool speedDownDown = (speedDownState == GLFW_PRESS || speedDownState == GLFW_REPEAT);
    if (speedDownDown && !speedDownPrev)
        updateInterval = std::min(2.0, updateInterval * 1.25);
    speedDownPrev = speedDownDown;

    // ↑ / ↓: navigate HUD shape list
    int upState = glfwGetKey(window, GLFW_KEY_UP);
    static bool upPrev = false;
    bool upDown = (upState == GLFW_PRESS || upState == GLFW_REPEAT);
    if (upDown && !upPrev && !hudShapes.empty())
    {
        selectedHudShape = (selectedHudShape <= 0) ? (int)hudShapes.size() - 1 : selectedHudShape - 1;
        int itemTop = selectedHudShape * HUD_ITEM_H;
        if (itemTop < hudScrollOffset)
            hudScrollOffset = itemTop;
    }
    upPrev = upDown;

    int downState = glfwGetKey(window, GLFW_KEY_DOWN);
    static bool downPrev = false;
    bool downDown = (downState == GLFW_PRESS || downState == GLFW_REPEAT);
    if (downDown && !downPrev && !hudShapes.empty())
    {
        selectedHudShape = (selectedHudShape + 1) % (int)hudShapes.size();
        int fbh = 1;
        glfwGetFramebufferSize(window, nullptr, &fbh);
        int itemBottom = (selectedHudShape + 1) * HUD_ITEM_H - hudScrollOffset;
        if (itemBottom > fbh)
            hudScrollOffset += itemBottom - fbh;
    }
    downPrev = downDown;

    // Enter: load selected HUD shape onto grid
    int enterState = glfwGetKey(window, GLFW_KEY_ENTER);
    static bool enterPrev = false;
    bool enterDown = (enterState == GLFW_PRESS);
    if (enterDown && !enterPrev && selectedHudShape >= 0 && selectedHudShape < (int)hudShapes.size())
        loadShapeByName(hudShapes[selectedHudShape].name);
    enterPrev = enterDown;

    // H: toggle HUD visibility
    int hState = glfwGetKey(window, GLFW_KEY_H);
    static bool hPrev = false;
    bool hDown = (hState == GLFW_PRESS || hState == GLFW_REPEAT);
    if (hDown && !hPrev)
    {
        hudVisible = !hudVisible;
    }
    hPrev = hDown;
}
void GameLoop::initRendering()
{
    setupGeometry();
    setupShaderProgram();
    setupGrid(50, 50);
    // initialize HUD resources (VAO/VBO/shader/textures)
    initHUD();

    // register callbacks so scrolling and dragging affect the grid
    if (game_)
    {
        GLFWwindow *window = game_->getWindow();
        if (window)
        {
            glfwSetWindowUserPointer(window, this);
            glfwSetScrollCallback(window, GameLoop::scrollCallback);
            glfwSetMouseButtonCallback(window, GameLoop::mouseButtonCallback);
            glfwSetCursorPosCallback(window, GameLoop::cursorPosCallback);
        }
    }
}

void GameLoop::setupGeometry()
{
    float vertices[] = {
        -0.5f, 0.5f, 0.0f,  // Top Left
        -0.5f, -0.5f, 0.0f, // Bottom Left
        0.5f, -0.5f, 0.0f,  // Bottom Right
        -0.5f, 0.5f, 0.0f,  // Top Left
        0.5f, -0.5f, 0.0f,  // Bottom Right
        0.5f, 0.5f, 0.0f    // Top Right
    };

    // 1. Create the IDs
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // 2. Bind VAO first! 🗂️ (Start recording)
    glBindVertexArray(VAO);

    // 3. Bind and fill the VBO 📦
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 4. Tell OpenGL how to read the data 🔍
    // (Location 0, 3 numbers per vertex, type float, don't normalize, size of 3 floats, start at 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // 5. Unbind to stay organized (Optional but good practice)
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void GameLoop::setupGrid(int /*rows*/, int /*cols*/)
{
    // Create a fullscreen quad (two triangles) in NDC
    float quad[] = {
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f};

    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);
    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    gridVertexCount = 6;

    // Compile procedural grid shader
    unsigned int v = compileShader(GL_VERTEX_SHADER, gridVertexSource);
    unsigned int f = compileShader(GL_FRAGMENT_SHADER, gridFragmentSource);
    gridShaderProgram = glCreateProgram();

    glAttachShader(gridShaderProgram, v);
    glAttachShader(gridShaderProgram, f);
    glLinkProgram(gridShaderProgram);
    int success = 0;
    glGetProgramiv(gridShaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        int logLen = 0;
        glGetProgramiv(gridShaderProgram, GL_INFO_LOG_LENGTH, &logLen);
        std::string infoLog(logLen > 0 ? logLen : 1, '\0');
        glGetProgramInfoLog(gridShaderProgram, logLen, NULL, &infoLog[0]);
        std::cerr << "Grid shader program link error:\n"
                  << infoLog << std::endl;
        glDeleteProgram(gridShaderProgram);
        gridShaderProgram = 0;
    }
    glDeleteShader(v);
    glDeleteShader(f);

    // --- setup cell instanced rendering (unit quad centered at 0)
    float cellQuad[] = {
        -0.5f, -0.5f,
        0.5f, -0.5f,
        0.5f, 0.5f,
        -0.5f, -0.5f,
        0.5f, 0.5f,
        -0.5f, 0.5f};

    glGenVertexArrays(1, &cellVAO);
    glGenBuffers(1, &cellVBO);
    glGenBuffers(1, &instanceVBO);

    glBindVertexArray(cellVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cellVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cellQuad), cellQuad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // instance buffer (empty initially)
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // compile cell shader
    unsigned int cv = compileShader(GL_VERTEX_SHADER, cellVertexSource);
    unsigned int cf = compileShader(GL_FRAGMENT_SHADER, cellFragmentSource);
    cellShaderProgram = glCreateProgram();
    glAttachShader(cellShaderProgram, cv);
    glAttachShader(cellShaderProgram, cf);
    glLinkProgram(cellShaderProgram);
    int ok = 0;
    glGetProgramiv(cellShaderProgram, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        int logLen = 0;
        glGetProgramiv(cellShaderProgram, GL_INFO_LOG_LENGTH, &logLen);
        std::string infoLog(logLen > 0 ? logLen : 1, '\0');
        glGetProgramInfoLog(cellShaderProgram, logLen, NULL, &infoLog[0]);
        std::cerr << "Cell shader program link error:\n"
                  << infoLog << std::endl;
        glDeleteProgram(cellShaderProgram);
        cellShaderProgram = 0;
    }
    glDeleteShader(cv);
    glDeleteShader(cf);
}

// pack cell coordinates into 64-bit key
long long GameLoop::cellKey(int x, int y) const
{
    return static_cast<long long>((static_cast<std::uint64_t>(static_cast<std::uint32_t>(x)) << 32) |
                                  static_cast<std::uint32_t>(y));
}

std::pair<int, int> GameLoop::screenPosToCell(double xpos, double ypos) const
{
    int fbw = 1, fbh = 1;
    if (game_ == nullptr)
        return {0, 0};

    glfwGetFramebufferSize(game_->getWindow(), &fbw, &fbh);
    if (fbw <= 0 || fbh <= 0)
        return {0, 0};

    double ux = xpos / (double)fbw;
    double uy = 1.0 - (ypos / (double)fbh);
    double ndcX = (ux - 0.5) * ((double)fbw / (double)fbh * 2.0);
    double ndcY = (uy - 0.5) * 2.0;
    double worldX = ndcX - offsetX;
    double worldY = ndcY - offsetY;

    int cellX = (int)std::llround(worldX / (double)cellSize);
    int cellY = (int)std::llround(worldY / (double)cellSize);
    return {cellX, cellY};
}

// Toggle a cell based on screen coordinates (cursor pos)
void GameLoop::toggleCellAtScreenPos(double xpos, double ypos)
{
    const std::pair<int, int> cell = screenPosToCell(xpos, ypos);
    const int cellX = cell.first;
    const int cellY = cell.second;

    setCellAlive(cellX, cellY, !isCellAlive(cellX, cellY));
}

// Update instance buffer with visible alive cells
void GameLoop::updateInstanceBuffer()
{
    GLFWwindow *window = game_->getWindow();
    int fbw = 1, fbh = 1;
    glfwGetFramebufferSize(window, &fbw, &fbh);
    if (fbw <= 0 || fbh <= 0)
        return;

    float aspect = (float)fbw / (float)fbh;
    // visible ndc ranges
    float ndcLeft = -aspect;
    float ndcRight = aspect;
    float ndcBottom = -1.0f;
    float ndcTop = 1.0f;

    // world bounds = ndc - offset
    float wminx = ndcLeft - offsetX;
    float wmaxx = ndcRight - offsetX;
    float wminy = ndcBottom - offsetY;
    float wmaxy = ndcTop - offsetY;

    int minX = (int)std::floor(wminx / cellSize) - 1;
    int maxX = (int)std::floor(wmaxx / cellSize) + 1;
    int minY = (int)std::floor(wminy / cellSize) - 1;
    int maxY = (int)std::floor(wmaxy / cellSize) + 1;

    std::vector<float> instances;
    instances.reserve(1024);

    // iterate aliveCells and push those inside bounds
    for (auto key : aliveCells)
    {
        int x = static_cast<int>(static_cast<std::int32_t>(static_cast<std::uint32_t>(key >> 32)));
        int y = static_cast<int>(static_cast<std::int32_t>(static_cast<std::uint32_t>(key & 0xffffffffULL)));
        if (x >= minX && x <= maxX && y >= minY && y <= maxY)
        {
            instances.push_back((float)x);
            instances.push_back((float)y);
        }
    }

    instanceCount = (int)(instances.size() / 2);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, instances.size() * sizeof(float), instances.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

unsigned int GameLoop::compileShader(unsigned int shaderType, const char *source)
{
    unsigned int shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    // Check compile status
    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        int logLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        std::string infoLog(logLen > 0 ? logLen : 1, '\0');
        glGetShaderInfoLog(shader, logLen, NULL, &infoLog[0]);
        std::cerr << "Shader compile error:\n"
                  << infoLog << std::endl;
    }

    return shader;
}

void GameLoop::setupShaderProgram()
{
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);

    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    // 3. Link Shaders into a Program
    shaderProgram = glCreateProgram(); // Remember: shaderProgram is a class member!
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // Check link status
    int success = 0;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        int logLen = 0;
        glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &logLen);
        std::string infoLog(logLen > 0 ? logLen : 1, '\0');
        glGetProgramInfoLog(shaderProgram, logLen, NULL, &infoLog[0]);
        std::cerr << "Shader program link error:\n"
                  << infoLog << std::endl;
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }

    // 4. Clean up (the program has the code now, so we don't need the individual shaders)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void GameLoop::cleanupRendering()
{
    if (shaderProgram != 0)
    {
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }
    if (VBO != 0)
    {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (VAO != 0)
    {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    if (gridVBO != 0)
    {
        glDeleteBuffers(1, &gridVBO);
        gridVBO = 0;
    }
    if (gridVAO != 0)
    {
        glDeleteVertexArrays(1, &gridVAO);
        gridVAO = 0;
    }
    if (gridShaderProgram != 0)
    {
        glDeleteProgram(gridShaderProgram);
        gridShaderProgram = 0;
    }
    if (cellVBO != 0)
    {
        glDeleteBuffers(1, &cellVBO);
        cellVBO = 0;
    }
    if (instanceVBO != 0)
    {
        glDeleteBuffers(1, &instanceVBO);
        instanceVBO = 0;
    }
    if (cellVAO != 0)
    {
        glDeleteVertexArrays(1, &cellVAO);
        cellVAO = 0;
    }
    if (cellShaderProgram != 0)
    {
        glDeleteProgram(cellShaderProgram);
        cellShaderProgram = 0;
    }

    // HUD resources
    if (hudVBO != 0)
    {
        glDeleteBuffers(1, &hudVBO);
        hudVBO = 0;
    }
    if (hudVAO != 0)
    {
        glDeleteVertexArrays(1, &hudVAO);
        hudVAO = 0;
    }
    if (hudShaderProgram != 0)
    {
        glDeleteProgram(hudShaderProgram);
        hudShaderProgram = 0;
    }

    // delete HUD textures
    if (!hudShapes.empty())
    {
        std::vector<unsigned int> texs;
        texs.reserve(hudShapes.size());
        for (auto &hs : hudShapes)
            texs.push_back(hs.textureID);
        if (!texs.empty())
            glDeleteTextures((GLsizei)texs.size(), texs.data());
        hudShapes.clear();
    }
}

void GameLoop::render()
{
    // Reset critical GL state each frame to avoid HUD state leaking
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    GLFWwindow *window = game_ ? game_->getWindow() : nullptr;
    int fbw = 1, fbh = 1;
    if (window)
    {
        glfwGetFramebufferSize(window, &fbw, &fbh);
        glUseProgram(shaderProgram);

        // 1. Find the uniform "mailboxes" 📬
        int scaleLocation = glGetUniformLocation(shaderProgram, "scale");
        int offsetLocation = glGetUniformLocation(shaderProgram, "offset");
        int aspectLocation = glGetUniformLocation(shaderProgram, "aspect");

        // 2. Send the data: Make it 10% of its original size, and move it slightly right
        glUniform1f(scaleLocation, 0.1f);
        glUniform2f(offsetLocation, 0.2f, 0.0f);

        // compute aspect from framebuffer size
        float aspect = (fbh == 0) ? 1.0f : (float)fbw / (float)fbh;
        glUniform1f(aspectLocation, aspect);

        // 3. Draw the square
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // Draw procedural grid on top (fullscreen quad)
    if (gridShaderProgram != 0 && gridVAO != 0)
    {
        glUseProgram(gridShaderProgram);
        // uniforms
        int resLoc = glGetUniformLocation(gridShaderProgram, "u_resolution");
        int cellLoc = glGetUniformLocation(gridShaderProgram, "u_cellSize");
        int offLoc = glGetUniformLocation(gridShaderProgram, "u_offset");
        int aspLoc = glGetUniformLocation(gridShaderProgram, "u_aspect");
        int colLoc = glGetUniformLocation(gridShaderProgram, "u_gridColor");
        int bgLoc = glGetUniformLocation(gridShaderProgram, "u_bgColor");

        glUniform2f(resLoc, (float)fbw, (float)fbh);
        glUniform1f(cellLoc, cellSize);
        glUniform2f(offLoc, offsetX, offsetY);
        float aspect = (fbh == 0) ? 1.0f : (float)fbw / (float)fbh;
        glUniform1f(aspLoc, aspect);
        glUniform3f(colLoc, 0.15f, 0.15f, 0.15f);
        glUniform3f(bgLoc, 0.2f, 0.3f, 0.3f);

        glBindVertexArray(gridVAO);
        glDrawArrays(GL_TRIANGLES, 0, gridVertexCount);
    }

    // Update instance buffer and draw alive cells
    if (cellShaderProgram != 0 && cellVAO != 0)
    {
        updateInstanceBuffer();
        if (instanceCount > 0)
        {
            glUseProgram(cellShaderProgram);
            int cellSizeLoc = glGetUniformLocation(cellShaderProgram, "u_cellSize");
            int offLoc = glGetUniformLocation(cellShaderProgram, "u_offset");
            int aspLoc = glGetUniformLocation(cellShaderProgram, "u_aspect");
            int innerLoc = glGetUniformLocation(cellShaderProgram, "u_inner");
            int colorLoc = glGetUniformLocation(cellShaderProgram, "u_cellColor");

            glUniform1f(cellSizeLoc, cellSize);
            glUniform2f(offLoc, offsetX, offsetY);
            float aspect2 = (fbh == 0) ? 1.0f : (float)fbw / (float)fbh;
            glUniform1f(aspLoc, aspect2);
            glUniform1f(innerLoc, 0.85f); // larger inset: make live cells bigger inside each grid square
            glUniform3f(colorLoc, 0.0f, 0.8f, 0.0f);

            glBindVertexArray(cellVAO);
            glDrawArraysInstanced(GL_TRIANGLES, 0, 6, instanceCount);
        }
    }
    renderHUD();

    // Final safety reset so HUD state never bleeds into next frame
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(0);
    glUseProgram(0);
}

void GameLoop::update()
{
    if (game_ == nullptr)
        return;

    // Update cursor cell position
    double xpos, ypos;
    GLFWwindow *window = game_->getWindow();
    glfwGetCursorPos(window, &xpos, &ypos);
    updateCursorCell(xpos, ypos);

    // Update window title only if cursor or generation changed (avoid expensive string operations every frame)
    updateTitleIfChanged();

    double now = glfwGetTime();

    // Advance generation when requested or when running and interval elapsed
    if (stepRequested || ((state_ & RUNNING) && (now - lastUpdateTime >= updateInterval)))
    {
        updateGeneration();
        lastUpdateTime = now;
        stepRequested = false;
    }
}

void GameLoop::updateCellsForNextGeneration()
{
    if (aliveCells.empty())
        return;

    std::unordered_set<long long> candidates = aliveCells;
    for (long long key : aliveCells)
    {
        int x = static_cast<int>(static_cast<std::int32_t>(static_cast<std::uint32_t>(key >> 32)));
        int y = static_cast<int>(static_cast<std::int32_t>(static_cast<std::uint32_t>(key & 0xffffffffULL)));

        for (int dx = -1; dx <= 1; ++dx)
        {
            for (int dy = -1; dy <= 1; ++dy)
            {
                if (dx == 0 && dy == 0)
                    continue;
                candidates.insert(cellKey(x + dx, y + dy));
            }
        }
    }

    std::unordered_set<long long> nextAliveCells;
    for (long long key : candidates)
    {
        int x = static_cast<int>(static_cast<std::int32_t>(static_cast<std::uint32_t>(key >> 32)));
        int y = static_cast<int>(static_cast<std::int32_t>(static_cast<std::uint32_t>(key & 0xffffffffULL)));
        int neighbors = countAliveNeighbors(x, y);
        bool alive = isCellAlive(x, y);

        if ((alive && (neighbors == 2 || neighbors == 3)) || (!alive && neighbors == 3))
            nextAliveCells.insert(key);
    }

    aliveCells.swap(nextAliveCells);
}

void GameLoop::updateGeneration()
{
    updateCellsForNextGeneration();
    generationCount++;
    if (generationHistory.size() > static_cast<std::size_t>(generationCount))
        generationHistory.resize(static_cast<std::size_t>(generationCount));
    generationHistory.push_back(aliveCells);
}

// // --- GLFW callbacks -------------------------------------------------
// void GameLoop::scrollCallback(GLFWwindow *window, double xoffset, double yoffset)
// {
//     (void)xoffset;
//     (void)yoffset;
//     GameLoop *inst = static_cast<GameLoop *>(glfwGetWindowUserPointer(window));
//     if (!inst)
//         return;

//     // Zoom: increase cell size when scrolling up (yoffset > 0)
//     float factor = (float)std::pow(1.15f, (float)yoffset);
//     inst->cellSize *= factor;
//     if (inst->cellSize < 0.0005f)
//         inst->cellSize = 0.0005f;
//     if (inst->cellSize > 10.0f)
//         inst->cellSize = 10.0f;
// }

void GameLoop::mouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    (void)mods;
    GameLoop *inst = static_cast<GameLoop *>(glfwGetWindowUserPointer(window));
    if (!inst)
        return;
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            glfwGetCursorPos(window, &inst->lastMouseX, &inst->lastMouseY);
            inst->pressMouseX = inst->lastMouseX;
            inst->pressMouseY = inst->lastMouseY;
            inst->leftMouseDown = true;
            inst->clickCandidate = true;
            inst->dragging = true;
        }
        else if (action == GLFW_RELEASE)
        {
            if (inst->clickCandidate)
            {
                double dx = inst->lastMouseX - inst->pressMouseX;
                double dy = inst->lastMouseY - inst->pressMouseY;
                if ((dx * dx + dy * dy) <= 9.0)
                {
                    // If click happened inside HUD, interpret as thumbnail selection
                    if (inst->mouseInHUD())
                    {
                        // Compute which thumbnail (vertical list) was clicked using Y coordinate
                        double cy = inst->lastMouseY;
                        int index = (int)std::floor((cy - HUD_PADDING + inst->hudScrollOffset) / (double)HUD_ITEM_H);
                        if (index >= 0 && index < (int)inst->hudShapes.size())
                        {
                            inst->selectedHudShape = index;
                            inst->loadShapeByName(inst->hudShapes[index].name);
                        }
                    }
                    else
                    {
                        inst->toggleCellAtScreenPos(inst->lastMouseX, inst->lastMouseY);
                    }
                }
            }
            inst->leftMouseDown = false;
            inst->clickCandidate = false;
            inst->dragging = false;
        }
    }
}

void GameLoop::cursorPosCallback(GLFWwindow *window, double xpos, double ypos)
{
    GameLoop *inst = static_cast<GameLoop *>(glfwGetWindowUserPointer(window));
    if (!inst)
        return;
    if (!inst->dragging)
        return;

    int fbw = 1, fbh = 1;
    glfwGetFramebufferSize(window, &fbw, &fbh);
    if (fbw <= 0 || fbh <= 0)
    {
        inst->lastMouseX = xpos;
        inst->lastMouseY = ypos;
        return;
    }

    double dx = xpos - inst->lastMouseX;
    double dy = ypos - inst->lastMouseY;

    if (inst->leftMouseDown)
    {
        double totalDx = xpos - inst->pressMouseX;
        double totalDy = ypos - inst->pressMouseY;
        if ((totalDx * totalDx + totalDy * totalDy) > 9.0)
            inst->clickCandidate = false;
    }

    float aspect = (float)fbw / (float)fbh;
    // Convert pixel delta to world delta using the same mapping as the grid shader
    float worldDx = (float)dx / (float)fbw * 2.0f * aspect;
    float worldDy = -(float)dy / (float)fbh * 2.0f;

    inst->offsetX += worldDx;
    inst->offsetY += worldDy;

    inst->lastMouseX = xpos;
    inst->lastMouseY = ypos;
}

void GameLoop::updateCursorCell(double xpos, double ypos)
{
    const std::pair<int, int> cell = screenPosToCell(xpos, ypos);
    cursorCellX = cell.first;
    cursorCellY = cell.second;
}

void GameLoop::syncGenerationZeroSnapshot()
{
    if (generationCount != 0)
        return;

    if (generationHistory.empty())
        generationHistory.push_back(aliveCells);
    else
        generationHistory[0] = aliveCells;
}

void GameLoop::restoreGenerationState(int targetGeneration)
{
    if (targetGeneration < 0)
        targetGeneration = 0;

    if (generationHistory.empty())
    {
        aliveCells.clear();
        generationCount = 0;
        generationHistory.push_back(aliveCells);
        return;
    }

    std::size_t targetIndex = static_cast<std::size_t>(targetGeneration);
    if (targetIndex >= generationHistory.size())
        targetIndex = generationHistory.size() - 1;

    aliveCells = generationHistory[targetIndex];
    generationCount = static_cast<int>(targetIndex);
    generationHistory.resize(targetIndex + 1);
}

void GameLoop::rewindGenerations(int steps)
{
    if (steps <= 0)
        return;

    int targetGeneration = generationCount - steps;
    if (targetGeneration < 0)
        targetGeneration = 0;

    restoreGenerationState(targetGeneration);
}

bool GameLoop::isCellAlive(int x, int y) const
{
    long long key = cellKey(x, y);
    return aliveCells.count(key) > 0;
}

int GameLoop::countAliveNeighbors(int x, int y) const
{
    int count = 0;
    for (int dx = -1; dx <= 1; ++dx)
    {
        for (int dy = -1; dy <= 1; ++dy)
        {
            if (dx == 0 && dy == 0)
                continue;
            if (isCellAlive(x + dx, y + dy))
                count++;
        }
    }
    return count;
}

void GameLoop::setCellAlive(int x, int y, bool alive)
{
    long long key = cellKey(x, y);
    if (alive)
    {
        aliveCells.insert(key);
    }
    else
    {
        aliveCells.erase(key);
    }

    if (generationCount == 0)
        syncGenerationZeroSnapshot();
}

void GameLoop::updateTitleIfChanged()
{
    if (game_ == nullptr)
        return;

    std::string newTitle = "Conway's Game of Life - Gen " + std::to_string(generationCount) +
                           " | Cursor: (" + std::to_string(cursorCellX) + ", " + std::to_string(cursorCellY) + ")" +
                           " | Speed: " + std::to_string(updateInterval) + "s";

    // Only update if title actually changed
    if (newTitle != lastTitleText)
    {
        lastTitleText = newTitle;
        GLFWwindow *window = game_->getWindow();
        glfwSetWindowTitle(window, newTitle.c_str());
    }
}

void GameLoop::loadShapeFromImage(const std::string &imagePath)
{
    applyLoadedShape(ShapeLoader::loadShapeFromPath(imagePath));
}

void GameLoop::loadShapeByName(const std::string &imageName)
{
    applyLoadedShape(ShapeLoader::loadShapeByName(imageName));
}

void GameLoop::applyLoadedShape(const std::vector<std::pair<int, int>> &coords)
{
    if (coords.empty())
        return;

    int minX = coords.front().first;
    int maxX = coords.front().first;
    int minY = coords.front().second;
    int maxY = coords.front().second;

    for (const auto &coord : coords)
    {
        if (coord.first < minX)
            minX = coord.first;
        if (coord.first > maxX)
            maxX = coord.first;
        if (coord.second < minY)
            minY = coord.second;
        if (coord.second > maxY)
            maxY = coord.second;
    }

    const int centerX = (minX + maxX) / 2;
    const int centerY = (minY + maxY) / 2;

    aliveCells.clear();
    generationHistory.clear();
    generationCount = 0;

    for (const auto &coord : coords)
        aliveCells.insert(cellKey(coord.first - centerX, coord.second - centerY));

    generationHistory.push_back(aliveCells);
    lastTitleText.clear();
    updateTitleIfChanged();
}