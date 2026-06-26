/**
 * @file command_config.h
 * @brief 配置管理命令声明。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <algorepo/runtime/runtime.h>

/**
 * @brief 执行配置管理命令。
 *
 * 处理 list/get/set/reset 子命令，管理工具配置项。
 *
 * @param ctx 运行时上下文。
 *
 * @return
 * - ALGO_SUCCESS      执行成功。
 * - ALGO_ERR_REQUIRED 缺少必要参数。
 * - ALGO_ERR_ARG      参数或子命令错误。
 * - ALGO_ERR_RANGE    路径过长。
 */
AlgoStatus cmd_config(int argc, char *argv[], RuntimeContext *ctx);

/**
 * @brief 显示 config 命令的帮助信息。
 */
void help_config(void);
