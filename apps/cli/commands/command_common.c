/**
 * @file command_common.c
 * @brief 命令模块公共工具函数与解析器实现。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include "command_common.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <algorepo/console/console_ui.h>
#include <algorepo/utils/utils_compat.h>
#include <algorepo/utils/utils_str.h>
#include <algorepo/utils/utils_time.h>


/* 统一的 getopt 错误处理：区分未知选项和缺少参数 */
AlgoStatus handle_getopt_error(int optopt, const char *optstring, char *argv[], int optind) {
    if (optopt) {
        if (strchr(optstring, optopt)) {
            console_log_error("选项 '-%c' 需要一个参数值，但未找到", optopt);
        } else {
            console_log_error("未知选项: \"-%c\"", optopt);
        }
    } else {
        console_log_error("未知选项: \"%s\"", argv[optind - 1]);
    }
    return ALGO_ERR_ARG;
}


/* 解析整数参数，检查范围 [min, max] */
AlgoStatus parse_int_arg(const char *text, int min, int max, int *out) {
    if (!text || !out) return ALGO_ERR_ARG;

    errno       = 0;
    char *end   = NULL;
    long  value = strtol(text, &end, 10);

    if (errno != 0 || end == text || *end != '\0') return ALGO_ERR_ARG;
    if (value < min || value > max) return ALGO_ERR_RANGE;

    *out = (int)value;
    return ALGO_SUCCESS;
}

/* 解析题目 ID 参数（1 到 INT_MAX） */
AlgoStatus parse_id_arg(const char *text, int *out) {
    return parse_int_arg(text, 1, 2147483647, out);
}

/* 解析中文难度字符串，返回 0-4 或 -1 */
int parse_difficulty_chinese(const char *s) {
    if (!s || s[0] == '\0') return -1;

    int value = 0;
    if (parse_int_arg(s, 0, 4, &value) == ALGO_SUCCESS) return value;

    if (strcmp(s, "简单") == 0 || strcmp(s, "容易") == 0) return 1;
    if (strcmp(s, "中等") == 0) return 2;
    if (strcmp(s, "困难") == 0) return 3;
    if (strcmp(s, "极难") == 0) return 4;
    if (strcmp(s, "未知") == 0) return 0;

    return -1;
}

/* 解析中文掌握度字符串，返回 0-4 或 -1 */
int parse_mastery_chinese(const char *s) {
    if (!s || s[0] == '\0') return -1;

    int value = 0;
    if (parse_int_arg(s, 0, 4, &value) == ALGO_SUCCESS) return value;

    if (strcmp(s, "了解") == 0) return 1;
    if (strcmp(s, "熟悉") == 0) return 2;
    if (strcmp(s, "掌握") == 0) return 3;
    if (strcmp(s, "精通") == 0) return 4;
    if (strcmp(s, "未知") == 0) return 0;

    return -1;
}

/* 统一的难度解析：先尝试数字，再尝试中文 */
AlgoStatus parse_difficulty(const char *text, int *out) {
    if (!text || !out) return ALGO_ERR_ARG;
    int v = parse_difficulty_chinese(text);
    if (v < 0) return ALGO_ERR_ARG;
    *out = v;
    return ALGO_SUCCESS;
}

/* 统一的掌握度解析：先尝试数字，再尝试中文 */
AlgoStatus parse_mastery(const char *text, int *out) {
    if (!text || !out) return ALGO_ERR_ARG;
    int v = parse_mastery_chinese(text);
    if (v < 0) return ALGO_ERR_ARG;
    *out = v;
    return ALGO_SUCCESS;
}

/* 打印操作失败的状态信息 */
void print_status_error(const char *action, AlgoStatus status) {
    console_log_error("%s失败: %s", action, algo_status_to_string(status));
}

/* 大小写不敏感的字符串子串搜索 */
char *stristr(const char *h, const char *n) {
    if (!h || !n || !*n) return (char *)h;
    for (; *h; h++) {
        if (tolower((unsigned char)*h) == tolower((unsigned char)*n)) {
            bool ok = true;
            for (size_t i = 1; n[i]; i++) {
                if (!h[i] || tolower((unsigned char)h[i]) != tolower((unsigned char)n[i])) {
                    ok = false;
                    break;
                }
            }
            if (ok) return (char *)h;
        }
    }
    return NULL;
}

/* 解析逗号分隔的 ID 列表，如 "1,2,3" */
AlgoStatus parse_comma_ids(const char *s, int *ids, int *count, int max) {
    *count = 0;
    if (!s || s[0] == '\0') return ALGO_ERR_ARG;
    char buf[256];
    utils_strcpy_safe(buf, s, sizeof(buf));
    char *tok = strtok(buf, ",");
    while (tok && *count < max) {
        int id = 0;
        if (parse_id_arg(tok, &id) != ALGO_SUCCESS) return ALGO_ERR_ARG;
        ids[(*count)++] = id;
        tok = strtok(NULL, ",");
    }
    return *count > 0 ? ALGO_SUCCESS : ALGO_ERR_ARG;
}

/* 判断指定 Unix 时间戳是否属于今天 */
bool match_today(int64_t date) {
    if (date <= 0) return false;
    time_t t_now  = (time_t)utils_get_now_timestamp();
    time_t t_date = (time_t)date;
    struct tm tm_now, tm_date;
    if (utils_compat_localtime_safe(&t_now, &tm_now) != 0) return false;
    if (utils_compat_localtime_safe(&t_date, &tm_date) != 0) return false;
    return tm_now.tm_year == tm_date.tm_year &&
           tm_now.tm_mon  == tm_date.tm_mon  &&
           tm_now.tm_mday == tm_date.tm_mday;
}

/* 判断指定 Unix 时间戳是否在最近 N 天内 */
bool match_recent(int64_t date, int days) {
    if (date <= 0) return false;
    int64_t diff = utils_get_now_timestamp() - date;
    return diff >= 0 && diff <= (int64_t)days * 86400;
}
