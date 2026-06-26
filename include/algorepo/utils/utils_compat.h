/**
 * @file utils_compat.h
 * @brief 跨平台兼容层：终端初始化、安全文件 I/O 与环境检测。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 *
 * 本模块封装 Windows 与 POSIX 平台的差异，提供统一的终端初始化、UTF-8
 * 文件 I/O、目录操作、路径处理与终端检测接口。调用方无需处理平台差异，
 * 各函数内部已完成条件编译适配。
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>

#include <algorepo/core/problem_types.h>

/* --- Lifecycle ------------------------------------------------------------- */

/**
 * @brief 初始化跨平台兼容层。
 *
 * Windows 端设置 UTF-8 控制台编码、转换宽字符命令行参数并启用 VT100
 * 虚拟终端转义序列。POSIX 端无操作。应在 main() 最开头调用。
 *
 * @param argc 指向 argc 的指针（main 传入）。
 * @param argv 指向 argv 的指针（main 传入）。
 */
void utils_compat_init(int *argc, char ***argv);

/**
 * @brief 清理跨平台兼容层占用的资源。
 *
 * Windows 端释放 UTF-8 命令行参数内存。应在程序退出前调用。
 */
void utils_compat_cleanup(void);

/* --- Safe File I/O --------------------------------------------------------- */

/**
 * @brief 跨平台安全的 fopen 替代。
 *
 * Windows 端将 UTF-8 路径与模式转为宽字符后调用 _wfopen，避免中文路径乱码。
 * POSIX 端直接透传 fopen。
 *
 * @param path 文件路径（UTF-8 编码）。
 * @param mode 打开模式（如 "rb"、"wb" 等）。
 * @return 文件指针，失败返回 NULL。
 */
FILE *utils_compat_fopen(const char *path, const char *mode);

/**
 * @brief 跨平台安全的 localtime 替代。
 *
 * Windows 端封装 localtime_s，POSIX 端封装 localtime_r。
 *
 * @param timer  输入时间指针。
 * @param out_tm 输出 tm 结构体指针。
 * @return 0 成功，-1 失败（含参数为 NULL 的情况）。
 */
int utils_compat_localtime_safe(const time_t *timer, struct tm *out_tm);

/* --- Filesystem & Path ----------------------------------------------------- */

/**
 * @brief 检测给定路径是否存在（文件或目录）。
 *
 * @param path 待检测的路径。
 * @return true 若存在，否则 false（含 path 为 NULL 或空串的情况）。
 */
bool utils_compat_path_exists(const char *path);

/**
 * @brief 跨平台创建目录。
 *
 * 仅创建单级目录，上层目录需已存在。目录已存在（EEXIST）视为成功。
 *
 * @param path 目录路径。
 * @return true 创建成功或目录已存在，false 创建失败。
 */
bool utils_compat_make_dir(const char *path);

/**
 * @brief 跨平台获取当前工作目录。
 *
 * Windows 端通过 _wgetcwd 获取宽字符路径后转为 UTF-8。
 *
 * @param buffer 输出缓冲区。
 * @param size   缓冲区大小（字节）。
 * @return 成功返回 buffer，失败返回 NULL。
 */
char *utils_compat_getcwd(char *buffer, size_t size);

/**
 * @brief 返回路径中最后一个斜杠（'/' 或 '\\'）的指针。
 *
 * 同时匹配 Unix 与 Windows 分隔符，可用于提取文件名。
 *
 * @param path 路径字符串。
 * @return 指向最后一个目录分隔符的指针，未找到或 path 为 NULL 时返回 NULL。
 */
char *utils_compat_get_last_slash(char *path);

/**
 * @brief 获取当前用户的家目录路径。
 *
 * 优先读取 HOME 环境变量，Windows 端回退到 USERPROFILE。
 *
 * @param buf  输出缓冲区。
 * @param size 缓冲区大小（字节）。
 * @return true 获取成功，false 失败（含 buf 为 NULL 或 size 为 0 的情况）。
 */
bool utils_compat_get_user_home(char *buf, size_t size);

/**
 * @brief 跨平台安全删除文件。
 *
 * Windows 端通过 DeleteFileW 删除，POSIX 端调用 remove。
 *
 * @param path 待删除的文件路径。
 * @return
 * - ALGO_SUCCESS   删除成功。
 * - ALGO_ERR_FILE  删除失败。
 * - ALGO_ERR_ARG   path 为 NULL 或空串。
 */
AlgoStatus utils_compat_remove_file(const char *path);

/* --- Terminal Detection ---------------------------------------------------- */

/**
 * @brief 检测标准输出是否连接到终端。
 *
 * 用于判断是否启用彩色/格式化输出（管道或重定向时禁用）。
 *
 * @return true 若 stdout 是交互式终端，否则 false。
 */
bool utils_compat_isatty_stdout(void);
