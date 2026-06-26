/**
 * @file repl_loop.h
 * @brief REPL 交互引擎入口：生命周期衔接与主循环声明。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <algorepo/runtime/runtime.h>

/**
 * @brief 进入交互式 REPL 引擎。
 *
 * 装载运行时数据后启动 REPL 主循环。运行时生命周期由 cli_run 统一管理，
 * 本函数不负责 runtime_destroy。
 *
 * @param ctx 已通过 runtime_init 初始化的运行时上下文。
 * @return ALGO_SUCCESS 正常退出，其他值表示装载或运行阶段出错。
 */
AlgoStatus cli_enter_repl(RuntimeContext *ctx);

/**
 * @brief REPL 交互主循环。
 *
 * 打印启动横幅、注册 REPL 专用信号处理器、循环读取用户输入并分发命令，
 * 退出前恢复外层信号处理器。
 *
 * @param ctx 运行时上下文（必须已通过 runtime_load 装载数据）。
 * @return 0 正常退出（EOF 或用户 quit），-1 收到 SIGINT 中断。
 */
int repl_loop_run(RuntimeContext *ctx);
