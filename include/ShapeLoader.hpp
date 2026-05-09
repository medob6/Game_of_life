#pragma once

#include <string>
#include <vector>
#include <utility>

class ShapeLoader
{
public:
    // Load an image file and return alive cell coordinates (x, y)
    static std::vector<std::pair<int, int>> loadShapeFromPath(const std::string &imagePath);

    // Load an image from the Shapes folder using only the image name.
    static std::vector<std::pair<int, int>> loadShapeByName(const std::string &imageName);
};
