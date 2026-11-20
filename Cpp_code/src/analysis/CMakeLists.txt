add_library(poker_analysis
    EquityCalculator.cpp
    Odds.cpp
    PreflopChart.cpp
    HandRanking.cpp
    Recommendation.cpp
    BasicAdvisor.cpp
    Advisor.cpp
)
target_link_libraries(poker_analysis PUBLIC poker_game)
