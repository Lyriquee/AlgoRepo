/**
 * @file utils_fs.h
 * @brief 跨平台文件系统检测、拷贝与路径处理工具。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 *
 * 本模块提供文件存在性检测、二进制文件拷贝、扩展名提取以及路径规范化与
 * 绝对路径判断等文件系统相关操作，所有函数均为跨平台实现。
 */

#pragma once

#include <stdbool.h>

#include <algorepo/core/problem_types.h>

/* --- File Operations ------------------------------------------------------- */

/**
 * @brief 检查指定路径的文件是否存在。
 *
 * @param path 文件路径。
 * @return true 若文件存在，否则 false（含 path 为 NULL 的情况）。
 */
bool utils_file_exists(const char *path);

/**
 * @brief 跨平台二进制文件拷贝（流式读写，支持大文件）。
 *
 * src 与 dest 相同时视为成功，不执行任何操作。
 *
 * @param src  源文件路径。
 * @param dest 目标文件路径。
 * @return
 * - ALGO_SUCCESS   拷贝成功。
 * - ALGO_ERR_FILE  文件打开或写入失败。
 * - ALGO_ERR_ARG   参数非法（src 或 dest 为 NULL）。
 */
AlgoStatus utils_copy_file(const char *src, const char *dest);

/**
 * @brief 获取文件路径的扩展名（含前导点号）。
 *
 * 处理无后缀文件及 ".gitignore" 类隐藏文件，二者均返回空字符串。
 *
 * @param path 文件路径。
 * @return 指向 path 内部扩展名起始位置的指针，无扩展名时返回 ""。
 */
const char *utils_get_file_ext(const char *path);

/* --- Path Utilities -------------------------------------------------------- */

/**
 * @brief 将路径中的反斜杠统一替换为正斜杠（原地修改）。
 *
 * @param path 待规范化的路径字符串，为 NULL 时无操作。
 */
void utils_path_normalize_separators(char *path);

/**
 * @brief 判断路径是否为绝对路径。
 *
 * 识别 Unix 绝对路径（以 '/' 开头）、Windows 盘符路径（如 "C:"）
 * 及 UNC 网络路径（以 "\\\\" 开头）。
 *
 * @param path 待判断的路径字符串。
 * @return true 若为绝对路径，否则 false（含 path 为 NULL 的情况）。
 */
bool utils_path_is_absolute(const char *path);
