add_library(poker_eval
    HandEvaluator.cpp
)
target_link_libraries(poker_eval PUBLIC poker_core)
