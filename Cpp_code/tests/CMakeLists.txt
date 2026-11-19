add_executable(poker_tests
        test_main.cpp
        test_card.cpp
)
target_link_libraries(poker_tests PRIVATE poker_core)

add_test(NAME poker_tests COMMAND poker_tests)
