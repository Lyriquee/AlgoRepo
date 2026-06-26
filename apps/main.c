/**
 * @file main.c
 * @brief 应用程序主入口：CLI / TUI 前端路由与平台兼容层生命周期管理。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorepo/utils/utils_compat.h>

#include "cli/cli_run.h"
#include "tui/tui_run.h"

/* --- 私有函数声明 --------------------------------------------------------- */

static bool is_tui_mode(int argc, char *argv[]);

/* --- 应用程序主入口 ------------------------------------------------------- */

/**
 * @brief 应用程序主入口。
 *
 * 依次执行：平台兼容层初始化 -> 前端路由（CLI/TUI） -> 资源清理。
 */
int main(int argc, char *argv[]) {
    /* 平台兼容层初始化（UTF-8 控制台、VT100、宽字符命令行） */
    utils_compat_init(&argc, &argv);

    int exit_code = EXIT_SUCCESS;

    /* 前端入口路由 */
    if (is_tui_mode(argc, argv)) {
        printf("功能开发中，敬请期待\n");
    } else {
        /* 默认进入 CLI 模式 */
        exit_code = cli_run(argc, argv);
    }

    /* 平台兼容层资源释放 */
    utils_compat_cleanup();

    return exit_code;
}

/* --- 私有函数实现 --------------------------------------------------------- */

/* 检测命令行参数中是否包含 TUI 模式标志 */
static bool is_tui_mode(int argc, char *argv[]) {
    if (argc < 2) return false;

    return strcmp(argv[1], "--tui") == 0 ||
           strcmp(argv[1], "-t") == 0;
}
