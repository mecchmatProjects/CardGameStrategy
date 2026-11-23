add_library(poker_ui
    Console.cpp
    ConsoleObserver.cpp
    HumanStrategy.cpp
    AnalysisScreens.cpp
    App.cpp
)
target_link_libraries(poker_ui PUBLIC poker_ai)
