#include "poker/game/Player.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace poker {

Player::Player(int seat, std::string name, int chips, std::unique_ptr<Strategy> strategy)
    : seat_(seat), name_(std::move(name)), chips_(chips), strategy_(std::move(strategy)) {
    if (!strategy_) throw std::invalid_argument("Гравцю потрібна стратегія");
    if (chips_ < 0) throw std::invalid_argument("Стек не може бути від'ємним");
}

void Player::startHand() {
    hole_ = {};
    folded_ = false;
    allIn_ = false;
    sittingOut_ = chips_ <= 0;
    streetBet_ = 0;
    contribution_ = 0;
}

void Player::startStreet() { streetBet_ = 0; }

int Player::commit(int amount) {
    const int actual = std::clamp(amount, 0, chips_);
    chips_ -= actual;
    streetBet_ += actual;
    contribution_ += actual;
    if (chips_ == 0) allIn_ = true;
    return actual;
}

}
