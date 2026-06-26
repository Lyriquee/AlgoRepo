/**
 * @file cli_run.h
 * @brief CLI 生命周期入口：信号处理、全局选项解析、双轨道分流。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#pragma once

/**
 * @brief CLI 生命周期入口。
 *
 * 解析全局选项，初始化运行时上下文，根据参数选择 REPL 交互模式或直出单次模式。
 *
 * @return EXIT_SUCCESS 或 EXIT_FAILURE。
 */
int cli_run(int argc, char *argv[]);
