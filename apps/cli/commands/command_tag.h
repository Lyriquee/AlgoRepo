/**
 * @file command_tag.h
 * @brief 标签管理命令声明。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <algorepo/runtime/runtime.h>

/**
 * @brief 执行标签管理命令。
 *
 * 提供标签的添加、移除、设置、清除、查看、重命名、删除及列表等子操作。
 *
 * @param ctx 运行时上下文。
 *
 * @return
 * - ALGO_SUCCESS      执行成功。
 * - ALGO_ERR_REQUIRED 缺少必要参数。
 * - ALGO_ERR_ARG      参数错误。
 * - ALGO_ERR_NOT_FOUND 题目不存在。
 */
AlgoStatus cmd_tag(int argc, char *argv[], RuntimeContext *ctx);

/**
 * @brief 显示 tag 命令的帮助信息。
 */
void help_tag(void);
