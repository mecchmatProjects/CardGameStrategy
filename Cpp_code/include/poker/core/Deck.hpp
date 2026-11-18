#pragma once

#include "poker/core/Card.hpp"
#include "poker/core/Random.hpp"

#include <cstddef>
#include <vector>

namespace poker {

/// Колода з 52 карт (або менше, якщо деякі карти виключені).
class Deck {
public:
    Deck();                            ///< Повна впорядкована колода
    explicit Deck(CardMask excluded);  ///< Колода без вказаних карт

    /// Повертає всі карти в колоду (крім excluded) у стандартному порядку.
    void reset(CardMask excluded = 0);

    /// Тасує карти, що залишились.
    void shuffle(Rng& rng);

    /// Роздає верхню карту. @throws std::out_of_range якщо колода порожня
    Card deal();

    std::size_t size() const { return cards_.size() - top_; }
    bool empty() const { return size() == 0; }

private:
    std::vector<Card> cards_;
    std::size_t top_ = 0;
};

}  // namespace poker
