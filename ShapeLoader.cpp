#include "include/ShapeLoader.hpp"
#include <algorithm>
#include <iostream>
#include <queue>
#include <cmath>

// If precomputed shapes were generated, include them and use at runtime.
#include "PrecomputedShapes.hpp"

// Single-header image loading library (header-only implementation)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace
{
    constexpr const char *kShapesDirectory = "/home/mbousset/Desktop/Game_of_life/Shapes/";

    std::string resolveShapePath(const std::string &imageName)
    {
        if (imageName.find('/') != std::string::npos)
            return imageName;

        std::string fileName = imageName;
        if (fileName.find('.') == std::string::npos)
            fileName += ".png";

        return std::string(kShapesDirectory) + fileName;
    }

    bool isLiveCellPixel(unsigned char r, unsigned char g, unsigned char b)
    {
        const int red = static_cast<int>(r);
        const int green = static_cast<int>(g);
        const int blue = static_cast<int>(b);
        const int maxChannel = std::max({red, green, blue});
        const int minChannel = std::min({red, green, blue});
        const int chroma = maxChannel - minChannel;

        // Live cells are yellow; background and grid lines are gray.
        return red > 150 && green > 120 && blue < 180 && chroma > 35 &&
               (red - blue) > 40 && (green - blue) > 30 && std::abs(red - green) < 120;
    }

    struct Component
    {
        double centerX;
        double centerY;
    };

    std::vector<Component> extractLiveComponents(unsigned char *imageData, int width, int height)
    {
        std::vector<Component> components;
        std::vector<unsigned char> visited(static_cast<size_t>(width * height), 0);
        const int directions[8][2] = {
            {-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}};

        auto pixelIndex = [width](int x, int y)
        {
            return (y * width + x) * 4;
        };

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                const int index = y * width + x;
                if (visited[static_cast<size_t>(index)])
                    continue;

                const int base = pixelIndex(x, y);
                if (!isLiveCellPixel(imageData[base], imageData[base + 1], imageData[base + 2]))
                    continue;

                std::queue<std::pair<int, int>> pending;
                pending.push({x, y});
                visited[static_cast<size_t>(index)] = 1;

                long long sumX = 0;
                long long sumY = 0;
                long long count = 0;

                while (!pending.empty())
                {
                    const std::pair<int, int> current = pending.front();
                    pending.pop();
                    const int cx = current.first;
                    const int cy = current.second;

                    sumX += cx;
                    sumY += cy;
                    ++count;

                    for (const auto &direction : directions)
                    {
                        const int nx = cx + direction[0];
                        const int ny = cy + direction[1];
                        if (nx < 0 || ny < 0 || nx >= width || ny >= height)
                            continue;

                        const int neighborIndex = ny * width + nx;
                        if (visited[static_cast<size_t>(neighborIndex)])
                            continue;

                        const int neighborBase = pixelIndex(nx, ny);
                        if (!isLiveCellPixel(imageData[neighborBase], imageData[neighborBase + 1], imageData[neighborBase + 2]))
                            continue;

                        visited[static_cast<size_t>(neighborIndex)] = 1;
                        pending.push({nx, ny});
                    }
                }

                if (count > 0)
                    components.push_back({static_cast<double>(sumX) / count, static_cast<double>(sumY) / count});
            }
        }

        return components;
    }

    std::vector<double> clusterAxisValues(const std::vector<double> &values)
    {
        std::vector<double> sortedValues = values;
        std::sort(sortedValues.begin(), sortedValues.end());

        std::vector<double> clusters;
        for (double value : sortedValues)
        {
            if (clusters.empty() || std::abs(value - clusters.back()) > 2.5)
                clusters.push_back(value);
            else
                clusters.back() = (clusters.back() + value) * 0.5;
        }

        return clusters;
    }

    double estimateGridPitch(const std::vector<double> &clusterCenters)
    {
        if (clusterCenters.size() < 2)
            return 1.0;

        std::vector<double> diffs;
        for (size_t i = 1; i < clusterCenters.size(); ++i)
        {
            const double diff = clusterCenters[i] - clusterCenters[i - 1];
            if (diff > 0.5)
                diffs.push_back(diff);
        }

        if (diffs.empty())
            return 1.0;

        std::sort(diffs.begin(), diffs.end());
        return diffs[diffs.size() / 2];
    }

    int quantizeToGridIndex(double origin, double pitch, double value)
    {
        if (pitch <= 0.0)
            pitch = 1.0;
        return static_cast<int>(std::llround((value - origin) / pitch));
    }

    std::vector<std::pair<int, int>> loadShapeFromImageData(unsigned char *imageData, int width, int height)
    {
        std::vector<std::pair<int, int>> shape;
        const std::vector<Component> components = extractLiveComponents(imageData, width, height);

        if (components.empty())
            return shape;

        std::vector<double> xCenters;
        std::vector<double> yCenters;
        xCenters.reserve(components.size());
        yCenters.reserve(components.size());

        for (const auto &component : components)
        {
            xCenters.push_back(component.centerX);
            yCenters.push_back(component.centerY);
        }

        const std::vector<double> clusteredX = clusterAxisValues(xCenters);
        const std::vector<double> clusteredY = clusterAxisValues(yCenters);
        const double originX = clusteredX.front();
        const double originY = clusteredY.front();
        const double pitchX = estimateGridPitch(clusteredX);
        const double pitchY = estimateGridPitch(clusteredY);

        shape.reserve(components.size());
        for (const auto &component : components)
        {
            const int gridX = quantizeToGridIndex(originX, pitchX, component.centerX);
            const int gridY = quantizeToGridIndex(originY, pitchY, component.centerY);
            shape.push_back({gridX, -gridY});
        }

        return shape;
    }
}

std::vector<std::pair<int, int>> ShapeLoader::loadShapeFromPath(const std::string &imagePath)
{
    std::vector<std::pair<int, int>> shape;

    int width, height, channels;
    unsigned char *imageData = stbi_load(imagePath.c_str(), &width, &height, &channels, 4);

    if (!imageData)
    {
        std::cerr << "Failed to load image: " << imagePath << std::endl;
        return shape;
    }

    std::cerr << "Loaded image " << imagePath << " with dimensions " << width << "x" << height << std::endl;

    shape = loadShapeFromImageData(imageData, width, height);

    stbi_image_free(imageData);

    std::cerr << "Loaded shape from " << imagePath << " with " << shape.size() << " alive cells" << std::endl;

    return shape;
}

std::vector<std::pair<int, int>> ShapeLoader::loadShapeByName(const std::string &imageName)
{
    // Try to find a precomputed entry by basename (filename only)
    std::string base = imageName;
    // strip any path
    size_t pos = base.find_last_of("/\\");
    if (pos != std::string::npos)
        base = base.substr(pos + 1);

    // try as-is
    auto it = PRECOMPUTED_SHAPES.find(base);
    if (it != PRECOMPUTED_SHAPES.end())
        return it->second;

    // if no extension, try adding .png
    if (base.find('.') == std::string::npos)
    {
        std::string withPng = base + ".png";
        it = PRECOMPUTED_SHAPES.find(withPng);
        if (it != PRECOMPUTED_SHAPES.end())
            return it->second;
    }

    // fallback to dynamic loading
    return loadShapeFromPath(resolveShapePath(imageName));
}
