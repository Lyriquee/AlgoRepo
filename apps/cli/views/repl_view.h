/**
 * @file repl_view.h
 * @brief REPL 视图：启动横幅、状态信息与彩色提示显示。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <algorepo/runtime/runtime.h>

/**
 * @brief 打印 REPL 启动横幅和状态信息。
 *
 * 输出工作模式、目录、数据库、题解目录以及题库统计信息。
 */
void repl_view_print_banner(const RuntimeContext *ctx);

/**
 * @brief 打印 REPL 提示符。
 *
 * 提示符颜色根据运行模式和上次命令状态变化。
 *
 * @param last_status 上次命令的返回状态。
 */
void repl_view_print_prompt(const RuntimeContext *ctx, int last_status);
