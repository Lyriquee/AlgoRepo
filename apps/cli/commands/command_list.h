/**
 * @file command_list.h
 * @brief 列表浏览命令声明。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <algorepo/runtime/runtime.h>

/**
 * @brief 执行列表浏览命令。
 *
 * 按分类浏览题库中的题目，支持标签、难度、掌握度、时间过滤与排序。
 *
 * @param ctx 运行时上下文。
 *
 * @return
 * - ALGO_SUCCESS      执行成功。
 * - ALGO_ERR_ARG      参数错误。
 * - ALGO_ERR_NOT_FOUND 指定题目不存在。
 */
AlgoStatus cmd_list(int argc, char *argv[], RuntimeContext *ctx);

/**
 * @brief 显示 list 命令的帮助信息。
 */
void help_list(void);
