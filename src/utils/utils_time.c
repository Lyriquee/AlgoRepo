/**
 * @file utils_time.c
 * @brief 时间戳获取与日期格式化工具实现。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include <algorepo/utils/utils_time.h>

#include <string.h>
#include <time.h>

#include <algorepo/utils/utils_compat.h>
#include <algorepo/utils/utils_str.h>

/* --- 公开接口实现 --------------------------------------------------------- */

/* 获取当前 Unix 时间戳（秒） */
int64_t utils_get_now_timestamp(void) {
    return (int64_t)time(NULL);
}

/* 将时间戳格式化为日期字符串，支持可选时间显示 */
void utils_format_timestamp(int64_t ts, char *buffer, size_t buffer_size,
                            bool show_time) {
    if (!buffer || buffer_size == 0) return;

    /* 非法时间戳（非正数）统一输出占位符 */
    if (ts <= 0) {
        utils_strcpy_safe(buffer, "-", buffer_size);
        return;
    }

    time_t t = (time_t)ts;
    struct tm tm_info;

    /* 跨平台安全本地时间转换，失败时回退为占位符 */
    if (utils_compat_localtime_safe(&t, &tm_info) != 0) {
        utils_strcpy_safe(buffer, "-", buffer_size);
        return;
    }

    if (show_time) {
        strftime(buffer, buffer_size, "%Y-%m-%d %H:%M", &tm_info);
    } else {
        strftime(buffer, buffer_size, "%Y-%m-%d", &tm_info);
    }
}
