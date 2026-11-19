add_library(poker_game
    Types.cpp
)
target_link_libraries(poker_game PUBLIC poker_eval)
