add_executable(poker_tests
    test_main.cpp
    test_card.cpp
    test_evaluator.cpp
    test_pots.cpp
    test_game.cpp
    test_analysis.cpp
    test_bots.cpp
)
target_link_libraries(poker_tests PRIVATE poker_ai)

add_test(NAME poker_tests COMMAND poker_tests)

add_test(NAME cards_lab_smoke
         COMMAND cards_lab --all --poker-n 20000 --pref-n 5000 --split-n 5000)
