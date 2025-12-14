#include "Game/Game.h"
#include <iostream>
#include <exception>

int main(int argc, char* argv[]) {
    try {
        Game::Game game;
        
        if (!game.Initialize()) {
            std::cerr << "Failed to initialize game" << std::endl;
            return 1;
        }
        
        game.Run();
        game.Shutdown();
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}
