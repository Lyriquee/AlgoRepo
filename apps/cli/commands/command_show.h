/**
 * @file command_show.h
 * @brief 题目详情查看命令声明。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <algorepo/runtime/runtime.h>

/**
 * @brief 执行查看题目详情命令。
 *
 * 查看指定题目的详细信息，支持显示完整信息和源代码。
 *
 * @param ctx 运行时上下文。
 *
 * @return
 * - ALGO_SUCCESS      执行成功。
 * - ALGO_ERR_REQUIRED 缺少题目 ID。
 * - ALGO_ERR_ARG      无效的 ID 格式。
 * - ALGO_ERR_NOT_FOUND 题目不存在。
 */
AlgoStatus cmd_show(int argc, char *argv[], RuntimeContext *ctx);

/**
 * @brief 显示 show 命令的帮助信息。
 */
void help_show(void);
