add_library(poker_ui
        Console.cpp
)
target_link_libraries(poker_ui PUBLIC poker_ai)
