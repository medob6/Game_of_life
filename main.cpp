#include "include/Game.hpp"
#include "include/GameLoop.hpp"

int main(int argc, char **argv)
{
    try
    {
        Game game;
        GameLoop GLoop(game);

        game.start();
        const std::string shapeName = (argc > 1) ? argv[1] : "Gosper_glider_gun.png";
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
