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
 * 描述和笔记支持 :e 拉起编辑器输入，编辑器不可用时回退到行内输入。
 * 每步均提供循环重试机制，直到输入合法或遇到 EOF。
 *
 * @param p   待填充的题目对象。
 * @param ctx 运行时上下文（用于读取编辑器配置及工作区路径）。
 * @return
 * - ALGO_SUCCESS  创建成功。
 * - ALGO_ERR_EOF  用户中断输入。
 */
AlgoStatus wizard_fill_problem(Problem *p, RuntimeContext *ctx);


/* --- 字段输入辅助 ----------------------------------------------------------- */

/**
 * @brief 安全复制文本字段，过长时输出错误。
 *
 * @param dest       目标缓冲区。
 * @param size       缓冲区大小。
 * @param src        源字符串。
 * @param field_name 字段名称，用于错误提示。
 * @return
 * - ALGO_SUCCESS            复制成功。
 * - ALGO_ERR_STR_TOO_LONG   源字符串超出目标缓冲区。
 */
AlgoStatus wizard_copy_text_field(char *dest, size_t size, const char *src,
                                  const char *field_name);

/**
 * @brief 通用长文本字段输入：先 fgets 行内输入，输入 ":e" 拉起编辑器。
 *
 * 直接回车留空（返回 ALGO_SUCCESS 且 dest[0] == '\0'），
 * 输入 ":e" 则打开编辑器编辑临时文件，编辑器失败或内容超长时回到 fgets 重试，
 * 其余内容直接作为字段值，超长时报错并要求重新输入。
 *
 * @param prompt    提示语（如 "题目描述"），自动追加 " (输入 :e 拉起编辑器):"。
 * @param dest      目标缓冲区（可含初值，作为编辑器初始内容）。
 * @param dest_size 缓冲区大小。
 * @param max_len   最大允许字符数。
 * @param ctx       运行时上下文（编辑器命令与临时文件路径）。
 * @return
 * - ALGO_SUCCESS            输入成功（dest[0] 为空表示用户跳过）。
 * - ALGO_ERR_EOF            用户中断。
 */
AlgoStatus wizard_input_text_field(const char *prompt, char *dest,
                                    size_t dest_size, size_t max_len,
                                    RuntimeContext *ctx);


/* --- 菜单输出 --------------------------------------------------------------- */

/**
 * @brief 打印难度选择菜单。
 */
void print_difficulty_menu(void);

/**
 * @brief 打印掌握程度选择菜单。
 */
void print_mastery_menu(void);
