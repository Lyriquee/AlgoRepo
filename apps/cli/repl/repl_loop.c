/**
 * @file repl_loop.c
 * @brief REPL 交互循环：信号处理、提示显示与命令分发主循环。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include "repl_loop.h"

#include <signal.h>
#include <stdio.h>

#include <algorepo/console/console_ui.h>

#include "repl_cmd.h"
#include "views/repl_view.h"

/* --- 信号处理 ------------------------------------------------------------- */

/* SIGINT 中断标志（异步安全） */
static volatile sig_atomic_t g_interrupted = 0;

/*
 * REPL 模式 SIGINT 信号处理器。
 *
 * 设置中断标志并重新注册自身。主循环轮询该标志以决定是否退出，
 * 不在此处执行任何非异步安全操作
 */
static void repl_sigint_handler(int sig) {
    (void)sig;
    g_interrupted = 1;
    signal(SIGINT, repl_sigint_handler);
}

/* --- 公开接口实现 --------------------------------------------------------- */

/* 进入 REPL 交互模式 */
AlgoStatus cli_enter_repl(RuntimeContext *ctx) {
    AlgoStatus st = runtime_load(ctx);
    if (st == ALGO_ERR_NOT_FOUND && !ctx->global_mode) {
        console_log_error("当前目录不是一个 AlgoRepo 工作区");
        console_log_info("请先运行 \"algo init\" 初始化工作区，或运行 'algo -g' 进入全局交互模式");
        return st;
    }
    if (st != ALGO_SUCCESS) {
        console_log_fatal("题库数据加载失败");
        return st;
    }

    /* 根据配置项设置颜色模式（与直出模式行为一致） */
    if (!ctx->no_color) console_set_color_mode(ctx->config.color);

    repl_loop_run(ctx);
    return ALGO_SUCCESS;
}

/* REPL 主循环 */
int repl_loop_run(RuntimeContext *ctx) {
    repl_view_print_banner(ctx);

    // 保存外层信号处理器（cli_run 安装的），安装 REPL 专用处理器
    void (*old_sigint)(int) = signal(SIGINT, repl_sigint_handler);

    char       line[4096];
    AlgoStatus last_status = ALGO_SUCCESS;

    while (!g_interrupted) {
        repl_view_print_prompt(ctx, last_status);

        if (!fgets(line, sizeof(line), stdin)) {
            break;  // EOF
        }

        if (g_interrupted) break;

        AlgoStatus st = repl_cmd_dispatch(ctx, line);
        if (st == ALGO_ERR_EOF) break;  // 用户请求退出
        last_status = st;
    }

    // 中断退出提示
    if (g_interrupted) {
        console_print_muted("^C\n");
        console_log_warn("收到中断信号，正在安全退出...");
    }

    // 恢复外层信号处理器
    signal(SIGINT, old_sigint);

    return g_interrupted ? -1 : 0;
}
