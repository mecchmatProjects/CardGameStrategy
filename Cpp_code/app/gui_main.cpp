#include "poker/gui/PokerApp.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>
#include <string>

int main(int argc, char* argv[]) {
    std::uint64_t seed = std::random_device{}();
    bool demo = false;
    int level = 2;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--demo") == 0) demo = true;
        if (std::strcmp(argv[i], "--level") == 0 && i + 1 < argc) level = std::atoi(argv[++i]);
        if (std::strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            try {
                seed = std::stoull(argv[++i]);
            } catch (const std::exception&) {
                std::cerr << "Некоректний seed: " << argv[i] << '\n';
                return 1;
            }
        }
    }

    poker::gui::PokerApp app(seed, demo, level - 1);
    app.run();
    return 0;
}
