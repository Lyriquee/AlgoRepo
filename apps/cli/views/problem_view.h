/**
 * @file problem_view.h
 * @brief 题目列表与详情视图：彩色格式化输出、标签渲染、源码预览。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include <algorepo/core/problem.h>
#include <algorepo/core/problem_collection.h>

/**
 * @brief 打印单行题目摘要。
 *
 * 在表格行中输出题目 ID、标题、难度、掌握度、标签和日期。
 *
 * @param p         题目对象指针。
 * @param show_time 是否在日期列显示时间（true 显示日期时间，false 仅显示日期）。
 */
void problem_view_print_row(const Problem *p, bool show_time);

/**
 * @brief 打印列表表头。
 *
 * 输出带分隔线的表格标题行（ID、标题、难度、掌握度、标签、日期）。
 *
 * @param show_time 是否显示时间列标题。
 */
void problem_view_print_header(bool show_time);

/**
 * @brief 打印完整题目列表。
 *
 * 依次输出表头、所有行和底部分隔线。集合为空时显示 empty_msg。
 *
 * @param pcol      题目集合指针。
 * @param empty_msg 集合为空时显示的消息，为 NULL 时使用默认文本。
 * @param show_time 是否显示时间列。
 */
void problem_view_print_list(const ProblemCollection *pcol,
                             const char *empty_msg, bool show_time);

/**
 * @brief 打印题目详情（键值对格式）。
 *
 * 输出 ID、标题、难度、掌握度、标签、更新时间、源码路径、描述和笔记。
 * 描述和笔记按 100 列宽度自动换行，续行保持缩进对齐。
 *
 * @param p 题目对象指针。
 */
void problem_view_print_detail(const Problem *p);

/**
 * @brief 打印一行题目概要。
 *
 * 紧凑单行格式输出 ID、标题、难度、掌握度和标签，供 show/code 等命令复用。
 *
 * @param p 题目对象指针。
 */
void problem_view_print_summary(const Problem *p);

/**
 * @brief 打印题目源码内容预览。
 *
 * 根据 head_lines 和 tail_lines 参数输出源码文件的前后若干行，
 * 支持仅显示头部、仅显示尾部或同时显示头尾（中间省略）。
 *
 * @param p          题目对象指针。
 * @param head_lines 显示源码前 N 行，0 表示不显示。
 * @param tail_lines 显示源码后 N 行，0 表示不显示。
 */
void problem_view_print_code(const Problem *p, int head_lines, int tail_lines);

/**
 * @brief 彩色打印难度标签。
 *
 * 根据难度枚举值输出对应颜色的中文名称。
 *
 * @param diff 难度枚举值。
 */
void problem_view_print_difficulty(Difficulty diff);

/**
 * @brief 彩色打印掌握度标签。
 *
 * 根据掌握度枚举值输出对应颜色的中文名称。
 *
 * @param mastery 掌握度枚举值。
 */
void problem_view_print_mastery(Mastery mastery);

/**
 * @brief 将题目标签列表格式化为逗号分隔字符串。
 *
 * 无标签时输出 "-"。
 *
 * @param p      题目对象指针。
 * @param buffer 输出缓冲区。
 * @param size   缓冲区大小。
 */
void problem_view_format_tags(const Problem *p, char *buffer, size_t size);
