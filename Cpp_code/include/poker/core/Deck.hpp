#pragma once

#include "poker/core/Card.hpp"
#include "poker/core/Random.hpp"

#include <cstddef>
#include <vector>

namespace poker {

class Deck {
public:
    Deck();
    explicit Deck(CardMask excluded);

    void reset(CardMask excluded = 0);

    void shuffle(Rng& rng);

    Card deal();

    std::size_t size() const { return cards_.size() - top_; }
    bool empty() const { return size() == 0; }

private:
    std::vector<Card> cards_;
    std::size_t top_ = 0;
};

}
