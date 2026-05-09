#include <dirent.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

// Include the shape loader implementation to reuse its image parsing logic.
#include "../ShapeLoader.cpp"

bool hasImageExt(const std::string &name)
{
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return lower.find(".png") != std::string::npos || lower.find(".jpg") != std::string::npos || lower.find(".jpeg") != std::string::npos;
}

int main()
{
    const std::string dir = "Shapes";
    DIR *d = opendir(dir.c_str());
    if (!d)
    {
        std::cerr << "Could not open Shapes directory: " << dir << std::endl;
        return 1;
    }

    std::vector<std::string> files;
    struct dirent *entry;
    while ((entry = readdir(d)) != nullptr)
    {
        std::string name = entry->d_name;
        if (name.empty() || name[0] == '.')
            continue;
        if (!hasImageExt(name))
            continue;
        files.push_back(name);
    }
    closedir(d);

    std::sort(files.begin(), files.end());

    std::ofstream out("include/PrecomputedShapes.hpp");
    if (!out)
    {
        std::cerr << "Failed to open output header file" << std::endl;
        return 1;
    }

    out << "#pragma once\n";
    out << "#include <unordered_map>\n";
    out << "#include <vector>\n";
    out << "#include <utility>\n";
    out << "#include <string>\n\n";
    out << "static const std::unordered_map<std::string, std::vector<std::pair<int,int>>> PRECOMPUTED_SHAPES = {\n";

    for (const auto &f : files)
    {
        std::string path = dir + "/" + f;
        std::vector<std::pair<int, int>> shape = ShapeLoader::loadShapeFromPath(path);
        out << "    {\"" << f << "\", {";
        bool first = true;
        for (auto &p : shape)
        {
            if (!first)
                out << ", ";
            out << "{" << p.first << ", " << p.second << "}";
            first = false;
        }
        out << "}},\n";
    }

    out << "};\n";
    out.close();

    std::cout << "Wrote include/PrecomputedShapes.hpp with " << files.size() << " entries.\n";
    return 0;
}
