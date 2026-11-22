find_package(Threads REQUIRED)

add_library(poker_gui
    Render.cpp
    GameBridge.cpp
)
target_link_libraries(poker_gui PUBLIC poker_ai raylib Threads::Threads)
target_compile_definitions(poker_gui PRIVATE HOLDEM_ASSETS_DIR="${PROJECT_SOURCE_DIR}/assets")
