/**
 * @file command_search.h
 * @brief 搜索题目命令声明。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <algorepo/runtime/runtime.h>

/**
 * @brief 执行搜索题目命令。
 *
 * 在题库中按关键词、标签、难度、掌握度等条件检索题目，
 * 支持内容深度搜索。
 *
 * @param ctx 运行时上下文。
 *
 * @return
 * - ALGO_SUCCESS  执行成功。
 * - ALGO_ERR_ARG  参数错误。
 */
AlgoStatus cmd_search(int argc, char *argv[], RuntimeContext *ctx);

/**
 * @brief 显示 search 命令的帮助信息。
 */
void help_search(void);
