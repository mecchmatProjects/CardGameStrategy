#include "poker/ui/App.hpp"

#include <cstring>
#include <iostream>
#include <random>
#include <string>

namespace {

void printUsage(const char* program) {
    std::cout << "Використання: " << program << " [--seed N] [--no-color]\n"
              << "  --seed N     фіксований seed (однакові роздачі при кожному запуску)\n"
              << "  --no-color   вимкнути кольоровий вивід\n";
}

}

int main(int argc, char* argv[]) {
    poker::ui::AppOptions options;
    options.seed = std::random_device{}();

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            try {
                options.seed = std::stoull(argv[++i]);
            } catch (const std::exception&) {
                std::cerr << "Некоректний seed: " << argv[i] << '\n';
                return 1;
            }
        } else if (std::strcmp(argv[i], "--no-color") == 0) {
            options.colors = false;
        } else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            printUsage(argv[0]);
            return 0;
        } else {
            std::cerr << "Невідомий аргумент: " << argv[i] << '\n';
            printUsage(argv[0]);
            return 1;
        }
    }

    poker::ui::App app(options);
    return app.run();
}
