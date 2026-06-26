/**
 * @file repl_cmd.c
 * @brief REPL 专属命令处理器及命令行分词调度器实现。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include "repl_cmd.h"

#include "views/help_view.h"
#include <stdlib.h>
#include <string.h>

#include <algorepo/console/console_ui.h>
#include <algorepo/storage/storage.h>
#include <algorepo/utils/utils_compat.h>
#include <algorepo/utils/utils_str.h>


/* --- 私有函数声明 --------------------------------------------------------- */

/* 将输入行按空白切分为 argv 数组 */
static int tokenize(char *line, char *argv[], int max_args);


/* --- REPL 专属命令处理器 --------------------------------------------------- */

AlgoStatus cmd_global(int argc, char *argv[], RuntimeContext *ctx) {
    (void)argc; (void)argv;

    if (ctx->global_mode) {
        console_log_info("  已在全局题库模式下");
        return ALGO_SUCCESS;
    }

    // 先保存当前数据再切换
    runtime_save_db(ctx);
    ctx->global_mode = true;
    utils_strcpy_safe(ctx->workspace_root, ctx->home_dir,
                      sizeof(ctx->workspace_root));
    runtime_refresh_path_cache(ctx);
    // 确保全局目录存在后装载全局数据库
    utils_compat_make_dir(ctx->solutions_path);
    pcol_clear(&ctx->pcol);
    storage_load(&ctx->pcol, ctx->db_path);
    console_print_accent("  已切换到全局题库\n");
    console_print_muted("  数据库: %s\n", ctx->db_path);
    return ALGO_SUCCESS;
}

AlgoStatus cmd_local(int argc, char *argv[], RuntimeContext *ctx) {
    (void)argc; (void)argv;

    if (!ctx->global_mode) {
        console_log_info("  已在本地工作区模式下");
        return ALGO_SUCCESS;
    }

    // 先保存当前数据再切换
    runtime_save_db(ctx);
    ctx->global_mode = false;
    // 用当前工作目录或初始工作区根目录恢复本地路径
    char cwd[ALGO_MAX_PATH_LEN] = {0};
    if (utils_compat_getcwd(cwd, sizeof(cwd)))
        utils_strcpy_safe(ctx->workspace_root, cwd, sizeof(ctx->workspace_root));
    runtime_refresh_path_cache(ctx);
    // 确保本地目录存在后装载本地数据库
    utils_compat_make_dir(ctx->solutions_path);
    pcol_clear(&ctx->pcol);
    storage_load(&ctx->pcol, ctx->db_path);
    console_print_primary("  已切换到本地工作区\n");
    console_print_muted("  数据库: %s\n", ctx->db_path);
    return ALGO_SUCCESS;
}

AlgoStatus cmd_save(int argc, char *argv[], RuntimeContext *ctx) {
    (void)argc; (void)argv;

    if (runtime_save_db(ctx) == ALGO_SUCCESS) {
        console_log_success("数据已成功手动落盘");
        return ALGO_SUCCESS;
    }
    console_log_error("数据保存失败");
    return ALGO_ERR_FILE;
}

AlgoStatus cmd_clear(int argc, char *argv[], RuntimeContext *ctx) {
    (void)argc; (void)argv; (void)ctx;

#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
    return ALGO_SUCCESS;
}

/* --- REPL 专属命令帮助 ----------------------------------------------------- */

void help_global(void) {
    help_view_print_usage("global", "");
    help_view_print_description("将当前 Shell 会话切换至全局题库模式");
}

void help_local(void) {
    help_view_print_usage("local", "");
    help_view_print_description("将当前 Shell 会话切换至本地工作区模式");
}

void help_save(void) {
    help_view_print_usage("save", "");
    help_view_print_description("手动将当前内存中的题库数据保存到磁盘数据库文件");
}

void help_clear(void) {
    help_view_print_usage("clear", "");
    help_view_print_description("清除当前终端屏幕显示内容");
}

/* --- REPL 命令分发 --------------------------------------------------------- */

/**
 * @brief REPL 命令分发器。
 *
 * 将输入行切分为命令和参数，查找命令表（CORE + REPL）并执行。
 *
 * 内置子逻辑：
 * - 空行静默忽略；
 * - exit / quit / q 返回 ALGO_ERR_EOF 通知主循环退出；
 * - ! 前缀将剩余内容传递给 system() 执行外部命令；
 * - -h / --help 出现在参数中任意位置则调用对应帮助函数。
 *
 * @return ALGO_ERR_EOF 表示用户请求退出，否则返回命令 handler 的执行结果。
 */
int repl_cmd_dispatch(RuntimeContext *ctx, const char *line) {
    char buf[4096];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    utils_trim_newline(buf);
    if (buf[0] == '\0') return ALGO_SUCCESS;

    /* 退出命令 */
    if (strcmp(buf, "exit") == 0 || strcmp(buf, "quit") == 0 ||
        strcmp(buf, "q") == 0)
        return ALGO_ERR_EOF;

    /* ! 前缀：执行外部命令 */
    if (buf[0] == '!') {
        int ret = system(buf + 1);
        return (ret == 0) ? ALGO_SUCCESS : ALGO_ERR_UNKNOWN;
    }

    /* 分词 */
    char *argv[64];
    int   argc = tokenize(buf, argv, 64);
    if (argc == 0) return ALGO_SUCCESS;

    /* 查找命令（先 CORE 表再 REPL 表） */
    const Command *cmd = command_find(argv[0]);
    if (!cmd) {
        console_log_error("未知命令: \"%s\"，输入 h 或 help 查看可用命令",
                          argv[0]);
        return ALGO_ERR_ARG;
    }

    /* 检查帮助选项（可在任意参数位置） */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 ||
            strcmp(argv[i], "--help") == 0) {
            if (cmd->help_fn) cmd->help_fn();
            return ALGO_SUCCESS;
        }
    }

    return cmd->handler(argc, argv, ctx);
}


/* 私有函数实现------------------------------------------------------ */

static int tokenize(char *line, char *argv[], int max_args) {
    int   argc = 0;
    char *p    = line;

    while (*p && argc < max_args) {
        // 跳过前导空白
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') break;

        if (*p == '"') {
            // 双引号包裹的参数
            p++;                     // 跳过起始引号
            argv[argc++] = p;
            while (*p && *p != '"') p++;
            if (*p == '"') *p++ = '\0';
        } else {
            argv[argc++] = p;
            while (*p && *p != ' ' && *p != '\t') p++;
            if (*p) *p++ = '\0';
        }
    }

    return argc;
}

