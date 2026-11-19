/**
 * @file help_view.c
 * @brief 帮助信息视图实现。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include "help_view.h"

#include <stdio.h>
#include <string.h>

#include <algorepo/console/console_ui.h>
#include <algorepo/storage/storage_file.h>

#include "command.h"


/* --- 版本信息 --------------------------------------------------------------- */

/* 打印版本信息 */
void help_view_print_version(void) {
    // 定义于apps/cli/CMakeLists.txt
    console_print_role(COLOR_ROLE_TITLE, "AlgoRepo CLI %s\n", ALGO_CLI_VERSION);
    console_print_role(COLOR_ROLE_SECONDARY, "Data format: ARDB v%d\n",
                       ALGO_FILE_VERSION);
}


/* --- 私有函数声明 ----------------------------------------------------------- */

/* 打印一组命令列表（标题 + 循环条目） */
static void help_view_print_command_group(const char *title,
                                          int (*count_fn)(void),
                                          const Command *(*get_fn)(int));


/* --- 横幅与主帮助 ----------------------------------------------------------- */

/* 打印启动横幅 */
void help_view_print_banner(void) {
    console_print_separator('-', 60);
    console_print_accent("     AlgoRepo v%s  交互式题库管理工具\n",
                         ALGO_CLI_VERSION);
    console_print_separator('-', 60);
}

/* 打印主帮助信息（所有命令列表与全局选项） */
void help_view_print_main(void) {
    help_view_print_usage("<command>", "[options]");
    console_print_role(COLOR_ROLE_DESC,
                       "AlgoRepo 是一个本地算法题代码仓库管理工具\n\n");

    // 核心命令列表
    help_view_print_command_group("核心命令:", command_core_count,
                                  command_core_get);

    // REPL 专属命令列表
    console_print_newline();
    help_view_print_command_group("REPL 专属命令:", command_repl_count,
                                  command_repl_get);

    // 全局选项
    console_print_newline();
    help_view_print_help_section("全局选项:");
    help_view_print_help_option("-g, --global", NULL, "使用用户全局题库");
    help_view_print_help_option("    --no-color", NULL,
                                "强制关闭颜色输出");
    help_view_print_help_option("-h, --help", NULL, "显示帮助");
    help_view_print_help_option("-V, --version", NULL,
                                "显示版本信息\n");
    console_print_muted("使用 \"algo help <命令>\" 查看详细用法\n");
}


/* --- 子命令帮助 ------------------------------------------------------------- */

/* 打印单个命令条目（名称、别名、描述） */
void help_view_print_command_entry(const char *name, const char *alias,
                                   const char *desc) {
    if (!name || !desc) return;

    char alias_buf[16] = "";
    if (alias && alias[0])
        snprintf(alias_buf, sizeof(alias_buf), "(%s)", alias);

    console_print_accent("  %-9s ", name);
    console_print_secondary("%-7s ", alias_buf);
    console_print_role(COLOR_ROLE_DESC, "%s\n", desc);
}

/* 打印子命令详细帮助 */
void help_view_print_sub_cmd(const char *subcmd) {
    const Command *cmd = command_find(subcmd);
    if (!cmd) {
        console_log_error("未知命令 \"%s\"", subcmd);
        return;
    }
    if (cmd->help_fn)
        cmd->help_fn();
    else
        console_log_warn("命令 \"%s\" 暂无详细帮助信息", subcmd);
}


/* --- 共享的帮助打印函数 ----------------------------------------------------- */

/* 打印用法行 */
void help_view_print_usage(const char *cmd, const char *option) {
    if (!cmd || !option) return;
    console_print_title("用法:");
    console_print_accent("  algo");
    console_print_muted(" %s", cmd);
    console_print_role(COLOR_ROLE_PARAM, " %s\n\n", option);
}

void help_view_print_description(const char *desc) {
    if (!desc) return;
    console_print_role(COLOR_ROLE_DESC, "%s\n\n", desc);
}

/* 打印帮助选项行（对齐到第 32 列） */
void help_view_print_help_option(const char *flags, const char *param,
                                 const char *desc) {
    console_print_spaces(2);
    console_print_primary("%s", flags);
    if (param && param[0]) {
        console_print_spaces(1);
        console_print_role(COLOR_ROLE_PARAM, "%s", param);
    }
    int used = 2 + (int)strlen(flags) +
               (param && param[0] ? 1 + (int)strlen(param) : 0);
    int pad  = 32 - used;
    if (pad < 1) pad = 1;
    console_print_spaces(pad);
    console_print_role(COLOR_ROLE_DESC, "%s\n", desc);
}

/* 打印帮助小节标题 */
void help_view_print_help_section(const char *title) {
    console_print_title(title);
}

/* 打印帮助示例行 */
void help_view_print_help_example(const char *example) {
    console_print_role(COLOR_ROLE_EXAMPLE, "  %s\n", example);
}


/* --- 私有函数实现 ----------------------------------------------------------- */

/* 打印一组命令列表 */
static void help_view_print_command_group(const char *title,
                                          int (*count_fn)(void),
                                          const Command *(*get_fn)(int)) {
    int count = count_fn();
    if (count == 0) return;
    console_print_title(title);
    for (int i = 0; i < count; i++) {
        const Command *c = get_fn(i);
        help_view_print_command_entry(c->name, c->alias, c->desc);
    }
}
