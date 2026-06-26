/**
 * @file utils_time.h
 * @brief 时间戳获取与日期格式化工具。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 *
 * 提供跨平台的当前时间获取以及将 Unix 时间戳格式化为人类可读日期字符串的
 * 功能。格式化输出支持仅日期与日期时间两种模式。
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* --- Time Functions -------------------------------------------------------- */

/**
 * @brief 获取当前 Unix 时间戳（秒）。
 *
 * @return 自 Unix 纪元以来的秒数，以 int64_t 返回。
 */
int64_t utils_get_now_timestamp(void);

/**
 * @brief 将 Unix 时间戳格式化为日期字符串。
 *
 * 当 show_time 为 true 时输出格式为 "YYYY-MM-DD HH:MM"，
 * 否则为 "YYYY-MM-DD"。若时间戳非法或转换失败，写入 "-"。
 *
 * @param ts          待格式化的 Unix 时间戳（秒）。
 * @param buffer      输出缓冲区。
 * @param buffer_size 缓冲区大小（字节）。
 * @param show_time   是否同时输出时间部分。
 */
void utils_format_timestamp(int64_t ts, char *buffer, size_t buffer_size,
                            bool show_time);
