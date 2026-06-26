/**
 * @file wizard_view.c
 * @brief 交互式题目创建向导实现。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include "wizard_view.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <algorepo/console/console_ui.h>
#include <algorepo/core/problem.h>
#include <algorepo/utils/utils_str.h>

#include "../commands/command_common.h"
#include "problem_view.h"


/* --- 字段输入辅助 ----------------------------------------------------------- */

/* 安全复制文本字段，过长时输出错误 */
AlgoStatus wizard_copy_text_field(char *dest, size_t size, const char *src,
                                  const char *field_name) {
    AlgoStatus status = utils_strcpy_safe(dest, src ? src : "", size);
    if (status != ALGO_SUCCESS) {
        console_log_error("%s过长", field_name);
    }
    return status;
}

/* 读取可选字符串字段（空输入跳过） */
AlgoStatus wizard_read_optional_string(const char *prompt, char *dest,
                                       size_t size) {
    char       buffer[ALGO_MAX_DESC_LEN] = {0};
    AlgoStatus status =
        console_input_string(prompt, buffer, sizeof(buffer));
    if (status != ALGO_SUCCESS) return status;
    if (buffer[0] == '\0') return ALGO_SUCCESS;
    return utils_strcpy_safe(dest, buffer, size);
}


/* --- 菜单输出 --------------------------------------------------------------- */

/* 打印难度选择菜单 */
void print_difficulty_menu(void) {
    console_print_primary("题目难度：\n");
    console_print_spaces(2);
    console_print_role(COLOR_ROLE_PARAM, "1) ");
    problem_view_print_difficulty(ALGO_DIFF_EASY);
    console_print_spaces(3);
    console_print_role(COLOR_ROLE_PARAM, "2) ");
    problem_view_print_difficulty(ALGO_DIFF_MEDIUM);
    console_print_spaces(3);
    console_print_role(COLOR_ROLE_PARAM, "3) ");
    problem_view_print_difficulty(ALGO_DIFF_HARD);
    console_print_spaces(3);
    console_print_role(COLOR_ROLE_PARAM, "4) ");
    problem_view_print_difficulty(ALGO_DIFF_EXTREME);
    console_print_newline();
}

/* 打印掌握程度选择菜单 */
void print_mastery_menu(void) {
    console_print_primary("掌握程度：\n");
    console_print_spaces(2);
    console_print_role(COLOR_ROLE_PARAM, "1) ");
    problem_view_print_mastery(ALGO_MASTERY_LEARNING);
    console_print_spaces(3);
    console_print_role(COLOR_ROLE_PARAM, "2) ");
    problem_view_print_mastery(ALGO_MASTERY_FAMILIAR);
    console_print_spaces(3);
    console_print_role(COLOR_ROLE_PARAM, "3) ");
    problem_view_print_mastery(ALGO_MASTERY_MASTERED);
    console_print_spaces(3);
    console_print_role(COLOR_ROLE_PARAM, "4) ");
    problem_view_print_mastery(ALGO_MASTERY_EXPERT);
    console_print_newline();
}


/* --- 枚举选择辅助 ----------------------------------------------------------- */

/** @brief 枚举解析函数签名 */
typedef AlgoStatus (*parse_enum_fn)(const char *buf, int *value);

/** @brief 菜单打印函数签名 */
typedef void (*print_menu_fn)(void);

/**
 * @brief 通用枚举选择循环：显示菜单、读取输入、解析并赋值。
 *
 * @param print_menu  打印选项菜单的回调。
 * @param parse_fn    将用户输入解析为枚举值的回调。
 * @param error_msg   解析失败时的错误提示信息。
 * @param target      指向目标枚举字段的指针（0 = 未知/跳过）。
 * @return ALGO_SUCCESS 选择完成或被用户跳过；ALGO_ERR_EOF 用户中断。
 */
static AlgoStatus wizard_select_enum(print_menu_fn print_menu,
                                      parse_enum_fn parse_fn,
                                      const char *error_msg,
                                      int *target) {
    while (*target == 0) {
        console_print_newline();
        print_menu();
        console_print_prompt("> ");

        char buf[64] = {0};
        AlgoStatus status = console_input_string("", buf, sizeof(buf));
        if (status == ALGO_ERR_EOF) return status;
        if (buf[0] == '\0' || strcmp(buf, "0") == 0) break;

        int value = 0;
        if (parse_fn(buf, &value) != ALGO_SUCCESS) {
            console_log_error("%s", error_msg);
            continue;
        }
        *target = value;
    }
    return ALGO_SUCCESS;
}


/* --- 各字段输入步骤 --------------------------------------------------------- */

/* 标题输入：循环直到非空 */
static AlgoStatus wizard_input_title(Problem *p) {
    while (p->title[0] == '\0') {
        console_print_primary("题目标题：\n");
        console_print_prompt("> ");
        AlgoStatus status =
            console_input_string("", p->title, sizeof(p->title));
        if (status == ALGO_ERR_EOF) return status;
        if (status == ALGO_ERR_STR_TOO_LONG) {
            console_log_error("标题过长(最多%d字符)",
                              ALGO_MAX_TITLE_LEN - 1);
            p->title[0] = '\0';
            continue;
        }
        utils_trim(p->title);
        if (p->title[0] == '\0') {
            console_log_warn("题目标题不能为空");
        }
    }
    return ALGO_SUCCESS;
}

/* 描述输入（可选） */
static AlgoStatus wizard_input_description(Problem *p) {
    if (p->desc[0] != '\0') return ALGO_SUCCESS;

    console_print_primary("\n题目描述 (可选): \n");
    console_print_prompt("> ");
    AlgoStatus status =
        console_input_string("", p->desc, sizeof(p->desc));
    if (status == ALGO_ERR_EOF) return status;
    return ALGO_SUCCESS;
}

/* 难度选择 */
static AlgoStatus wizard_input_difficulty(Problem *p) {
    return wizard_select_enum(print_difficulty_menu, parse_difficulty,
                               "输入无效，请输入 1-4、中文名称或直接回车",
                               (int *)&p->diff);
}

/* 掌握度选择 */
static AlgoStatus wizard_input_mastery(Problem *p) {
    return wizard_select_enum(print_mastery_menu, parse_mastery,
                               "输入无效，请输入 1-4、中文名称或直接回车",
                               (int *)&p->mastery);
}

/* 标签输入（可选，逗号分隔） */
static AlgoStatus wizard_input_tags(Problem *p) {
    while (p->tag_count == 0) {
        console_print_newline();
        console_print_primary("题目标签 (逗号分隔，可选): \n");
        console_print_prompt("> ");

        char tags_buf[512] = {0};
        AlgoStatus status =
            console_input_string("", tags_buf, sizeof(tags_buf));
        if (status == ALGO_ERR_EOF) return status;

        if (tags_buf[0] != '\0') {
            status = problem_parse_tags(p, tags_buf);
            if (status == ALGO_ERR_TAG_FULL) {
                console_log_error("标签数量不能超过 %d 个",
                                  ALGO_MAX_TAGS);
                memset(p->tags, 0, sizeof(p->tags));
                p->tag_count = 0;
                continue;
            }
            if (status != ALGO_SUCCESS) return status;
        }
        if (tags_buf[0] == '\0') break;
    }
    return ALGO_SUCCESS;
}

/* 笔记输入（可选） */
static AlgoStatus wizard_input_notes(Problem *p) {
    if (p->notes[0] != '\0') return ALGO_SUCCESS;

    console_print_newline();
    console_print_primary("题目笔记（可选）: \n");
    console_print_prompt("> ");
    AlgoStatus status =
        console_input_string("", p->notes, sizeof(p->notes));
    if (status != ALGO_SUCCESS) return status;
    return ALGO_SUCCESS;
}


/* --- 交互式向导主流程 ------------------------------------------------------- */

/* 交互式题目创建向导：依次引导用户输入标题、描述、难度、掌握度、标签和笔记 */
AlgoStatus wizard_fill_problem(Problem *p) {
    AlgoStatus status;

    status = wizard_input_title(p);
    if (status != ALGO_SUCCESS) return status;

    status = wizard_input_description(p);
    if (status != ALGO_SUCCESS) return status;

    status = wizard_input_difficulty(p);
    if (status != ALGO_SUCCESS) return status;

    status = wizard_input_mastery(p);
    if (status != ALGO_SUCCESS) return status;

    status = wizard_input_tags(p);
    if (status != ALGO_SUCCESS) return status;

    status = wizard_input_notes(p);
    if (status != ALGO_SUCCESS) return status;

    return ALGO_SUCCESS;
}
