#pragma once

#include "poker/core/Random.hpp"

#include <cstdint>

namespace poker::ui {

struct AppOptions {
    std::uint64_t seed = 0;
    bool colors = true;
};

class App {
public:
    explicit App(AppOptions options);
    int run();

private:
    void playGame();
    void showRules();

    AppOptions options_;
    Rng rng_;
};

}
