/**
 * @file command_init.h
 * @brief 初始化工作区命令声明。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <algorepo/runtime/runtime.h>

/**
 * @brief 执行初始化工作区命令。
 *
 * 在当前目录（或全局 ~/.algorepo）创建 AlgoRepo 工作区结构。
 *
 * @param ctx 运行时上下文。
 *
 * @return
 * - ALGO_SUCCESS      初始化成功。
 * - ALGO_ERR_ARG      参数错误。
 * - ALGO_ERR_FILE     工作区已存在且未指定强制覆盖。
 * - ALGO_ERR_RANGE    路径过长。
 */
AlgoStatus cmd_init(int argc, char *argv[], RuntimeContext *ctx);

/**
 * @brief 显示 init 命令的帮助信息。
 */
void help_init(void);
