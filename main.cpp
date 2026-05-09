#include "include/Game.hpp"
#include "include/GameLoop.hpp"
#include <algorithm>
#include <cctype>
#include <dirent.h>
#include <iostream>
#include <string>
#include <vector>

namespace
{
    const char *kColorBlue = "\033[34m";
    const char *kColorGreen = "\033[32m";
    const char *kColorYellow = "\033[33m";
    const char *kColorReset = "\033[0m";

    bool hasSupportedImageExtension(const std::string &name)
    {
        const size_t dot = name.find_last_of('.');
        if (dot == std::string::npos)
            return false;

        std::string ext = name.substr(dot);
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c)
                       { return static_cast<char>(std::tolower(c)); });
        return ext == ".png" || ext == ".jpg" || ext == ".jpeg";
    }

    std::vector<std::string> listShapes()
    {
        std::vector<std::string> shapes;
        DIR *dir = opendir("Shapes");
        if (!dir)
        {
            std::cerr << kColorYellow << "Warning: could not open ./Shapes directory.\n" << kColorReset;
            return shapes;
        }

        dirent *entry;
        while ((entry = readdir(dir)) != nullptr)
        {
            const std::string name = entry->d_name;
            if (!name.empty() && name[0] != '.' && hasSupportedImageExtension(name))
                shapes.push_back(name);
        }

        closedir(dir);
        std::sort(shapes.begin(), shapes.end());
        return shapes;
    }

    void printHelp()
    {
        std::cout << kColorBlue << "Game of Life - Help\n"
                  << kColorReset;
        std::cout << kColorGreen << "Usage:\n" << kColorReset;
        std::cout << "  ./GameOfLife [shape_name]\n";
        std::cout << "  ./GameOfLife -help\n\n";

        std::cout << kColorGreen << "Keyboard controls:\n" << kColorReset;
        std::cout << "  ESC        Quit\n";
        std::cout << "  SPACE      Start/Pause simulation\n";
        std::cout << "  N          Next generation (single step)\n";
        std::cout << "  R          Reset to generation 0\n";
        std::cout << "  B          Rewind 10 generations\n";
        std::cout << "  =          Increase simulation speed\n";
        std::cout << "  -          Decrease simulation speed\n";
        std::cout << "  H          Show/Hide shape panel (HUD)\n";
        std::cout << "  UP/DOWN    Navigate shape list\n";
        std::cout << "  ENTER      Load selected shape\n\n";

        std::cout << kColorGreen << "Mouse controls:\n" << kColorReset;
        std::cout << "  Left click         Toggle one cell\n";
        std::cout << "  Left click + drag  Pan the grid\n";
        std::cout << "  Mouse wheel        Zoom grid\n";
        std::cout << "  Mouse wheel (HUD)  Scroll shape list when cursor is on HUD\n";
    }

    bool isHelpRequest(const std::string &arg)
    {
        return arg == "-help" || arg == "--help" || arg == "-h" || arg == "help";
    }

    bool isPositiveNumber(const std::string &value)
    {
        return !value.empty() &&
               std::all_of(value.begin(), value.end(), [](unsigned char c)
                           { return std::isdigit(c) != 0; });
    }

    bool chooseStartupMode(std::string &shapeName)
    {
        while (true)
        {
            std::cout << kColorBlue << "\nGame of Life CLI\n" << kColorReset;
            std::cout << "Press " << kColorGreen << "[Enter]" << kColorReset << " to start with default shape\n";
            std::cout << "Type " << kColorGreen << "h" << kColorReset << " for help, "
                      << kColorGreen << "c" << kColorReset << " to choose shape, "
                      << kColorGreen << "q" << kColorReset << " to quit: ";

            std::string input;
            if (!std::getline(std::cin, input))
                return false;

            if (input.empty())
                return true;
            if (input == "q" || input == "Q")
                return false;
            if (input == "h" || input == "H")
            {
                std::cout << '\n';
                printHelp();
                continue;
            }
            if (input == "c" || input == "C")
            {
                const std::vector<std::string> shapes = listShapes();
                if (shapes.empty())
                {
                    std::cout << kColorYellow << "No shapes found in ./Shapes\n" << kColorReset;
                    continue;
                }

                std::cout << kColorGreen << "Available shapes:\n" << kColorReset;
                for (size_t i = 0; i < shapes.size(); ++i)
                    std::cout << "  " << (i + 1) << ". " << shapes[i] << '\n';

                std::cout << "Choose a number or exact shape file name: ";
                std::string choice;
                if (!std::getline(std::cin, choice))
                    return false;
                if (choice.empty())
                    continue;

                if (isPositiveNumber(choice))
                {
                    try
                    {
                        const size_t index = std::stoul(choice);
                        if (index >= 1 && index <= shapes.size())
                        {
                            shapeName = shapes[index - 1];
                            std::cout << kColorGreen << "Selected: " << shapeName << kColorReset << '\n';
                            return true;
                        }
                    }
                    catch (const std::exception &)
                    {
                    }
                }

                const std::vector<std::string>::const_iterator it = std::find(shapes.begin(), shapes.end(), choice);
                if (it != shapes.end())
                {
                    shapeName = *it;
                    std::cout << kColorGreen << "Selected: " << shapeName << kColorReset << '\n';
                    return true;
                }

                std::cout << kColorYellow << "Invalid shape selection.\n" << kColorReset;
                continue;
            }

            std::cout << kColorYellow << "Unknown option. Type h for help.\n" << kColorReset;
        }
    }
}

int main(int argc, char **argv)
{
    std::string shapeName = "Gosper_glider_gun.png";

    if (argc > 1)
    {
        const std::string firstArg = argv[1];
        if (isHelpRequest(firstArg))
        {
            printHelp();
            return 0;
        }
        shapeName = firstArg;
    }
    else if (!chooseStartupMode(shapeName))
    {
        return 0;
    }

    try
    {
        Game game;
        GameLoop GLoop(game);

        game.start();
        GLoop.loadShapeByName(shapeName);
        GLoop.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }

    return 0;
}
