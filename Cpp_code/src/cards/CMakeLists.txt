add_library(poker_cards
        PrefCard.cpp
)
target_link_libraries(poker_cards PUBLIC poker_core)
