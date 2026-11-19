/**
 * @file repl_view.c
 * @brief REPL 视图实现。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include "repl_view.h"

#include <stdio.h>
#include <string.h>

#include <algorepo/console/console_ui.h>
#include <algorepo/utils/utils_str.h>


/* --- 私有函数声明 ----------------------------------------------------------- */

static size_t repl_count_unique_tags(const ProblemCollection *pcol);


/* --- 公开接口实现 ----------------------------------------------------------- */

/* 打印 REPL 启动横幅和状态信息 */
void repl_view_print_banner(const RuntimeContext *ctx) {
    char sep[81];
    memset(sep, '=', 80);
    sep[80] = '\0';

    console_print_role(COLOR_ROLE_DIVIDER, "%s\n", sep);
    console_print_accent("                    AlgoRepo v%s  交互式题库管理工具\n",
                         ALGO_CLI_VERSION);
    console_print_role(COLOR_ROLE_DIVIDER, "%s\n\n", sep);

    // 工作模式
    console_print_primary("  工作模式 : ");
    if (ctx->global_mode)
        console_print_role(COLOR_ROLE_TITLE, "全局题库\n");
    else
        console_print_role(COLOR_ROLE_TITLE, "本地工作区\n");

    // 数据库
    console_print_primary("  数据库   : ");
    console_print_role(COLOR_ROLE_DESC, "%s\n",
                       ctx->db_path[0] ? ctx->db_path : "(未加载)");

    // 题解目录
    console_print_primary("  题解目录 : ");
    console_print_role(COLOR_ROLE_DESC, "%s\n",
                       ctx->solutions_path[0] ? ctx->solutions_path : "(未加载)");

    // 统计信息
    console_print_newline();
    console_print_primary("  题目数量 : ");
    console_print_role(COLOR_ROLE_DESC, "%lu\n",
                       (unsigned long)ctx->pcol.size);
    console_print_primary("  标签数量 : ");
    console_print_role(COLOR_ROLE_DESC, "%lu\n",
                       (unsigned long)repl_count_unique_tags(&ctx->pcol));

    // 提示
    console_print_newline();
    console_print_muted("  提示:\n");
    console_print_muted("    help / h    查看命令帮助\n");
    console_print_muted("    exit / q    保存并退出\n");
    console_print_muted("    !<cmd>      执行系统命令 (如 !ls)\n");
    console_print_divider();
}

/* 打印 REPL 提示符 */
void repl_view_print_prompt(const RuntimeContext *ctx, int last_status) {
    if (ctx->no_color) {
        console_print_text("algo❯ ");
        fflush(stdout);
        return;
    }

    // 全局模式使用标题颜色，本地模式使用主要颜色
    if (ctx->global_mode)
        console_print_accent("algo");
    else
        console_print_primary("algo");

    // 上次命令状态决定提示符颜色
    if (last_status == ALGO_SUCCESS)
        console_print_success("❯ ");
    else
        console_print_error_raw("❯ ");

    fflush(stdout);
}


/* --- 私有函数实现 ----------------------------------------------------------- */

/* 统计题库中不重复标签的数量 */
static size_t repl_count_unique_tags(const ProblemCollection *pcol) {
    char   seen[256][ALGO_MAX_TAG_LEN] = {{0}};
    size_t count                       = 0;

    for (size_t i = 0; i < pcol->size; i++) {
        const Problem *p = &pcol->items[i];
        for (size_t j = 0; j < p->tag_count; j++) {
            bool found = false;
            for (size_t k = 0; k < count; k++)
                if (strcmp(seen[k], p->tags[j]) == 0) {
                    found = true;
                    break;
                }
            if (!found && count < 256)
                utils_strcpy_safe(seen[count++], p->tags[j],
                                  ALGO_MAX_TAG_LEN);
        }
    }

    return count;
}
