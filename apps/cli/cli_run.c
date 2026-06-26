/**
 * @file cli_run.c
 * @brief CLI 生命周期实现。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include "cli_run.h"

#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <algorepo/runtime/runtime.h>
#include <algorepo/console/console_ui.h>

#include "command.h"
#include "repl/repl_loop.h"
#include "views/help_view.h"


/* 全局信号状态------------------------------------------------------ */

/* 全局运行时上下文探针，供信号处理器感知当前模式 */
static RuntimeContext *g_active_ctx = NULL;

/* SIGINT 触发标志（异步安全）*/
static volatile sig_atomic_t g_sigint_fired = 0;


/* --- 私有函数声明---------------------------------------------------------- */

static void       cli_sigint_handler(int sig);
static AlgoStatus cli_dispatch_direct(RuntimeContext *ctx, int argc,
                                      char *argv[]);
static bool       cli_is_help_option(const char *arg);


/* --- 公开接口实现---------------------------------------------------------- */

/* CLI 生命周期入口：解析全局选项，初始化上下文，双轨道分流 */
int cli_run(int argc, char *argv[]) {
    bool  global_mode  = false;
    bool  no_color     = false;
    bool  found_subcmd = false;
    char *filtered[256];
    int   filtered_count = 0;

    // 解析全局选项
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--no-color") == 0) {
            no_color = true;
            continue;
        }
        if (strcmp(argv[i], "--version") == 0 ||
            strcmp(argv[i], "-V") == 0) {
            help_view_print_version();
            return EXIT_SUCCESS;
        }
        if (!found_subcmd && cli_is_help_option(argv[i])) {
            help_view_print_main();
            return EXIT_SUCCESS;
        }
        if (!found_subcmd && (strcmp(argv[i], "-g") == 0 ||
                              strcmp(argv[i], "--global") == 0)) {
            global_mode = true;
            continue;
        }

        bool is_option = (argv[i][0] == '-');
        if (!is_option && !found_subcmd) found_subcmd = true;
        if (is_option && !found_subcmd) {
            console_log_error("未知全局选项: \"%s\"", argv[i]);
            return EXIT_FAILURE;
        }
        filtered[filtered_count++] = argv[i];
    }

    // 统一生命周期起点
    RuntimeContext ctx;
    if (runtime_init(&ctx, global_mode) != ALGO_SUCCESS) {
        console_log_fatal("环境句柄初始化失败");
        return EXIT_FAILURE;
    }
    ctx.no_color       = no_color;
    ctx.is_repl_active = false;
    if (no_color) console_set_color_mode("never");

    // 挂载全局探针，注册信号处理器
    g_active_ctx            = &ctx;
    void (*old_sigint)(int) = signal(SIGINT, cli_sigint_handler);
#ifdef SIGBREAK
    void (*old_sigbreak)(int) = signal(SIGBREAK, cli_sigint_handler);
#endif
#ifdef SIGTERM
    void (*old_sigterm)(int) = signal(SIGTERM, cli_sigint_handler);
#endif

    AlgoStatus status = ALGO_SUCCESS;

    // 双轨道分流
    if (filtered_count < 1) {
        // 轨道一：交互 REPL
        ctx.is_repl_active = true;
        status             = cli_enter_repl(&ctx);
    } else if (command_is_repl_only(filtered[0])) {
        // REPL 专属命令拒绝直出执行
        console_log_error(
            "\"%s\" 是控制台专属命令，请在交互式 Shell 中使用",
            filtered[0]);
        status = ALGO_ERR_ARG;
    } else {
        // 轨道二：单次直出模式
        status = cli_dispatch_direct(&ctx, filtered_count, filtered);
    }

    // 统一生命周期终点
    if (runtime_destroy(&ctx) != ALGO_SUCCESS) {
        console_log_error("释放或持久化运行环境资源失败");
        if (status == ALGO_SUCCESS) status = ALGO_ERR_FILE;
    }

    // 卸载探针并恢复信号
    g_active_ctx = NULL;
    signal(SIGINT, old_sigint);
#ifdef SIGBREAK
    signal(SIGBREAK, old_sigbreak);
#endif
#ifdef SIGTERM
    signal(SIGTERM, old_sigterm);
#endif

    return (status == ALGO_SUCCESS) ? EXIT_SUCCESS : EXIT_FAILURE;
}


/* --- 私有函数实现---------------------------------------------------------- */

/* SIGINT 信号处理器：REPL 模式下仅设置标志位，直出或无上下文时直接退出 */
static void cli_sigint_handler(int sig) {
    (void)sig;
    g_sigint_fired = 1;

    // REPL 模式仅留标志位；直出或无上下文时直接退出
    if (!g_active_ctx || !g_active_ctx->is_repl_active) {
        _Exit(EXIT_FAILURE);
    }
}

/* 单次命令调度器（仅查 CORE 表） */
static AlgoStatus cli_dispatch_direct(RuntimeContext *ctx, int argc,
                                      char *argv[]) {
    const char    *cmd_name = argv[0];
    const Command *cmd      = command_find_core(cmd_name);

    if (!cmd) {
        console_log_error("未知命令: \"%s\"", cmd_name);
        return ALGO_ERR_ARG;
    }

    // 检查是否有帮助选项
    for (int i = 1; i < argc; i++) {
        if (cli_is_help_option(argv[i])) {
            if (cmd->help_fn) cmd->help_fn();
            return ALGO_SUCCESS;
        }
    }

    // 需要工作区的命令先加载运行时
    if (cmd->require_workspace) {
        AlgoStatus status = runtime_load(ctx);
        if (status == ALGO_ERR_NOT_FOUND) {
            if (ctx->global_mode) {
                console_log_error("全局题库尚未初始化，请先执行: algo init -g");
            } else {
                console_log_error("当前目录未找到工作区，请执行 algo init 创建，或使用 -g 访问全局题库");
            }
            return status;
        }
        if (status != ALGO_SUCCESS) {
            console_log_fatal("工作区装载失败");
            return status;
        }
        if (!ctx->no_color) console_set_color_mode(ctx->config.color);
    }

    return cmd->handler(argc, argv, ctx);
}

/* 判断参数是否为帮助选项 */
static bool cli_is_help_option(const char *arg) {
    return strcmp(arg, "-h") == 0 ||
           strcmp(arg, "--help") == 0 ||
           strcmp(arg, "-help") == 0;
}

