/**
 * @file help_view.h
 * @brief 帮助信息视图：版本信息、主帮助、子命令帮助及共享打印函数。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>

/**
 * @brief 打印版本信息。
 */
void help_view_print_version(void);


/* --- 横幅与主帮助 ----------------------------------------------------------- */

/**
 * @brief 打印启动横幅。
 */
void help_view_print_banner(void);

/**
 * @brief 打印主帮助信息（所有命令列表与全局选项）。
 */
void help_view_print_main(void);


/* --- 子命令帮助 ------------------------------------------------------------- */

/**
 * @brief 打印子命令详细帮助。
 *
 * @param subcmd 子命令名称。
 */
void help_view_print_sub_cmd(const char *subcmd);

/**
 * @brief 打印单个命令条目（名称、别名、描述）。
 */
void help_view_print_command_entry(const char *name, const char *alias,
                                   const char *desc);


/* --- 共享的帮助打印函数（供所有 commands/ 中的 help_* 使用） ---------------- */

/**
 * @brief 打印用法行 "用法: algo <cmd> [options]"。
 */
void help_view_print_usage(const char *cmd, const char *option);

/**
 * @brief 打印命令描述段落。
 */
void help_view_print_description(const char *desc);

/**
 * @brief 打印帮助选项行（对齐到第 32 列）。
 */
void help_view_print_help_option(const char *flags, const char *param,
                                 const char *desc);

/**
 * @brief 打印帮助小节标题。
 */
void help_view_print_help_section(const char *title);

/**
 * @brief 打印帮助示例行。
 */
void help_view_print_help_example(const char *example);
