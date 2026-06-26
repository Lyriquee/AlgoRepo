/**
 * @file command_rm.c
 * @brief 删除题目命令实现。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include "command_rm.h"

#include <getopt.h>
#include <stdio.h>

#include <algorepo/console/console_ui.h>
#include <algorepo/service/problem_service.h>
#include "command.h"
#include "command_common.h"

/* 执行删除题目命令，支持空格分隔批量 ID */
AlgoStatus cmd_rm(int argc, char *argv[], RuntimeContext *ctx) {
    bool force     = false;
    bool keep_file = false;

    static const struct option long_opts[] = {
        {"force",     no_argument, 0, 'f'},
        {"yes",       no_argument, 0, 'y'},
        {"keep-file", no_argument, 0, 'k'},
        {0, 0, 0, 0}};

    optind = 1; opterr = 0;
    int c;
    while ((c = getopt_long(argc, argv, "fyk", long_opts, NULL)) != -1) {
        switch (c) {
            case 'f': case 'y': force = true; break;
            case 'k': keep_file = true; break;
            case '?': return handle_getopt_error(optopt, "fyk", argv, optind);
            default: return ALGO_ERR_ARG;
        }
    }

    if (optind >= argc) { console_log_error("缺少题目 ID"); return ALGO_ERR_REQUIRED; }

    // 收集所有位置参数作为 ID 列表（支持空格分隔批量删除）
    int ids[256], id_count = 0;
    for (int i = optind; i < argc && id_count < 256; i++) {
        int id = 0;
        if (parse_id_arg(argv[i], &id) != ALGO_SUCCESS) {
            console_log_error("无效 ID: %s", argv[i]);
            return ALGO_ERR_ARG;
        }
        ids[id_count++] = id;
    }

    if (id_count > 1 && !force) {
        bool confirmed = false;
        console_confirm("确认批量删除以上题目?", &confirmed);
        if (!confirmed) { console_log_info("已取消删除"); return ALGO_SUCCESS; }
    }

    int deleted = 0;
    for (int i = 0; i < id_count; i++) {
        int id = ids[i];
        Problem *p = pcol_get_by_id(&ctx->pcol, id);
        if (!p) {
            console_log_error("题目不存在: #%d，跳过", id);
            continue;
        }

        // 单条确认（非强制模式且仅一个 ID 时）
        if (!force && id_count == 1) {
            bool confirmed = false;
            char prompt[ALGO_MAX_TITLE_LEN + 64];
            snprintf(prompt, sizeof(prompt), "确认删除 #%d %s?", p->id, p->title);
            AlgoStatus st = console_confirm(prompt, &confirmed);
            if (st != ALGO_SUCCESS) return st;
            if (!confirmed) { console_log_info("已取消删除"); return ALGO_SUCCESS; }
        }

        AlgoStatus st = service_remove_problem(ctx, id, keep_file);
        if (st != ALGO_SUCCESS) {
            console_log_error("删除 #%d 失败", id);
            continue;
        }
        deleted++;
    }

    console_log_success("已删除 %d 道题目", deleted);
    return ALGO_SUCCESS;
}

/* 显示 rm 命令帮助信息 */
void help_rm(void) {
    help_view_print_usage("rm", "[options] <id...>");
    help_view_print_description("从数据库中移除题目记录，支持空格分隔批量删除");
    help_view_print_help_section("选项:");
    help_view_print_help_option("-h, --help",   NULL,       "显示此帮助信息");
    help_view_print_help_option("-f, --force",  NULL,       "跳过确认提示");
    help_view_print_help_option("-y, --yes",    NULL,       "与 -f 效果相同");
    help_view_print_help_option("-k, --keep-file", NULL,   "仅从数据库移除记录，保留源文件\n");
    help_view_print_help_section("示例:");
    help_view_print_help_example("algo rm 1");
    help_view_print_help_example("algo rm 101 102 103 -f");
    help_view_print_help_example("algo rm 1 --keep-file");
}
