/**
 * @file utils_str.h
 * @brief 字符串安全操作、规范化与校验工具。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 *
 * 本模块提供字符串安全复制、slug 格式化、空白修剪与空值检测等常用字符串
 * 处理函数，所有写操作均以 '\0' 结尾并接受缓冲区大小参数以避免溢出。
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include <algorepo/core/problem_types.h>

/* --- String Functions ------------------------------------------------------ */

/**
 * @brief 安全复制字符串到目标缓冲区。
 *
 * 始终以 '\0' 结尾。若源字符串长度超出 dest_size - 1 则截断。
 *
 * @param dest      目标缓冲区。
 * @param src       源字符串（可为 NULL，此时结果为空字符串）。
 * @param dest_size 目标缓冲区大小（字节）。
 * @return
 * - ALGO_SUCCESS            复制完成且未截断。
 * - ALGO_ERR_STR_TOO_LONG   源字符串过长，已截断。
 * - ALGO_ERR_ARG            参数非法（dest 或 src 为 NULL，或 dest_size 为 0）。
 */
AlgoStatus utils_strcpy_safe(char *dest, const char *src, size_t dest_size);

/**
 * @brief 将输入字符串转换为文件友好的 slug 格式。
 *
 * 保留 UTF-8 多字节字符，ASCII 字母转为小写，空白和标点替换为连字符，
 * 合并连续分隔符。输入为 NULL 或结果为空时写入 "untitled"。
 *
 * @param input       源字符串。
 * @param output      输出缓冲区。
 * @param output_size 输出缓冲区大小（字节）。
 */
void utils_slugify(const char *input, char *output, size_t output_size);

/**
 * @brief 去除字符串首尾空白字符（原地修改）。
 *
 * @param str 待处理的字符串，为 NULL 或空字符串时无操作。
 */
void utils_trim(char *str);

/**
 * @brief 检查字符串是否为空。
 *
 * @param s 待检查的字符串指针。
 * @return true 若 s 为 NULL 或首字符为 '\0'，否则 false。
 */
bool utils_str_empty(const char *s);

/**
 * @brief 去除行尾换行符（\n 和 \r）。
 *
 * 就地修改，向前扫描直至遇到非换行字符。
 */
void utils_trim_newline(char *line);
