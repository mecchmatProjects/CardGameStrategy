#include "poker/game/Game.hpp"

#include "poker/eval/HandEvaluator.hpp"
#include "poker/game/PotManager.hpp"

#include <algorithm>
#include <initializer_list>
#include <stdexcept>
#include <utility>

namespace poker {

Game::Game(GameConfig config, std::uint64_t seed) : config_(config), rng_(seed) {
    if (config_.smallBlind <= 0 || config_.bigBlind < config_.smallBlind) {
        throw std::invalid_argument("Некоректні блайнди");
    }
    if (config_.startingChips <= 0) {
        throw std::invalid_argument("Стартовий стек має бути додатним");
    }
}

int Game::addPlayer(std::string name, std::unique_ptr<Strategy> strategy) {
    if (players_.size() >= 10) throw std::length_error("За столом максимум 10 гравців");
    const int seat = static_cast<int>(players_.size());
    players_.emplace_back(seat, std::move(name), config_.startingChips, std::move(strategy));
    stats_.emplace_back();
    vpipCounted_.push_back(false);
    pfrCounted_.push_back(false);
    return seat;
}

void Game::setObserver(GameObserver* observer) { observer_ = observer; }

void Game::rebuy(int seat, int amount) {
    if (amount <= 0) throw std::invalid_argument("Сума ребаю має бути додатною");
    players_.at(static_cast<std::size_t>(seat)).addChips(amount);
}

int Game::playersWithChips() const {
    return static_cast<int>(std::count_if(players_.begin(), players_.end(),
                                          [](const Player& p) { return p.chips() > 0; }));
}

int Game::totalChips() const {
    int total = 0;
    for (const auto& p : players_) total += p.chips();
    return total;
}

int Game::pot() const {
    int total = 0;
    for (const auto& p : players_) total += p.handContribution();
    return total;
}

int Game::countInHand() const {
    return static_cast<int>(std::count_if(players_.begin(), players_.end(),
                                          [](const Player& p) { return p.inHand(); }));
}

int Game::countCanAct() const {
    return static_cast<int>(std::count_if(players_.begin(), players_.end(),
                                          [](const Player& p) { return p.canAct(); }));
}

int Game::nextSeat(int from, bool requireInHand) const {
    const int n = static_cast<int>(players_.size());
    for (int step = 1; step <= n; ++step) {
        const int seat = ((from + step) % n + n) % n;
        const Player& p = players_[static_cast<std::size_t>(seat)];
        if (requireInHand ? p.inHand() : p.chips() > 0) return seat;
    }
    return -1;
}

void Game::prepareSeatOrder() {
    order_.clear();
    const int n = static_cast<int>(players_.size());
    for (int step = 0; step < n; ++step) {
        const int seat = (dealer_ + step) % n;
        if (!players_[static_cast<std::size_t>(seat)].isSittingOut()) order_.push_back(seat);
    }
}

Position Game::positionOf(int seat) const {
    const auto it = std::find(order_.begin(), order_.end(), seat);
    const int k = static_cast<int>(std::distance(order_.begin(), it));
    return positionFromButton(k, static_cast<int>(order_.size()));
}

bool Game::playHand() {
    if (playersWithChips() < 2) return false;

    ++handNumber_;
    board_.clear();
    dealer_ = nextSeat(dealer_, false);
    for (auto& p : players_) p.startHand();
    prepareSeatOrder();
    deck_.reset();
    deck_.shuffle(rng_);
    raisesThisStreet_ = 0;
    preflopRaises_ = 0;
    preflopAggressor_ = -1;
    history_.clear();
    for (const int seat : order_) {
        ++stats_[static_cast<std::size_t>(seat)].hands;
        vpipCounted_[static_cast<std::size_t>(seat)] = false;
        pfrCounted_[static_cast<std::size_t>(seat)] = false;
    }

    observer().onHandStart(handNumber_, players_, dealer_);

    postBlinds();
    dealHoleCards();

    for (const Street street : {Street::Preflop, Street::Flop, Street::Turn, Street::River}) {
        if (street != Street::Preflop) {
            for (auto& p : players_) p.startStreet();
            highestBet_ = 0;
            minRaise_ = config_.bigBlind;
            raisesThisStreet_ = 0;
            dealBoard(street == Street::Flop ? 3 : 1);
            observer().onStreet(street, board_, pot());
        }
        if (countInHand() > 1) bettingRound(street);
        if (countInHand() == 1) {
            awardUncontested();
            observer().onHandEnd(players_);
            return true;
        }
    }

    showdown();
    observer().onHandEnd(players_);
    return true;
}

void Game::postBlinds() {
    const bool headsUp = order_.size() == 2;
    const int smallBlindSeat = headsUp ? order_[0] : order_[1];
    bigBlindSeat_ = headsUp ? order_[1] : order_[2];

    postBlind(smallBlindSeat, config_.smallBlind);
    postBlind(bigBlindSeat_, config_.bigBlind);

    highestBet_ = config_.bigBlind;
    minRaise_ = config_.bigBlind;
}

void Game::postBlind(int seat, int amount) {
    Player& player = players_[static_cast<std::size_t>(seat)];
    const int posted = player.commit(amount);
    observer().onAction(player, Action{ActionType::PostBlind, posted}, pot());
}

void Game::dealHoleCards() {
    std::vector<HoleCards> dealt(players_.size());
    const std::size_t n = order_.size();
    for (std::size_t round = 0; round < 2; ++round) {
        for (std::size_t i = 1; i <= n; ++i) {
            dealt[static_cast<std::size_t>(order_[i % n])][round] = deck_.deal();
        }
    }
    for (const int seat : order_) {
        Player& player = players_[static_cast<std::size_t>(seat)];
        HoleCards hole = dealt[static_cast<std::size_t>(seat)];
        if (hole[1].rankValue() > hole[0].rankValue() ||
            (hole[1].rankValue() == hole[0].rankValue() && hole[1].suit() > hole[0].suit())) {
            std::swap(hole[0], hole[1]);
        }
        player.setHole(hole);
        observer().onHoleCards(player);
    }
}

void Game::dealBoard(int count) {
    deck_.deal();
    for (int i = 0; i < count; ++i) board_.push_back(deck_.deal());
}

void Game::bettingRound(Street street) {
    const int n = static_cast<int>(players_.size());

    const int actors = countCanAct();
    if (actors == 0) return;
    if (actors == 1) {
        for (const auto& p : players_) {
            if (p.canAct() && p.streetBet() >= highestBet_) return;
        }
    }

    std::vector<bool> needsToAct(players_.size());
    for (int i = 0; i < n; ++i) needsToAct[static_cast<std::size_t>(i)] = players_[static_cast<std::size_t>(i)].canAct();

    int seat = nextSeat(street == Street::Preflop ? bigBlindSeat_ : dealer_, true);

    while (countInHand() > 1 &&
           std::find(needsToAct.begin(), needsToAct.end(), true) != needsToAct.end()) {
        const auto idx = static_cast<std::size_t>(seat);
        Player& player = players_[idx];
        if (needsToAct[idx] && player.canAct()) {
            const int previousHighest = highestBet_;
            const int potBefore = pot();
            const int toCallBefore = highestBet_ - player.streetBet();
            const Action requested = player.strategy().decide(makeContext(seat, street));
            const Action resolved = execute(seat, requested);
            recordAction(seat, street, resolved, highestBet_ > previousHighest, potBefore, toCallBefore);
            observer().onAction(player, resolved, pot());

            if (highestBet_ > previousHighest) {
                const int increment = highestBet_ - previousHighest;
                if (increment >= minRaise_) minRaise_ = increment;
                for (int i = 0; i < n; ++i) {
                    const auto j = static_cast<std::size_t>(i);
                    if (i != seat && players_[j].canAct()) needsToAct[j] = true;
                }
            }
        }
        needsToAct[idx] = false;
        seat = (seat + 1) % n;
    }
}

Action Game::execute(int seat, const Action& requested) {
    Player& player = players_[static_cast<std::size_t>(seat)];
    const int toCall = highestBet_ - player.streetBet();
    const int previousHighest = highestBet_;

    auto goAllIn = [&]() {
        player.commit(player.chips());
        highestBet_ = std::max(highestBet_, player.streetBet());
        return Action{ActionType::AllIn, player.streetBet()};
    };

    switch (requested.type) {
        case ActionType::Fold:
        case ActionType::PostBlind:
            if (toCall <= 0) return Action::check();
            player.fold();
            return Action::fold();

        case ActionType::Check:
            if (toCall <= 0) return Action::check();
            player.fold();
            return Action::fold();

        case ActionType::Call: {
            if (toCall <= 0) return Action::check();
            const int added = player.commit(toCall);
            if (player.isAllIn()) return Action{ActionType::AllIn, player.streetBet()};
            return Action{ActionType::Call, added};
        }

        case ActionType::Bet:
        case ActionType::Raise: {
            const int target = std::max(requested.amount, highestBet_ + minRaise_);
            if (target - player.streetBet() >= player.chips()) return goAllIn();
            player.commit(target - player.streetBet());
            highestBet_ = target;
            return Action{previousHighest == 0 ? ActionType::Bet : ActionType::Raise, target};
        }

        case ActionType::AllIn:
            return goAllIn();
    }
    return Action::fold();
}

DecisionContext Game::makeContext(int seat, Street street) const {
    const Player& player = players_[static_cast<std::size_t>(seat)];
    DecisionContext ctx;
    ctx.hole = player.hole();
    ctx.board = board_;
    ctx.street = street;
    ctx.position = positionOf(seat);
    ctx.pot = pot();
    ctx.currentBet = player.streetBet();
    ctx.highestBet = highestBet_;
    ctx.toCall = std::min(std::max(0, highestBet_ - player.streetBet()), player.chips());
    ctx.minRaiseTo = highestBet_ + minRaise_;
    ctx.stack = player.chips();
    ctx.bigBlind = config_.bigBlind;
    ctx.activeOpponents = countInHand() - 1;
    ctx.playersInHand = static_cast<int>(order_.size());
    ctx.raisesThisStreet = raisesThisStreet_;
    ctx.preflopRaises = preflopRaises_;
    ctx.preflopAggressor = preflopAggressor_ == seat;

    int maxOpponent = 0;
    double vpipSum = 0.0, aggressionSum = 0.0;
    int opponents = 0;
    for (const auto& o : players_) {
        if (o.seat() == seat || !o.inHand()) continue;
        maxOpponent = std::max(maxOpponent, o.chips() + o.streetBet());
        vpipSum += smoothedVpip(o.seat());
        aggressionSum += smoothedAggression(o.seat());
        ++opponents;
    }
    ctx.effectiveStack = std::max(0, std::min(player.chips(), maxOpponent - player.streetBet()));
    ctx.seat = seat;
    ctx.history = history_;
    for (const auto& o : players_) {
        if (o.seat() != seat && o.inHand()) ctx.opponents.push_back(opponentInfo(o));
    }
    if (opponents > 0) {
        ctx.opponentsVpip = vpipSum / opponents;
        ctx.opponentsAggression = aggressionSum / opponents;
    }
    return ctx;
}

void Game::recordAction(int seat, Street street, const Action& resolved, bool raised, int potBefore,
                        int toCall) {
    const auto idx = static_cast<std::size_t>(seat);
    PlayerStats& st = stats_[idx];
    history_.push_back({seat, street, resolved.type, resolved.amount, raised, potBefore, toCall});
    if (street != Street::Preflop && toCall > 0) {
        ++st.facedBet;
        if (resolved.type == ActionType::Fold) ++st.foldedToBet;
    }
    if (street == Street::Preflop && raised && !pfrCounted_[idx]) {
        pfrCounted_[idx] = true;
        ++st.pfr;
    }
    if (raised) {
        ++raisesThisStreet_;
        ++st.aggressive;
        if (street == Street::Preflop) {
            ++preflopRaises_;
            preflopAggressor_ = seat;
        }
    } else if (resolved.type == ActionType::Call || resolved.type == ActionType::AllIn) {
        ++st.passive;
    }
    const bool voluntary = raised || resolved.type == ActionType::Call || resolved.type == ActionType::AllIn;
    if (street == Street::Preflop && voluntary && !vpipCounted_[idx]) {
        vpipCounted_[idx] = true;
        ++st.vpip;
    }
}

double Game::smoothedVpip(int seat) const {
    const PlayerStats& st = stats_[static_cast<std::size_t>(seat)];
    return (st.vpip + 0.28 * 10.0) / (st.hands + 10.0);
}

OpponentInfo Game::opponentInfo(const Player& p) const {
    const PlayerStats& st = stats_[static_cast<std::size_t>(p.seat())];
    OpponentInfo info;
    info.seat = p.seat();
    info.chips = p.chips();
    info.streetBet = p.streetBet();
    info.hands = st.hands;
    info.vpip = smoothedVpip(p.seat());
    info.pfr = (st.pfr + 0.18 * 10.0) / (st.hands + 10.0);
    info.aggression = smoothedAggression(p.seat());
    info.foldToBet = (st.foldedToBet + 0.45 * 6.0) / (st.facedBet + 6.0);
    return info;
}

double Game::smoothedAggression(int seat) const {
    const PlayerStats& st = stats_[static_cast<std::size_t>(seat)];
    return (st.aggressive + 0.4 * 6.0) / (st.aggressive + st.passive + 6.0);
}

void Game::awardUncontested() {
    for (auto& player : players_) {
        if (!player.inHand()) continue;
        const int amount = pot();
        player.award(amount);
        PotResult result;
        result.amount = amount;
        result.winners.push_back({player.seat(), amount});
        result.description = "усі суперники скинули карти";
        observer().onPotAwarded(result, players_);
        return;
    }
}

void Game::showdown() {
    const int n = static_cast<int>(players_.size());
    std::vector<HandValue> values(players_.size());
    std::vector<ShowdownEntry> entries;

    for (std::size_t i = 1; i <= order_.size(); ++i) {
        const int seat = order_[i % order_.size()];
        const Player& player = players_[static_cast<std::size_t>(seat)];
        if (!player.inHand()) continue;
        values[static_cast<std::size_t>(seat)] = evaluateHand(player.hole(), board_);
        entries.push_back({seat, player.hole(), values[static_cast<std::size_t>(seat)]});
    }
    observer().onShowdown(entries, players_);

    std::vector<int> contributions(players_.size());
    std::vector<bool> folded(players_.size());
    for (std::size_t i = 0; i < players_.size(); ++i) {
        contributions[i] = players_[i].handContribution();
        folded[i] = !players_[i].inHand();
    }

    const auto pots = buildPots(contributions, folded);
    auto distanceFromButton = [&](int seat) { return (seat - dealer_ - 1 + n) % n; };

    for (std::size_t k = 0; k < pots.size(); ++k) {
        const Pot& pot = pots[k];
        if (pot.eligible.empty()) continue;

        std::uint32_t best = 0;
        for (const int s : pot.eligible) best = std::max(best, values[static_cast<std::size_t>(s)].score);

        std::vector<int> winners;
        for (const int s : pot.eligible) {
            if (values[static_cast<std::size_t>(s)].score == best) winners.push_back(s);
        }
        std::sort(winners.begin(), winners.end(),
                  [&](int a, int b) { return distanceFromButton(a) < distanceFromButton(b); });

        const int count = static_cast<int>(winners.size());
        const int share = pot.amount / count;
        int remainder = pot.amount % count;

        PotResult result;
        result.index = static_cast<int>(k);
        result.amount = pot.amount;
        result.description = values[static_cast<std::size_t>(winners.front())].describe();
        for (const int s : winners) {
            const int won = share + (remainder > 0 ? 1 : 0);
            if (remainder > 0) --remainder;
            players_[static_cast<std::size_t>(s)].award(won);
            result.winners.push_back({s, won});
        }
        observer().onPotAwarded(result, players_);
    }
}

}
