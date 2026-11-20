add_library(poker_analysis
    EquityCalculator.cpp
    HandRanking.cpp
    Recommendation.cpp
)
target_link_libraries(poker_analysis PUBLIC poker_game)
