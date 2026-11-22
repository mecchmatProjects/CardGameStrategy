add_executable(holdem main.cpp)
target_link_libraries(holdem PRIVATE poker_ui)

if(TARGET poker_gui)
    add_executable(holdem_gui gui_main.cpp)
    target_link_libraries(holdem_gui PRIVATE poker_gui)
    if(WIN32)
        set_target_properties(holdem_gui PROPERTIES WIN32_EXECUTABLE TRUE)
        if(MSVC)
            target_link_options(holdem_gui PRIVATE /ENTRY:mainCRTStartup)
        endif()
    endif()
    add_custom_command(TARGET holdem_gui POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${PROJECT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:holdem_gui>/assets)
endif()
