#pragma once

#include "poker/analysis/Advisor.hpp"
#include "poker/core/Random.hpp"

namespace poker::ui {

void printRecommendation(const Recommendation& rec);

void runEquityCalculator(Rng& rng);

void runSituationAnalysis(Rng& rng);

void showPreflopChart();

void runBotSimulation(Rng& rng);

}
