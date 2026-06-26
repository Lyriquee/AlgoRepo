add_library(algo_cli STATIC
    cli_run.c
    command.c

    commands/command_common.c

    commands/command_init.c
    commands/command_add.c
    commands/command_rm.c

    commands/command_list.c
    commands/command_show.c
    commands/command_edit.c

    commands/command_tag.c
    commands/command_search.c
    commands/command_stats.c

    commands/command_config.c
    commands/command_help.c

    repl/repl_loop.c
    repl/repl_cmd.c

    views/problem_view.c
    views/wizard_view.c
    views/help_view.c
    views/repl_view.c
)

target_compile_definitions(algo_cli PRIVATE
    ALGO_CLI_VERSION="1.0.0"
)

target_link_libraries(algo_cli PUBLIC algo::service algo::console)

target_include_directories(algo_cli PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/commands
    ${CMAKE_CURRENT_SOURCE_DIR}/repl
    ${CMAKE_CURRENT_SOURCE_DIR}/views
)

add_library(algo::cli ALIAS algo_cli)
