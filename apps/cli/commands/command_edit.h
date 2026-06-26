/**
 * @file command_edit.h
 * @brief 编辑题目命令声明。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <algorepo/runtime/runtime.h>

/**
 * @brief 执行编辑题目命令。
 *
 * 修改题目内容或元数据，支持命令行选项直接修改或交互式编辑。
 * 无选项时默认用编辑器打开源码文件。
 *
 * @param ctx 运行时上下文。
 *
 * @return
 * - ALGO_SUCCESS      编辑成功。
 * - ALGO_ERR_REQUIRED 缺少题目 ID。
 * - ALGO_ERR_ARG      参数错误。
 * - ALGO_ERR_NOT_FOUND 题目不存在。
 */
AlgoStatus cmd_edit(int argc, char *argv[], RuntimeContext *ctx);

/**
 * @brief 显示 edit 命令的帮助信息。
 */
void help_edit(void);
