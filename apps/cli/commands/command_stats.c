/**
 * @file command_stats.c
 * @brief 统计仪表盘命令实现。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include "command_stats.h"

#include <getopt.h>
#include <stdio.h>
#include <string.h>

#include <algorepo/console/console_ui.h>
#include <algorepo/service/problem_query.h>
#include <algorepo/utils/utils_str.h>

#include "command_common.h"
#include "views/problem_view.h"


/* --- 私有函数声明 --------------------------------------------------------- */

static void print_bar_difficulty(Difficulty diff, size_t count, size_t total, int bar_width);
static void print_bar_mastery(Mastery m, size_t count, size_t total, int bar_width);
static int  str_display_width(const char *s);


/* --- 公开接口实现 --------------------------------------------------------- */

/* 显示 stats 命令的帮助信息 */
void help_stats(void) {
    help_view_print_usage("stats", "[options]");
    help_view_print_description("展示题库的统计仪表盘");
    help_view_print_help_section("别名: stat, st");
    help_view_print_help_section("选项:");
    help_view_print_help_option("-h, --help",  NULL,      "显示此帮助信息");
    help_view_print_help_option("-g, --tag",   "<tag>",   "限定指定标签");
    help_view_print_help_option("-d, --diff",  "<diff>",  "限定指定难度");
    help_view_print_help_option("    --today", NULL,      "今天战报");
    help_view_print_help_option("    --recent","<days>",  "近期进展\n");
    help_view_print_help_section("示例:");
    help_view_print_help_example("algo st");
    help_view_print_help_example("algo st -g dp");
}

/* 执行 stats 命令：展示题库统计仪表盘 */
AlgoStatus cmd_stats(int argc, char *argv[], RuntimeContext *ctx) {
    const char *filter_tag = NULL;
    int filter_diff = -1;
    int recent = -1;
    (void)recent; // today/recent 筛选暂未实现

    static const struct option long_opts[] = {
        {"tag",    required_argument, 0, 'g'},
        {"diff",   required_argument, 0, 'd'},
        {"today",  no_argument,       0, 'T'},
        {"recent", required_argument, 0, 'R'},
        {0, 0, 0, 0}};

    optind = 1; opterr = 0;
    int c;
    while ((c = getopt_long(argc, argv, "g:d:", long_opts, NULL)) != -1) {
        switch (c) {
            case 'g': filter_tag = optarg; break;
            case 'd': if (parse_difficulty(optarg, &filter_diff) != ALGO_SUCCESS) { console_log_error("无效难度"); return ALGO_ERR_ARG; } break;
            case 'T': break; // today 筛选暂未实现
            case 'R': if (parse_int_arg(optarg, 1, 365, &recent) != ALGO_SUCCESS) { console_log_error("无效 days"); return ALGO_ERR_ARG; } break;
            case '?': return handle_getopt_error(optopt, "g:d:", argv, optind);
            default: return ALGO_ERR_ARG;
        }
    }

    // 统计过滤后的题目
    size_t diff_counts[5] = {0};
    size_t mastery_counts[5] = {0};
    size_t total = 0;
    char   tag_names[256][ALGO_MAX_TAG_LEN] = {{0}};
    size_t tag_counts[256] = {0};
    size_t tag_used = 0;

    for (size_t i = 0; i < ctx->pcol.size; i++) {
        const Problem *p = &ctx->pcol.items[i];
        if (filter_tag && !problem_has_tag(p, filter_tag)) continue;
        if (filter_diff >= 0 && p->diff != (Difficulty)filter_diff) continue;
        total++;
        if (p->diff >= 0 && p->diff <= 4) diff_counts[p->diff]++;
        if (p->mastery >= 0 && p->mastery <= 4) mastery_counts[p->mastery]++;
        for (size_t j = 0; j < p->tag_count; j++) {
            size_t k = 0;
            for (; k < tag_used; k++) if (strcmp(tag_names[k], p->tags[j]) == 0) break;
            if (k == tag_used && tag_used < 256) { utils_strcpy_safe(tag_names[tag_used], p->tags[j], sizeof(tag_names[0])); tag_used++; }
            if (k < 256) tag_counts[k]++;
        }
    }

    console_print_divider();
    console_print_primary("                 算法工作台看板\n");
    console_print_divider();
    console_print_primary("总题数: ");
    console_print_role(COLOR_ROLE_VALUE, "%llu 道\n", (unsigned long long)total);

    // 难度分布
    console_print_title("\n--- 难度分布 (Difficulty) ------------------------");
    console_print_newline();
    print_bar_difficulty(ALGO_DIFF_EASY,    diff_counts[1], total, 20);
    print_bar_difficulty(ALGO_DIFF_MEDIUM,  diff_counts[2], total, 20);
    print_bar_difficulty(ALGO_DIFF_HARD,    diff_counts[3], total, 20);
    print_bar_difficulty(ALGO_DIFF_EXTREME, diff_counts[4], total, 20);
    print_bar_difficulty(ALGO_DIFF_UNKNOWN, diff_counts[0], total, 20);

    // 掌握度分布
    console_print_title("\n--- 掌握度分布 (Mastery) -------------------------");
    console_print_newline();
    print_bar_mastery(ALGO_MASTERY_LEARNING, mastery_counts[1], total, 20);
    print_bar_mastery(ALGO_MASTERY_FAMILIAR, mastery_counts[2], total, 20);
    print_bar_mastery(ALGO_MASTERY_MASTERED, mastery_counts[3], total, 20);
    print_bar_mastery(ALGO_MASTERY_EXPERT,   mastery_counts[4], total, 20);
    print_bar_mastery(ALGO_MASTERY_UNKNOWN,     mastery_counts[0], total, 20);

    // 高频标签
    if (tag_used > 0) {
        console_print_title("\n--- 高频标签 (Top Tags) ----------------------------");
        console_print_newline();
        /* 表头 */
        console_print_spaces(2);
        console_print_role(COLOR_ROLE_PRIMARY, "标签");
        console_print_spaces(18);
        console_print_role(COLOR_ROLE_PRIMARY, "数量\n");
        console_print_spaces(2);
        console_print_role(COLOR_ROLE_DIVIDER, "────────────────────");
        console_print_spaces(2);
        console_print_role(COLOR_ROLE_DIVIDER, "────\n");
        /* 数据行：补齐显示宽度使计数值对齐 */
        for (size_t i = 0; i < tag_used && i < 8; i++) {
            int dw = str_display_width(tag_names[i]);
            int pad = (dw < 22) ? (22 - dw) : 1;
            console_print_spaces(2);
            console_print_role(COLOR_ROLE_TAG, "%s", tag_names[i]);
            console_print_spaces(pad);
            console_print_role(COLOR_ROLE_VALUE, "%llu\n",
                               (unsigned long long)tag_counts[i]);
        }
    }
    console_print_divider();

    return ALGO_SUCCESS;
}


/* --- 私有函数实现 --------------------------------------------------------- */

/* 难度柱状条 */
static void print_bar_difficulty(Difficulty diff, size_t count, size_t total, int bar_width) {
    double percent = total == 0 ? 0.0 : (double)count * 100.0 / (double)total;
    int filled = total == 0 ? 0 : (int)((double)count * (double)bar_width / (double)total);
    console_print_spaces(2);
    problem_view_print_difficulty(diff);
    console_print_role(COLOR_ROLE_VALUE, " %3llu (%5.1f%%) [",
                       (unsigned long long)count, percent);
    for (int i = 0; i < bar_width; i++) {
        if (i < filled) console_print_role(COLOR_ROLE_BAR_FILL, "#");
        else            console_print_role(COLOR_ROLE_BAR_EMPTY, "-");
    }
    console_print_role(COLOR_ROLE_VALUE, "]\n");
}

/* 掌握度柱状条 */
static void print_bar_mastery(Mastery m, size_t count, size_t total, int bar_width) {
    double percent = total == 0 ? 0.0 : (double)count * 100.0 / (double)total;
    int filled = total == 0 ? 0 : (int)((double)count * (double)bar_width / (double)total);
    console_print_spaces(2);
    problem_view_print_mastery(m);
    console_print_role(COLOR_ROLE_VALUE, " %3llu (%5.1f%%) [",
                       (unsigned long long)count, percent);
    for (int i = 0; i < bar_width; i++) {
        if (i < filled) console_print_role(COLOR_ROLE_BAR_FILL, "#");
        else            console_print_role(COLOR_ROLE_BAR_EMPTY, "-");
    }
    console_print_role(COLOR_ROLE_VALUE, "]\n");
}

/* 计算 UTF-8 字符串的终端显示宽度（ASCII=1，CJK/全角=2） */
static int str_display_width(const char *s) {
    if (!s) return 0;
    int width = 0;
    while (*s) {
        unsigned char c = (unsigned char)*s;
        if (c < 0x80) {
            /* ASCII：单字节，显示宽度 1 */
            width++;
            s++;
        } else if ((c & 0xE0) == 0xC0) {
            /* 双字节 UTF-8 序列（Latin 扩展等），宽度 1 */
            width++;
            s += 2;
        } else if ((c & 0xF0) == 0xE0) {
            /* 三字节 UTF-8 序列（CJK 汉字等），宽度 2 */
            width += 2;
            s += 3;
        } else if ((c & 0xF8) == 0xF0) {
            /* 四字节 UTF-8 序列（Emoji 等），宽度 2 */
            width += 2;
            s += 4;
        } else {
            /* 非法序列，跳过 */
            width++;
            s++;
        }
    }
    return width;
}
