/**
 * @file command_common.h
 * @brief 命令模块公共工具函数与解析器声明。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <algorepo/runtime/runtime.h>
#include "views/help_view.h"

/**
 * @brief 统一的 getopt 错误处理，区分未知选项与缺少参数。
 *
 * 根据 getopt 全局状态输出对应的错误信息，并返回统一的错误码。
 *
 * @return
 * - ALGO_ERR_ARG  选项错误或参数缺失。
 */
AlgoStatus handle_getopt_error(int optopt, const char *optstring, char *argv[], int optind);

/**
 * @brief 解析整数参数并校验范围 [min, max]。
 *
 * @param out 输出解析后的整数值。
 *
 * @return
 * - ALGO_SUCCESS    解析成功。
 * - ALGO_ERR_ARG    输入无效或格式错误。
 * - ALGO_ERR_RANGE  超出指定范围。
 */
AlgoStatus parse_int_arg(const char *text, int min, int max, int *out);

/**
 * @brief 解析题目 ID 参数，范围为 1 到 INT_MAX。
 *
 * @param out 输出解析后的 ID 值。
 *
 * @return
 * - ALGO_SUCCESS    解析成功。
 * - ALGO_ERR_ARG    输入无效或格式错误。
 * - ALGO_ERR_RANGE  超出范围。
 */
AlgoStatus parse_id_arg(const char *text, int *out);

/**
 * @brief 将中文字符串解析为难度值。
 *
 * 支持中文名称（简单、容易、中等、困难、极难、未知）和数字 0-4。
 *
 * @return 解析后的难度值 0-4，无法识别时返回 -1。
 */
int parse_difficulty_chinese(const char *s);

/**
 * @brief 将中文字符串解析为掌握度值。
 *
 * 支持中文名称（了解、熟悉、掌握、精通、未知）和数字 0-4。
 *
 * @return 解析后的掌握度值 0-4，无法识别时返回 -1。
 */
int parse_mastery_chinese(const char *s);

/**
 * @brief 统一的难度解析入口：先尝试数字，再尝试中文。
 *
 * @param out 输出解析后的难度值。
 *
 * @return
 * - ALGO_SUCCESS  解析成功。
 * - ALGO_ERR_ARG  无法识别输入。
 */
AlgoStatus parse_difficulty(const char *text, int *out);

/**
 * @brief 统一的掌握度解析入口：先尝试数字，再尝试中文。
 *
 * @param out 输出解析后的掌握度值。
 *
 * @return
 * - ALGO_SUCCESS  解析成功。
 * - ALGO_ERR_ARG  无法识别输入。
 */
AlgoStatus parse_mastery(const char *text, int *out);

/**
 * @brief 打印操作失败时的状态信息。
 */
void print_status_error(const char *action, AlgoStatus status);

/**
 * @brief 解析逗号分隔的 ID 列表，如 "1,2,3"。
 *
 * @param s     输入的逗号分隔字符串。
 * @param ids   输出的 ID 数组。
 * @param count 输出解析到的 ID 数量。
 * @param max   ids 数组的最大容量。
 * @return
 * - ALGO_SUCCESS  解析成功，至少获得一个 ID。
 * - ALGO_ERR_ARG  输入为空或所有项均无法解析。
 */
AlgoStatus parse_comma_ids(const char *s, int *ids, int *count, int max);

/**
 * @brief 大小写不敏感的字符串子串搜索。
 *
 * @param h 被搜索的字符串（haystack）。
 * @param n 要查找的子串（needle）。
 * @return 指向 haystack 中首次匹配位置的指针，未找到返回 NULL。
 *         若 needle 为空则直接返回 haystack。
 */
char *stristr(const char *h, const char *n);

/**
 * @brief 判断指定 Unix 时间戳是否属于今天。
 *
 * @param date 待判断的时间戳。
 * @return true 若时间戳对应的日期与当前系统日期相同。
 */
bool match_today(int64_t date);

/**
 * @brief 判断指定 Unix 时间戳是否在最近 N 天内。
 *
 * @param date 待判断的时间戳。
 * @param days 天数阈值。
 * @return true 若时间戳距今不超过 days 天且非负。
 */
bool match_recent(int64_t date, int days);
