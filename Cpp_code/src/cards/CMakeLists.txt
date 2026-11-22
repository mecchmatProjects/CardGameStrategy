add_library(poker_cards
    PrefCard.cpp
    Decks.cpp
    CardGame.cpp
    PokerTasks.cpp
    Preferans.cpp
)
target_link_libraries(poker_cards PUBLIC poker_core)
