/**
 * @file command_rm.h
 * @brief 删除题目命令声明。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <algorepo/runtime/runtime.h>

/**
 * @brief 执行删除题目命令。
 *
 * 从数据库中移除题目记录，默认同步删除关联的源文件。
 *
 * @param ctx 运行时上下文。
 *
 * @return
 * - ALGO_SUCCESS      删除成功或用户取消。
 * - ALGO_ERR_REQUIRED 缺少题目 ID。
 * - ALGO_ERR_ARG      无效的 ID 格式。
 * - ALGO_ERR_NOT_FOUND 题目不存在。
 */
AlgoStatus cmd_rm(int argc, char *argv[], RuntimeContext *ctx);

/**
 * @brief 显示 rm 命令的帮助信息。
 */
void help_rm(void);
