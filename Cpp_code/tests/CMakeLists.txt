add_executable(poker_tests
    test_main.cpp
    test_card.cpp
    test_evaluator.cpp
)
target_link_libraries(poker_tests PRIVATE poker_game)

add_test(NAME poker_tests COMMAND poker_tests)
