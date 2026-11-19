/**
 * @file command_init.c
 * @brief 初始化工作区命令实现。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include "command_init.h"

#include <getopt.h>
#include <string.h>

#include <algorepo/console/console_ui.h>
#include <algorepo/utils/utils_fs.h>
#include <algorepo/utils/utils_compat.h>
#include <algorepo/utils/utils_str.h>

#include "command_common.h"

/* 显示 init 命令帮助信息 */
void help_init(void) {
    help_view_print_usage("init", "[-g] [-q|--quiet]");
    help_view_print_description("初始化 AlgoRepo 工作区");
    help_view_print_help_section("创建内容:");
    help_view_print_help_option(".algorepo/config.ini", NULL, "工作区配置");
    help_view_print_help_option(".algorepo/repo.db",    NULL, "题库数据库");
    help_view_print_help_option("solutions/",           NULL, "题目源码目录\n");
    help_view_print_help_section("选项:");
    help_view_print_help_option("-h, --help",  NULL, "显示此帮助信息");
    help_view_print_help_option("-q, --quiet", NULL, "减少输出");
}

/* 在当前目录（或全局 ~/.algorepo）初始化工作区 */
AlgoStatus cmd_init(int argc, char *argv[], RuntimeContext *ctx) {
    bool quiet = false;

    static const struct option long_opts[] = {
        {"quiet", no_argument, 0, 'q'},
        {0, 0, 0, 0}};

    optind = 1; opterr = 0;
    int c;
    while ((c = getopt_long(argc, argv, "q", long_opts, NULL)) != -1) {
        switch (c) {
            case 'q': quiet = true; break;
            case '?': return handle_getopt_error(optopt, "q", argv, optind);
            default: return ALGO_ERR_ARG;
        }
    }

    if (optind < argc) {
        console_log_error("无效参数: \"%s\"; init 不接受位置参数", argv[optind]);
        return ALGO_ERR_ARG;
    }

    /* 全局模式下禁止执行 init */
    if (ctx->global_mode) {
        console_log_error("全局模式下无需（也不允许）手动执行 'init'");
        console_log_info("全局配置与数据库已在程序首次运行时自动启用, 'init' 命令仅用于本地工作区初始化");
        return ALGO_ERR_ARG;
    }

    /* 检查当前目录是否为家目录（作为全局模式根目录） */
    {
        char cwd[ALGO_MAX_PATH_LEN];
        if (utils_compat_getcwd(cwd, sizeof(cwd)) &&
            strcmp(cwd, ctx->home_dir) == 0) {
            console_log_error("不允许在用户家目录下执行 'init'");
            console_log_info("全局配置与数据库已在程序首次运行时自动启用，无需手动初始化");
            console_log_info("请在子目录中执行 \"algo init\" 以创建本地工作区");
            return ALGO_ERR_ARG;
        }
    }

    if (utils_file_exists(ALGO_META_DIR)) {
        if (!quiet) {
            console_log_warn("当前目录已经是 AlgoRepo 工作区");
        }
        return ALGO_ERR_FILE;
    }

    AlgoStatus status = runtime_create_workspace(ctx, ".");
    if (status != ALGO_SUCCESS) { print_status_error("初始化工作区", status); return status; }

    if (!quiet) console_log_success("AlgoRepo 工作区初始化完成");
    return ALGO_SUCCESS;
}
