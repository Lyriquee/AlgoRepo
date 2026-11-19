/**
 * @file command_list.c
 * @brief 列表浏览命令实现。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include "command_list.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorepo/console/console_ui.h>

#include "command_common.h"
#include "views/problem_view.h"


/* --- 私有函数声明 --------------------------------------------------------- */

static int cmp_id(const void *a, const void *b);
static int cmp_diff(const void *a, const void *b);
static int cmp_mastery(const void *a, const void *b);
static int cmp_date(const void *a, const void *b);


/* --- 公开接口实现 --------------------------------------------------------- */

void help_list(void) {
    help_view_print_usage("list", "[options] [id]");
    help_view_print_description("按分类浏览题库中的题目");
    help_view_print_help_section("选项:");
    help_view_print_help_option("-h, --help",        NULL,      "显示此帮助信息");
    help_view_print_help_option("-g, --tag",         "<tag>",   "按标签过滤");
    help_view_print_help_option("-d, --diff",        "<diff>",  "按难度过滤");
    help_view_print_help_option("-m, --mastery",     "<num>",   "按掌握度过滤 (0-4)");
    help_view_print_help_option("-s, --sort",        "<field>", "排序: id, diff, mastery, time");
    help_view_print_help_option("-n, --limit",       "<num>",   "限制输出数量");
    help_view_print_help_option("-t, --time",        NULL,      "日期列显示时:分");
    help_view_print_help_option("    --today",       NULL,      "今天操作过的题目");
    help_view_print_help_option("    --recent",      "<days>",  "最近 N 天内操作过的\n");
    help_view_print_help_section("示例:");
    help_view_print_help_example("algo ls 1");
    help_view_print_help_example("algo ls -d 容易");
    help_view_print_help_example("algo ls --today");
}

/* 按分类浏览题库（支持标签/难度/掌握度/时间过滤与排序） */
AlgoStatus cmd_list(int argc, char *argv[], RuntimeContext *ctx) {
    int diff = -1, mastery = -1, limit = -1, recent = -1;
    const char *tag = NULL, *sort_str = NULL;
    bool today_only = false, show_time = false;

    static const struct option long_opts[] = {
        {"tag",     required_argument, 0, 'g'},
        {"diff",    required_argument, 0, 'd'},
        {"mastery", required_argument, 0, 'm'},
        {"sort",    required_argument, 0, 's'},
        {"limit",   required_argument, 0, 'n'},
        {"time",    no_argument,       0, 't'},
        {"today",   no_argument,       0, 'T'},
        {"recent",  required_argument, 0, 'R'},
        {0, 0, 0, 0}};

    optind = 1; opterr = 0;
    int c;
    while ((c = getopt_long(argc, argv, "g:d:m:s:n:t", long_opts, NULL)) != -1) {
        switch (c) {
            case 'g': tag = optarg; break;
            case 'd': if (parse_difficulty(optarg, &diff) != ALGO_SUCCESS) { console_log_error("无效难度"); return ALGO_ERR_ARG; } break;
            case 'm': if (parse_mastery(optarg, &mastery) != ALGO_SUCCESS) { console_log_error("无效掌握度: %s，请使用 0-4 或中文名称", optarg); return ALGO_ERR_ARG; } break;
            case 's': sort_str = optarg; break;
            case 'n': if (parse_int_arg(optarg, 1, 10000, &limit) != ALGO_SUCCESS) { console_log_error("无效 limit"); return ALGO_ERR_ARG; } break;
            case 't': show_time = true; break;
            case 'T': today_only = true; break;
            case 'R': if (parse_int_arg(optarg, 1, 365, &recent) != ALGO_SUCCESS) { console_log_error("无效 days"); return ALGO_ERR_ARG; } break;
            case '?': return handle_getopt_error(optopt, "g:d:m:s:n:t", argv, optind);
            default: return ALGO_ERR_ARG;
        }
    }

    // 单 ID 快速查看
    if (optind < argc && (diff < 0 && mastery < 0 && !tag && !today_only && recent < 0)) {
        int id;
        if (parse_id_arg(argv[optind], &id) == ALGO_SUCCESS) {
            Problem *p = pcol_get_by_id(&ctx->pcol, id);
            if (!p) { console_log_error("题目不存在: #%d", id); return ALGO_ERR_NOT_FOUND; }
            problem_view_print_header(show_time);
            problem_view_print_row(p, show_time);
            return ALGO_SUCCESS;
        }
    }

    ProblemCollection result;
    AlgoStatus st = pcol_init(&result);
    if (st != ALGO_SUCCESS) return st;

    for (size_t i = 0; i < ctx->pcol.size; i++) {
        const Problem *p = &ctx->pcol.items[i];
        if (diff >= 0 && p->diff != (Difficulty)diff) continue;
        if (mastery >= 0 && p->mastery != (Mastery)mastery) continue;
        if (tag && !problem_has_tag(p, tag)) continue;
        if (today_only && !match_today(p->created_at)) continue;
        if (recent > 0 && !match_recent(p->created_at, recent)) continue;
        st = pcol_add(&result, p);
        if (st != ALGO_SUCCESS) { pcol_free(&result); return st; }
        result.items[result.size - 1].id = p->id;
    }

    if (sort_str) {
        if (strcmp(sort_str, "diff") == 0) qsort(result.items, result.size, sizeof(Problem), cmp_diff);
        else if (strcmp(sort_str, "mastery") == 0) qsort(result.items, result.size, sizeof(Problem), cmp_mastery);
        else if (strcmp(sort_str, "time") == 0 || strcmp(sort_str, "date") == 0) qsort(result.items, result.size, sizeof(Problem), cmp_date);
        else qsort(result.items, result.size, sizeof(Problem), cmp_id);
    }
    if (limit > 0 && (size_t)limit < result.size) result.size = (size_t)limit;
    problem_view_print_list(&result, "没有匹配的题目", show_time);
    pcol_free(&result);
    return ALGO_SUCCESS;
}


/* --- 私有函数实现 --------------------------------------------------------- */

static int cmp_id(const void *a, const void *b) { return ((Problem *)a)->id - ((Problem *)b)->id; }
static int cmp_diff(const void *a, const void *b) { return ((Problem *)a)->diff - ((Problem *)b)->diff; }
static int cmp_mastery(const void *a, const void *b) { return ((Problem *)a)->mastery - ((Problem *)b)->mastery; }
static int cmp_date(const void *a, const void *b) { return ((Problem *)b)->created_at > ((Problem *)a)->created_at ? 1 : -1; }
