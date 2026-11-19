#include "poker/core/Deck.hpp"

#include <algorithm>
#include <iterator>
#include <stdexcept>

namespace poker {

    Deck::Deck() { reset(); }

    Deck::Deck(CardMask excluded) { reset(excluded); }

    void Deck::reset(CardMask excluded) {
        cards_.clear();
        top_ = 0;
        for (int i = 0; i < kDeckSize; ++i) {
            const Card card = Card::fromIndex(i);
            if (!(excluded & card.mask())) cards_.push_back(card);
        }
    }

    void Deck::shuffle(Rng& rng) {
        std::shuffle(std::next(cards_.begin(), static_cast<std::ptrdiff_t>(top_)), cards_.end(),
                     rng.engine());
    }

    Card Deck::deal() {
        if (empty()) throw std::out_of_range("Колода порожня");
        return cards_[top_++];
    }

}
