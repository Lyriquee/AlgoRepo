/**
 * @file command_stats.h
 * @brief 统计仪表盘命令声明。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <algorepo/runtime/runtime.h>

/**
 * @brief 执行统计命令。
 *
 * 展示题库的统计仪表盘，包括难度分布、掌握度分布和高频标签。
 *
 * @param ctx 运行时上下文。
 *
 * @return
 * - ALGO_SUCCESS  执行成功。
 * - ALGO_ERR_ARG  参数错误。
 */
AlgoStatus cmd_stats(int argc, char *argv[], RuntimeContext *ctx);

/**
 * @brief 显示 stats 命令的帮助信息。
 */
void help_stats(void);
