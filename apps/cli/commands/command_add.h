/**
 * @file command_add.h
 * @brief 添加题目命令声明。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <algorepo/runtime/runtime.h>

/**
 * @brief 执行添加题目命令。
 *
 * 将新题目添加到本地题库，支持命令行选项直接指定元数据，
 * 或省略参数进入交互式向导补全。
 *
 * @param ctx 运行时上下文。
 *
 * @return
 * - ALGO_SUCCESS      添加成功。
 * - ALGO_ERR_ARG      参数错误。
 * - ALGO_ERR_REQUIRED 缺少必填信息。
 * - ALGO_ERR_NOT_FOUND 源文件不存在。
 * - ALGO_ERR_FILE     文件操作失败。
 * - ALGO_ERR_RANGE    路径过长。
 */
AlgoStatus cmd_add(int argc, char *argv[], RuntimeContext *ctx);

/**
 * @brief 显示 add 命令的帮助信息。
 */
void help_add(void);
