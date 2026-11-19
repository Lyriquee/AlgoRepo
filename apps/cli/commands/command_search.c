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
#include <algorepo/utils/utils_compat.h>
#include <algorepo/utils/utils_str.h>

#include "command_common.h"
#include "views/problem_view.h"


/* --- 私有函数声明 --------------------------------------------------------- */

/** @brief 大小写感知的关键词匹配辅助 */
static bool kw_match(const char *haystack, const char *keyword, bool case_sensitive);

/** @brief 在内存中匹配单道题目，返回是否命中（纯计算，无 I/O） */
static bool search_match_filter(const Problem *p, const char *keyword,
                                bool case_sensitive, bool content_mode);

/** @brief -c 模式深度渲染：逐题显示匹配来源、行号与上下文 */
static void search_render_deep_view(const ProblemCollection *result,
                                     const char *keyword, bool case_sensitive);


/* --- 公开接口实现 --------------------------------------------------------- */

void help_search(void) {
    help_view_print_usage("search", "[options] <关键词>");
    help_view_print_description("在题库中精准检索题目，关键词默认在标题和标签中搜索");
    help_view_print_help_section("过滤选项 (先过滤再搜索):");
    help_view_print_help_option("-t, --title",        "<text>",    "限定标题包含指定文本");
    help_view_print_help_option("-g, --tag",          "<tag>",     "限定指定标签");
    help_view_print_help_option("-d, --diff",         "<diff>",    "限定题目难度 [0:未知/1:简单/2:中等/3:困难/4:极难]");
    help_view_print_help_option("-m, --mastery",      "<mastery>", "限定掌握程度 [0:未知/1:了解/2:熟悉/3:掌握/4:精通]");
    help_view_print_help_option("    --today",        NULL,        "限定今天操作过的题目");
    help_view_print_help_option("    --recent",       "<days>",    "限定最近 N 天内操作过的\n");
    help_view_print_help_section("搜索模式:");
    help_view_print_help_option("-c, --content",      NULL,        "深度内容搜索 (标题 + 标签 + 描述 + 笔记 + 源码)");
    help_view_print_help_option("-I, --case-sensitive", NULL,      "强制区分大小写");
    help_view_print_help_option("-n, --limit",        "<num>",     "限制输出数量 (默认 50)\n");
    help_view_print_help_section("说明:");
    console_print_muted("  1. 关键词默认仅在标题和标签中搜索，使用 -c 则扩展至描述、笔记和源码内容\n");
    console_print_muted("  2. -c 模式下将显式标注匹配来源，其中[笔记]与[源码]会自带精确行号以便后续编辑\n");
    console_print_muted("  3. 所有 -t/-g/-d/-m 过滤选项可任意组合，系统将先级联过滤，再匹配关键词\n\n");
    help_view_print_help_section("示例:");
    help_view_print_help_example("algo s \"hash\"");
    help_view_print_help_example("algo s \"dfs\" -c");
    help_view_print_help_example("algo s \"dp\" -g array -d 中等");
    help_view_print_help_example("algo s \"sort\" -t 快速 -m 精通");
    help_view_print_help_example("algo s \"tree\" -c --today");
}

/* CLI 入口：解析参数、交互回退、组织过滤→渲染管线 */
AlgoStatus cmd_search(int argc, char *argv[], RuntimeContext *ctx) {
    bool content = false, case_sensitive = false, today_only = false;
    const char *search_tag = NULL, *title_filter = NULL;
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
            case 't': title_filter = optarg; break;
            case 'g': search_tag = optarg; break;
            case 'd': if (parse_difficulty(optarg, &diff) != ALGO_SUCCESS) { console_log_error("无效难度"); return ALGO_ERR_ARG; } break;
            case 'm': if (parse_mastery(optarg, &mastery) != ALGO_SUCCESS) { console_log_error("无效掌握度: %s，请使用 0-4 或中文名称", optarg); return ALGO_ERR_ARG; } break;
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
        console_print_prompt("请输入搜索关键词 (输入 q 取消): ");
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

    /* 执行过滤 + 匹配管线 */
    ProblemCollection result;
    AlgoStatus st = pcol_init(&result);
    if (st != ALGO_SUCCESS) return st;

    for (size_t i = 0; i < ctx->pcol.size; i++) {
        const Problem *p = &ctx->pcol.items[i];

        /* 级联过滤（纯限定条件，不涉及关键词） */
        if (diff >= 0 && p->diff != (Difficulty)diff) continue;
        if (mastery >= 0 && p->mastery != (Mastery)mastery) continue;
        if (search_tag && !problem_has_tag(p, search_tag)) continue;
        if (title_filter && !stristr(p->title, title_filter)) continue;
        if (today_only && !match_today(p->created_at)) continue;
        if (recent > 0 && !match_recent(p->created_at, recent)) continue;

        /* 关键词匹配（纯计算） */
        if (search_match_filter(p, keyword, case_sensitive, content)) {
            st = pcol_add(&result, p);
            if (st != ALGO_SUCCESS) { pcol_free(&result); return st; }
            result.items[result.size - 1].id = p->id;
        }
    }

    if (limit > 0 && (size_t)limit < result.size) result.size = (size_t)limit;

    // 渲染输出
    if (result.size == 0) {
        console_log_info("没有匹配的题目");
    } else if (content) {
        search_render_deep_view(&result, keyword, case_sensitive);
    } else {
        problem_view_print_list(&result, NULL, false);
    }

    pcol_free(&result);
    return ALGO_SUCCESS;
}


/* --- 私有函数实现 --------------------------------------------------------- */

/* 大小写感知匹配：case_sensitive 时用 strstr，否则用 stristr */
static bool kw_match(const char *haystack, const char *keyword, bool case_sensitive) {
    if (!haystack || !keyword) return false;
    return case_sensitive
        ? (strstr(haystack, keyword) != NULL)
        : (stristr(haystack, keyword) != NULL);
}

/* 纯内存匹配：判断单道题目是否命中关键词 */
static bool search_match_filter(const Problem *p, const char *keyword,
                                 bool case_sensitive, bool content_mode) {
    // 标题匹配
    if (kw_match(p->title, keyword, case_sensitive)) return true;

    // 标签匹配
    for (size_t j = 0; j < p->tag_count; j++) {
        if (kw_match(p->tags[j], keyword, case_sensitive)) return true;
    }

    if (!content_mode) return false;

    // -c 模式：额外搜索描述、笔记、源码
    if (p->desc[0] && kw_match(p->desc, keyword, case_sensitive)) return true;
    if (p->notes[0] && kw_match(p->notes, keyword, case_sensitive)) return true;
    if (p->solution_path[0]) {
        FILE *fp = utils_compat_fopen(p->solution_path, "r");
        if (fp) {
            char sline[4096];
            bool found = false;
            while (fgets(sline, sizeof(sline), fp)) {
                if (kw_match(sline, keyword, case_sensitive)) {
                    found = true;
                    break;
                }
            }
            fclose(fp);
            if (found) return true;
        }
    }

    return false;
}

/* 对多行文本逐行搜索，匹配时通过回调输出行号与行内容。
 * 使用手动指针遍历替代 strtok，确保空行被正确计入行号 */
static void search_lines_with_numbers(const char *text, const char *keyword,
                                       bool case_sensitive,
                                       void (*emit)(int lineno, const char *line, void *user),
                                       void *user) {
    if (!text || !text[0]) return;
    const char *p   = text;
    const char *sol = text;
    int lineno = 1;

    while (*p) {
        if (*p == '\n') {
            size_t len = (size_t)(p - sol);
            if (len > 0 || sol < p) {
                char ch = *p;
                *(char *)p = '\0';
                if (kw_match(sol, keyword, case_sensitive))
                    emit(lineno, sol, user);
                *(char *)p = ch;
            }
            lineno++;
            p++;
            sol = p;
        } else {
            p++;
        }
    }
    if (*sol) {
        if (kw_match(sol, keyword, case_sensitive))
            emit(lineno, sol, user);
    }
}

/* 用于 search_lines_with_numbers 回调的上下文 */
typedef struct {
    const char *label;          // "描述" / "笔记"
    bool       show_lineno;     // 是否显示行号
} LineMatchCtx;

static void emit_deep_line(int lineno, const char *line, void *user) {
    LineMatchCtx *ctx = (LineMatchCtx *)user;
    // 截断过长行，保证 UTF-8 边界安全
    size_t ll = strlen(line);
    if (ll > 100) {
        int cut = 100;
        while (cut > 0 && (line[cut] & 0xC0) == 0x80) cut--;
        if (ctx->show_lineno)
            console_print_muted("  ↳ [%s:%d] 命中: ", ctx->label, lineno);
        else
            console_print_muted("  ↳ [%s] 命中: ", ctx->label);
        console_print_role(COLOR_ROLE_VALUE, "%.*s...\n", cut, line);
    } else {
        if (ctx->show_lineno)
            console_print_muted("  ↳ [%s:%d] 命中: ", ctx->label, lineno);
        else
            console_print_muted("  ↳ [%s] 命中: ", ctx->label);
        console_print_role(COLOR_ROLE_VALUE, "%s\n", line);
    }
}

/* 模式深度渲染：带分隔线、行号、匹配来源标注 */
static void search_render_deep_view(const ProblemCollection *result,
                                     const char *keyword, bool case_sensitive) {
    problem_view_print_header(false);
    for (size_t i = 0; i < result->size; i++) {
        if (i > 0) console_print_newline();
        const Problem *rp = &result->items[i];
        problem_view_print_row(rp, false);

        // 标题匹配
        if (kw_match(rp->title, keyword, case_sensitive)) {
            console_print_muted("  ↳ [标题] 命中: ");
            console_print_role(COLOR_ROLE_VALUE, "%s\n", rp->title);
        }

        // 标签匹配：合并为一行
        {
            bool first = true;
            for (size_t j = 0; j < rp->tag_count; j++) {
                if (kw_match(rp->tags[j], keyword, case_sensitive)) {
                    if (first) {
                        console_print_muted("  ↳ [标签] 命中: ");
                        first = false;
                    } else {
                        console_print_text(" ");
                    }
                    console_print_role(COLOR_ROLE_VALUE, "%s", rp->tags[j]);
                }
            }
            if (!first) console_print_newline();
        }

        // 描述匹配：逐行，不带行号
        if (rp->desc[0]) {
            LineMatchCtx dctx = { "描述", false };
            search_lines_with_numbers(rp->desc, keyword, case_sensitive,
                                       emit_deep_line, &dctx);
        }

        // 笔记匹配：逐行，带行号
        if (rp->notes[0]) {
            LineMatchCtx nctx = { "笔记", true };
            search_lines_with_numbers(rp->notes, keyword, case_sensitive,
                                       emit_deep_line, &nctx);
        }

        // 源码匹配：逐行，带行号
        if (rp->solution_path[0]) {
            FILE *fp = utils_compat_fopen(rp->solution_path, "r");
            if (fp) {
                char sline[4096];
                int slineno = 0;
                while (fgets(sline, sizeof(sline), fp)) {
                    slineno++;
                    if (kw_match(sline, keyword, case_sensitive)) {
                        // 裁剪首尾空白
                        char *t = sline;
                        while (*t == ' ' || *t == '\t') t++;
                        size_t tl = strlen(t);
                        while (tl > 0 && (t[tl-1] == '\n' || t[tl-1] == '\r' || t[tl-1] == ' '))
                            t[--tl] = '\0';
                        console_print_muted("  ↳ [源码:%d] 命中: ", slineno);
                        if (tl > 100) {
                            int cut = 100;
                            while (cut > 0 && (t[cut] & 0xC0) == 0x80) cut--;
                            console_print_role(COLOR_ROLE_VALUE, "%.*s...\n", cut, t);
                        } else {
                            console_print_role(COLOR_ROLE_VALUE, "%s\n", t);
                        }
                    }
                }
                fclose(fp);
            }
        }
    }
    console_print_divider();
}
