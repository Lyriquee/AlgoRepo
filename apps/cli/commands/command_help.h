/**
 * @file command_help.h
 * @brief 帮助与版本命令声明。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <algorepo/runtime/runtime.h>

/**
 * @brief 执行帮助命令。
 *
 * 显示全局帮助或指定命令的详细帮助信息。
 *
 * @return
 * - ALGO_SUCCESS  执行成功。
 * - ALGO_ERR_ARG  参数错误。
 */
AlgoStatus cmd_help(int argc, char *argv[], RuntimeContext *ctx);

/**
 * @brief 显示 help 命令自身的帮助信息。
 */
void help_help(void);

/**
 * @brief 执行版本命令。
 *
 * 显示 AlgoRepo CLI 的版本和构建信息。
 *
 * @return
 * - ALGO_SUCCESS  执行成功。
 */
AlgoStatus cmd_version(int argc, char *argv[], RuntimeContext *ctx);

/**
 * @brief 显示 version 命令自身的帮助信息。
 */
void help_version(void);
