/**
 * @file repl_cmd.h
 * @brief REPL 命令分发器与 REPL 专属命令处理器声明。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 *
 * 这些命令仅在交互式 REPL 内可用，通过 command.c 中的 REPL_COMMANDS 表注册。
 */

#pragma once

#include "command.h"

/* --- REPL 命令分发 -------------------------------------------------------- */

/**
 * @brief REPL 命令分发入口。
 *
 * 解析输入行，查找命令表并执行。内置 exit/quit/q 退出、
 * ! 前缀执行外部命令、-h/--help 打印帮助等子逻辑。
 *
 * @param ctx  运行时上下文。
 * @param line 用户输入行（可被修改）。
 * @return ALGO_ERR_EOF 表示用户请求退出，否则返回命令执行结果。
 */
int repl_cmd_dispatch(RuntimeContext *ctx, const char *line);

/* --- REPL 专属命令处理器 --------------------------------------------------- */

/** @brief 切换至全局题库模式。 */
AlgoStatus cmd_global(int argc, char *argv[], RuntimeContext *ctx);
void       help_global(void);

/** @brief 切换至本地工作区模式。 */
AlgoStatus cmd_local(int argc, char *argv[], RuntimeContext *ctx);
void       help_local(void);

/** @brief 手动将内存数据持久化落盘。 */
AlgoStatus cmd_save(int argc, char *argv[], RuntimeContext *ctx);
void       help_save(void);

/** @brief 清除终端屏幕显示。 */
AlgoStatus cmd_clear(int argc, char *argv[], RuntimeContext *ctx);
void       help_clear(void);
