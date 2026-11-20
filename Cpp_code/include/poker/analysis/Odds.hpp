#pragma once

#include "poker/core/Card.hpp"

#include <string>
#include <vector>

namespace poker {

double potOdds(int toCall, int pot);

double callEV(double equity, int pot, int toCall);

double bluffBreakEven(int bet, int pot);

struct OutsInfo {
    std::vector<Card> outs;
    int unseen = 0;
    double nextCard = 0.0;
    double byRiver = 0.0;
    int ruleOf2And4 = 0;
};

OutsInfo countOuts(const HoleCards& hole, const std::vector<Card>& board);

std::string percentString(double fraction);

}
