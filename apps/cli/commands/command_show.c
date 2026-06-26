/**
 * @file command_show.c
 * @brief 题目详情查看命令实现。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include "command_show.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorepo/console/console_ui.h>

#include "command.h"
#include "command_common.h"
#include "views/problem_view.h"

/* 显示 show 命令帮助信息 */
void help_show(void) {
    help_view_print_usage("show", "[options] <id>");
    help_view_print_description("查看指定题目的详细信息");
    help_view_print_help_section("选项:");
    help_view_print_help_option("-h, --help", NULL,  "显示此帮助信息");
    help_view_print_help_option("-a, --all",  NULL,  "显示所有信息");
    help_view_print_help_option("-c, --code", NULL,  "显示源文件代码");
    help_view_print_help_option("-H, --head", "[n]", "仅显示代码的前 n 行(默认:10)");
    help_view_print_help_option("-T, --tail", "[n]", "仅显示代码的后 n 行(默认:10)\n");
    help_view_print_help_section("示例:");
    help_view_print_help_example("algo show 1");
    help_view_print_help_example("algo show 1 2 3");
    help_view_print_help_example("algo show 1 --all -H 5");
}

/* 查看题目详情，支持空格分隔多个 ID 连续显示 */
AlgoStatus cmd_show(int argc, char *argv[], RuntimeContext *ctx) {
    bool show_all  = false;
    bool show_code = false;
    int  head_n    = 0;
    int  tail_n    = 0;

    static const struct option long_opts[] = {
        {"all",  no_argument,       0, 'a'},
        {"code", no_argument,       0, 'c'},
        {"head", optional_argument, 0, 'H'},
        {"tail", optional_argument, 0, 'T'},
        {0, 0, 0, 0}};

    optind = 1; opterr = 0;
    int c;
    while ((c = getopt_long(argc, argv, "acH:T:", long_opts, NULL)) != -1) {
        switch (c) {
            case 'a': show_all = true; break;
            case 'c': show_code = true; break;
            case 'H': head_n = 10; if (optarg) { int v = 0; if (parse_int_arg(optarg, 1, 10000, &v) == ALGO_SUCCESS) head_n = v; } break;
            case 'T': tail_n = 10; if (optarg) { int v = 0; if (parse_int_arg(optarg, 1, 10000, &v) == ALGO_SUCCESS) tail_n = v; } break;
            case '?': return handle_getopt_error(optopt, "acH:T:", argv, optind);
            default: return ALGO_ERR_ARG;
        }
    }

    if (optind >= argc) { console_log_error("缺少题目 ID"); return ALGO_ERR_REQUIRED; }

    // 收集所有位置参数作为 ID 列表（支持空格分隔批量查看）
    int ids[256], id_count = 0;
    for (int i = optind; i < argc && id_count < 256; i++) {
        int id = 0;
        if (parse_id_arg(argv[i], &id) != ALGO_SUCCESS) {
            console_log_error("无效 ID: %s", argv[i]);
            return ALGO_ERR_ARG;
        }
        ids[id_count++] = id;
    }

    int effective_head = (head_n == 0 && tail_n == 0) ? 1000000 : head_n;

    for (int i = 0; i < id_count; i++) {
        // 多条之间加分隔
        if (i > 0) console_print_newline();

        Problem *p = pcol_get_by_id(&ctx->pcol, ids[i]);
        if (!p) {
            console_log_error("题目不存在: #%d", ids[i]);
            continue;
        }

        if (show_all) {
            problem_view_print_detail_all(p);
            problem_view_print_code(p, effective_head, tail_n);
        } else if (show_code) {
            problem_view_print_summary(p);
            problem_view_print_code(p, effective_head, tail_n);
        } else {
            problem_view_print_detail(p);
        }
    }
    return ALGO_SUCCESS;
}
