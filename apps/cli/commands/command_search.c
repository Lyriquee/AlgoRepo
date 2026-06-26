/**
 * @file command_search.c
 * @brief 搜索题目命令实现。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include "command_search.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorepo/console/console_ui.h>
#include <algorepo/service/problem_query.h>
#include <algorepo/utils/utils_compat.h>

#include "command.h"
#include "command_common.h"
#include "views/problem_view.h"


/* --- 公开接口实现 --------------------------------------------------------- */

void help_search(void) {
    help_view_print_usage("search", "[options] <关键词>");
    help_view_print_description("在题库中精准检索");
    help_view_print_help_section("选项:");
    help_view_print_help_option("-h, --help",         NULL,      "显示此帮助信息");
    help_view_print_help_option("-c, --content",      NULL,      "深度内容搜索");
    help_view_print_help_option("-I, --case-sensitive", NULL,    "强制区分大小写");
    help_view_print_help_option("-t, --title",        "<text>",  "仅在标题中搜索");
    help_view_print_help_option("-g, --tag",          "<tag>",   "限定指定标签");
    help_view_print_help_option("-d, --diff",         "<diff>",  "限定指定难度");
    help_view_print_help_option("-m, --mastery",      "<num>",   "限定指定掌握度");
    help_view_print_help_option("    --today",        NULL,      "今天操作过的");
    help_view_print_help_option("    --recent",       "<days>",  "最近 N 天内");
    help_view_print_help_option("-n, --limit",        "<num>",   "限制数量 (默认 50)\n");
    help_view_print_help_section("示例:");
    help_view_print_help_example("algo s \"hash\"");
    help_view_print_help_example("algo s \"dfs\" -c");
}

/* 在题库中按关键词/标签/难度/掌握度检索，支持内容深度搜索 */
AlgoStatus cmd_search(int argc, char *argv[], RuntimeContext *ctx) {
    bool content = false, case_sensitive = false, title_only = false, today_only = false;
    const char *search_tag = NULL, *title_text = NULL;
    int diff = -1, mastery = -1, limit = 50, recent = -1;

    static const struct option long_opts[] = {
        {"content",       no_argument,       0, 'c'},
        {"case-sensitive",no_argument,       0, 'I'},
        {"title",         required_argument, 0, 't'},
        {"tag",           required_argument, 0, 'g'},
        {"diff",          required_argument, 0, 'd'},
        {"mastery",       required_argument, 0, 'm'},
        {"today",         no_argument,       0, 'T'},
        {"recent",        required_argument, 0, 'R'},
        {"limit",         required_argument, 0, 'n'},
        {0, 0, 0, 0}};

    optind = 1; opterr = 0;
    int c;
    while ((c = getopt_long(argc, argv, "cIt:g:d:m:n:", long_opts, NULL)) != -1) {
        switch (c) {
            case 'c': content = true; break;
            case 'I': case_sensitive = true; break;
            case 't': title_only = true; title_text = optarg; break;
            case 'g': search_tag = optarg; break;
            case 'd': if (parse_difficulty(optarg, &diff) != ALGO_SUCCESS) { console_log_error("无效难度"); return ALGO_ERR_ARG; } break;
            case 'm': if (parse_int_arg(optarg, 0, 4, &mastery) != ALGO_SUCCESS) { console_log_error("无效掌握度"); return ALGO_ERR_ARG; } break;
            case 'T': today_only = true; break;
            case 'R': if (parse_int_arg(optarg, 1, 365, &recent) != ALGO_SUCCESS) { console_log_error("无效 days"); return ALGO_ERR_ARG; } break;
            case 'n': if (parse_int_arg(optarg, 1, 10000, &limit) != ALGO_SUCCESS) { console_log_error("无效 limit"); return ALGO_ERR_ARG; } break;
            case '?': return handle_getopt_error(optopt, "cIt:g:d:m:n:", argv, optind);
            default: return ALGO_ERR_ARG;
        }
    }

    const char *keyword = (optind < argc) ? argv[optind] : NULL;

    // 未提供关键词则交互式输入
    char keyword_buf[256] = {0};
    if (!keyword) {
        console_print_prompt("请输入搜索关键词（输入 q 取消）: ");
        fflush(stdout);
        if (fgets(keyword_buf, sizeof(keyword_buf), stdin)) {
            size_t kw_len = strlen(keyword_buf);
            while (kw_len > 0 && (keyword_buf[kw_len - 1] == '\n' || keyword_buf[kw_len - 1] == '\r'))
                keyword_buf[--kw_len] = '\0';
            if (kw_len > 0 && strcmp(keyword_buf, "q") != 0)
                keyword = keyword_buf;
        }
    }
    if (!keyword || keyword[0] == '\0') return ALGO_SUCCESS;

    ProblemCollection result;
    AlgoStatus st = pcol_init(&result);
    if (st != ALGO_SUCCESS) return st;

    for (size_t i = 0; i < ctx->pcol.size; i++) {
        const Problem *p = &ctx->pcol.items[i];
        if (diff >= 0 && p->diff != (Difficulty)diff) continue;
        if (mastery >= 0 && p->mastery != (Mastery)mastery) continue;
        if (search_tag && !problem_has_tag(p, search_tag)) continue;
        if (today_only && !match_today(p->created_at)) continue;
        if (recent > 0 && !match_recent(p->created_at, recent)) continue;

        bool matched = false;
        if (keyword) {
            if (title_only) {
                matched = case_sensitive ? (strstr(p->title, keyword) != NULL) : (stristr(p->title, keyword) != NULL);
                if (title_text && !matched) continue;
            } else if (content) {
                matched = case_sensitive ? query_matches_keyword(p, keyword) : query_matches_keyword_ic(p, keyword);
                if (!matched && p->solution_path[0]) {
                    FILE *fp = utils_compat_fopen(p->solution_path, "r");
                    if (fp) {
                        char line[4096];
                        while (fgets(line, sizeof(line), fp)) {
                            bool hit = case_sensitive ? (strstr(line, keyword) != NULL) : (stristr(line, keyword) != NULL);
                            if (hit) { matched = true; break; }
                        }
                        fclose(fp);
                    }
                }
            } else {
                matched = case_sensitive ? query_matches_keyword(p, keyword) : query_matches_keyword_ic(p, keyword);
            }
        } else {
            matched = true;
        }

        if (matched) {
            st = pcol_add(&result, p);
            if (st != ALGO_SUCCESS) { pcol_free(&result); return st; }
            result.items[result.size - 1].id = p->id;
        }
    }

    if (limit > 0 && (size_t)limit < result.size) result.size = (size_t)limit;
    problem_view_print_list(&result, "没有匹配的题目", false);
    pcol_free(&result);
    return ALGO_SUCCESS;
}
