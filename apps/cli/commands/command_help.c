/**
 * @file command_help.c
 * @brief 帮助与版本命令实现。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include "command_help.h"

#include <stdio.h>

#include <algorepo/storage/storage_file.h>
#include <algorepo/console/console_ui.h>

#include "command.h"
#include "views/help_view.h"

/* 显示 help 命令自身的帮助信息 */
void help_help(void) {
    help_view_print_usage("help", "[command]");
    help_view_print_description("显示全局帮助或指定命令的详细帮助");
    help_view_print_help_section("示例:");
    help_view_print_help_example("algo help");
    help_view_print_help_example("algo help add");
}

/* 执行帮助命令：无参数显示主帮助，有参数显示对应子命令帮助 */
AlgoStatus cmd_help(int argc, char *argv[], RuntimeContext *ctx) {
    (void)ctx;
    if (argc == 1) {
        help_view_print_main();
        return ALGO_SUCCESS;
    }
    if (argc == 2) {
        help_view_print_sub_cmd(argv[1]);
        return ALGO_SUCCESS;
    }
    console_log_error("用法: algo help [command]");
    return ALGO_ERR_ARG;
}

/* 显示 version 命令的帮助信息 */
void help_version(void) {
    help_view_print_usage("version", "");
    help_view_print_description("显示 AlgoRepo CLI 版本和构建信息");
}

/* 执行版本命令：显示版本信息 */
AlgoStatus cmd_version(int argc, char *argv[], RuntimeContext *ctx) {
    (void)ctx;
    (void)argc;
    (void)argv;

    help_view_print_version();
    return ALGO_SUCCESS;
}
