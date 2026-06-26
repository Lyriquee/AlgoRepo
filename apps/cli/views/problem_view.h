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

/* 打印单行题目摘要 */
void problem_view_print_row(const Problem *p, bool show_time);
/* 打印列表表头 */
void problem_view_print_header(bool show_time);
/* 打印完整题目列表 */
void problem_view_print_list(const ProblemCollection *pcol,
                             const char *empty_msg, bool show_time);

/* 打印题目详情（键值对格式） */
void problem_view_print_detail(const Problem *p);
/* 打印一行题目概要，供 show/code 等命令复用 */
void problem_view_print_summary(const Problem *p);
/* 打印题目详情（全格式，含星级和分隔线） */
void problem_view_print_detail_all(const Problem *p);
/* 打印题目源码内容预览 */
void problem_view_print_code(const Problem *p, int head_lines, int tail_lines);

/* 彩色打印难度标签 */
void problem_view_print_difficulty(Difficulty diff);
/* 彩色打印掌握度标签 */
void problem_view_print_mastery(Mastery mastery);
/* 将题目标签列表格式化为逗号分隔字符串 */
void problem_view_format_tags(const Problem *p, char *buffer, size_t size);
