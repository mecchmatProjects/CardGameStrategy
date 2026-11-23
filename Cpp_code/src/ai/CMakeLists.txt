add_library(poker_ai
    Bots.cpp
)
target_link_libraries(poker_ai PUBLIC poker_analysis)
