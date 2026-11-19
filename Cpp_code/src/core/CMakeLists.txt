add_library(poker_core
        Card.cpp
        Deck.cpp
)
target_include_directories(poker_core PUBLIC ${PROJECT_SOURCE_DIR}/include)
