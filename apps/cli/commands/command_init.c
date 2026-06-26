/**
 * @file command_init.c
 * @brief 初始化工作区命令实现。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include "command_init.h"

#include <getopt.h>
#include <stdio.h>
#include <string.h>

#include <algorepo/console/console_ui.h>
#include <algorepo/utils/utils_fs.h>

#include "command.h"
#include "command_common.h"

/* 显示 init 命令帮助信息 */
void help_init(void) {
    help_view_print_usage("init", "[-g] [-f|--force] [-q|--quiet]");
    help_view_print_description("初始化 AlgoRepo 工作区");
    help_view_print_help_option("-g", NULL, "初始化全局配置目录 (~/.algorepo/)\n");
    help_view_print_help_section("创建内容:");
    help_view_print_help_option(".algorepo/config.ini", NULL, "工作区配置");
    help_view_print_help_option(".algorepo/repo.db",    NULL, "题库数据库");
    help_view_print_help_option("solutions/",           NULL, "题目源码目录\n");
    help_view_print_help_section("选项:");
    help_view_print_help_option("-h, --help",  NULL, "显示此帮助信息");
    help_view_print_help_option("-f, --force", NULL, "已存在工作区时覆盖配置");
    help_view_print_help_option("-q, --quiet", NULL, "减少输出");
}

/* 在当前目录（或全局 ~/.algorepo）初始化工作区 */
AlgoStatus cmd_init(int argc, char *argv[], RuntimeContext *ctx) {
    bool force = false, quiet = false;

    static const struct option long_opts[] = {
        {"force", no_argument, 0, 'f'},
        {"quiet", no_argument, 0, 'q'},
        {0, 0, 0, 0}};

    optind = 1; opterr = 0;
    int c;
    while ((c = getopt_long(argc, argv, "fq", long_opts, NULL)) != -1) {
        switch (c) {
            case 'f': force = true; break;
            case 'q': quiet = true; break;
            case '?': return handle_getopt_error(optopt, "fq", argv, optind);
            default: return ALGO_ERR_ARG;
        }
    }

    if (optind < argc) {
        console_log_error("无效参数: \"%s\"; init 不接受位置参数", argv[optind]);
        return ALGO_ERR_ARG;
    }

    if (ctx->global_mode) {
        char global_meta[ALGO_MAX_PATH_LEN];
        int w = snprintf(global_meta, sizeof(global_meta), "%s/%s", ctx->home_dir, ALGO_META_DIR);
        if (w < 0 || (size_t)w >= sizeof(global_meta)) return ALGO_ERR_RANGE;
        if (utils_file_exists(global_meta) && !force) {
            if (!quiet) {
                console_log_warn("全局配置目录已存在: %s", global_meta);
                console_log_info("使用 \"algo -g init -f\" 可重新初始化");
            }
            return ALGO_ERR_FILE;
        }
    } else {
        if (utils_file_exists(ALGO_META_DIR) && !force) {
            if (!quiet) {
                console_log_warn("当前目录已经是 AlgoRepo 工作区");
                console_log_info("使用 \"algo init -f\" 可重新初始化");
            }
            return ALGO_ERR_FILE;
        }
    }

    AlgoStatus status = runtime_create_workspace(ctx, ".");
    if (status != ALGO_SUCCESS) { print_status_error("初始化工作区", status); return status; }

    if (!quiet) console_log_success("AlgoRepo 工作区初始化完成");
    return ALGO_SUCCESS;
}
