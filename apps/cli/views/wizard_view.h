/**
 * @file wizard_view.h
 * @brief 交互式题目创建向导：逐步引导用户输入标题、描述、难度、掌握度、标签和笔记。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stddef.h>

#include <algorepo/runtime/runtime.h>

/**
 * @brief 交互式题目创建向导。
 *
 * 依次引导用户输入标题（必填）、描述、难度、掌握度、标签和笔记。
 * 每步均提供循环重试机制，直到输入合法或遇到 EOF。
 *
 * @return
 * - ALGO_SUCCESS  创建成功。
 * - ALGO_ERR_EOF  用户中断输入。
 */
AlgoStatus wizard_fill_problem(Problem *p);


/* --- 字段输入辅助 ----------------------------------------------------------- */

/**
 * @brief 安全复制文本字段，过长时输出错误。
 *
 * @param field_name 字段名称，用于错误提示。
 * @return
 * - ALGO_SUCCESS            复制成功。
 * - ALGO_ERR_STR_TOO_LONG   源字符串超出目标缓冲区。
 */
AlgoStatus wizard_copy_text_field(char *dest, size_t size, const char *src,
                                  const char *field_name);

/**
 * @brief 读取可选字符串字段，空输入直接跳过。
 *
 * @param prompt 提示信息。
 * @param dest   输出缓冲区。
 * @param size   缓冲区大小。
 * @return
 * - ALGO_SUCCESS            读取成功或用户跳过。
 * - ALGO_ERR_EOF            用户中断输入。
 * - ALGO_ERR_STR_TOO_LONG   输入超出缓冲区。
 */
AlgoStatus wizard_read_optional_string(const char *prompt, char *dest,
                                       size_t size);


/* --- 菜单输出 --------------------------------------------------------------- */

/**
 * @brief 打印难度选择菜单。
 */
void print_difficulty_menu(void);

/**
 * @brief 打印掌握程度选择菜单。
 */
void print_mastery_menu(void);
