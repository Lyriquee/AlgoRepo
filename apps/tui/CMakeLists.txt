add_library(algo_tui STATIC
    tui_run.c
)

target_link_libraries(algo_tui PUBLIC algo::service algo::console)

target_include_directories(algo_tui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(algo::tui ALIAS algo_tui)
