add_library(poker_ui
    Console.cpp
    ConsoleObserver.cpp
    AnalysisScreens.cpp
)
target_link_libraries(poker_ui PUBLIC poker_ai)
