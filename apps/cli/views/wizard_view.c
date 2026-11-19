/**
 * @file wizard_view.c
 * @brief 交互式题目创建向导实现。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include "wizard_view.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorepo/console/console_ui.h>
#include <algorepo/core/problem.h>
#include <algorepo/utils/utils_compat.h>
#include <algorepo/utils/utils_fs.h>
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
        console_print_prompt("❯ ");

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


/* --- 编辑器输入辅助 --------------------------------------------------------- */

/* 构建临时文件路径到 .algorepo 目录下 */
static void wizard_make_temp_path(const RuntimeContext *ctx, char *buf, size_t size) {
    const char *root = ctx->global_mode ? ctx->home_dir : ctx->workspace_root;
    size_t root_max = size > 24 ? size - 24 : 0;
    size_t rlen = strlen(root);
    if (rlen > root_max) rlen = root_max;
    int w = snprintf(buf, size, "%.*s/%s/.edit_temp", (int)rlen, root, ALGO_META_DIR);
    if (w < 0 || (size_t)w >= size) {
        snprintf(buf, size, "%.*s/.edit_temp", (int)rlen, root);
    }
}

/* 通过编辑器编辑文本字段。
 * 创建临时文件、写入初始内容、拉起编辑器、读回内容、校验长度、删除临时文件。
 * 返回 ALGO_SUCCESS 成功、ALGO_ERR_STR_TOO_LONG 超长、其他为错误。 */
static AlgoStatus wizard_edit_in_editor(const char *editor_cmd,
                                         const char *temp_path,
                                         char *dest, size_t dest_size,
                                         const char *field_name,
                                         size_t max_len) {
    /* 写入初始内容（如果有）到临时文件 */
    FILE *fp = utils_compat_fopen(temp_path, "w");
    if (!fp) return ALGO_ERR_FILE;
    if (dest[0]) fputs(dest, fp);
    fclose(fp);

    /* 构建并执行编辑器命令 */
    char cmd[ALGO_MAX_PATH_LEN * 2 + 64];
    int  w = snprintf(cmd, sizeof(cmd), "%s \"%s\"", editor_cmd, temp_path);
    if (w < 0 || (size_t)w >= sizeof(cmd)) {
        utils_compat_remove_file(temp_path);
        return ALGO_ERR_RANGE;
    }

    int ret = system(cmd);
    if (ret != 0) {
        utils_compat_remove_file(temp_path);
        return ALGO_ERR_UNKNOWN;
    }

    /* 读回编辑后的内容 */
    fp = utils_compat_fopen(temp_path, "r");
    if (!fp) {
        utils_compat_remove_file(temp_path);
        return ALGO_ERR_FILE;
    }

    size_t total = 0;
    char   line[4096];
    dest[0] = '\0';
    while (fgets(line, sizeof(line), fp) && total < dest_size - 1) {
        size_t len = strlen(line);
        if (total + len >= dest_size) {
            memcpy(dest + total, line, dest_size - 1 - total);
            total = dest_size - 1;
            break;
        }
        memcpy(dest + total, line, len);
        total += len;
    }
    dest[total] = '\0';
    fclose(fp);

    /* 删除临时文件 */
    utils_compat_remove_file(temp_path);

    /* 修剪尾部空白/换行 */
    utils_trim(dest);

    /* 校验长度 */
    if (total >= dest_size - 1 || strlen(dest) > max_len) {
        console_log_error("%s过长 (最多 %lu 字符)，请重新输入",
                          field_name, (unsigned long)max_len);
        dest[0] = '\0';
        return ALGO_ERR_STR_TOO_LONG;
    }

    return ALGO_SUCCESS;
}

/* --- 通用长文本字段输入 ------------------------------------------------------- */

/* 通用长文本字段输入：fgets 先行内输入，输入 ":e" 拉起编辑器。
 * 直接回车留空返回成功（dest[0]=='\0'），其余内容直接作为字段值。
 * 超长或编辑器失败时回到 fgets 循环重试。 */
AlgoStatus wizard_input_text_field(const char *prompt, char *dest,
                                    size_t dest_size, size_t max_len,
                                    RuntimeContext *ctx) {
    char temp_path[ALGO_MAX_PATH_LEN];
    wizard_make_temp_path(ctx, temp_path, sizeof(temp_path));

    while (1) {
        console_print_primary("\n%s (输入 :e 拉起编辑器):\n", prompt);
        console_print_prompt("❯ ");

        char line[4096] = {0};
        AlgoStatus status = console_input_string("", line, sizeof(line));
        if (status == ALGO_ERR_EOF) return status;
        if (status == ALGO_ERR_STR_TOO_LONG) {
            console_log_error("输入过长，请重新输入");
            continue;
        }

        /* 直接回车 → 留空 */
        if (line[0] == '\0') {
            dest[0] = '\0';
            return ALGO_SUCCESS;
        }

        /* :e → 拉起编辑器 */
        if (strcmp(line, ":e") == 0) {
            if (ctx->config.editor[0] == '\0') {
                console_log_error("未配置编辑器，请通过 \"algo config set editor <命令>\" 设置");
                continue;
            }
            status = wizard_edit_in_editor(ctx->config.editor, temp_path,
                                           dest, dest_size, prompt, max_len);
            if (status == ALGO_SUCCESS) return ALGO_SUCCESS;
            /* 超长或编辑器失败 → 回到 fgets 重试 */
            continue;
        }

        /* 普通内容 → 校验长度并保存 */
        utils_trim(line);
        size_t len = strlen(line);
        if (len > max_len) {
            console_log_error("%s过长 (最多 %lu 字符)，请重新输入",
                              prompt, (unsigned long)max_len);
            continue;
        }
        utils_strcpy_safe(dest, line, dest_size);
        return ALGO_SUCCESS;
    }
}

/* --- 各字段输入步骤 --------------------------------------------------------- */

/* 标题输入：循环直到非空且不超长 */
static AlgoStatus wizard_input_title(Problem *p) {
    while (p->title[0] == '\0') {
        console_print_primary("题目标题：\n");
        console_print_prompt("❯ ");
        AlgoStatus status =
            console_input_string("", p->title, sizeof(p->title));
        if (status == ALGO_ERR_EOF) return status;
        if (status == ALGO_ERR_STR_TOO_LONG) {
            console_log_error("标题过长 (最多 %d 字符), 请重新输入",
                              ALGO_MAX_TITLE_LEN - 2);
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

/* 描述输入（可选），fgets 先行，:e 拉起编辑器 */
static AlgoStatus wizard_input_description(Problem *p, RuntimeContext *ctx) {
    if (p->desc[0] != '\0') return ALGO_SUCCESS;
    return wizard_input_text_field("题目描述", p->desc,
                                    sizeof(p->desc), ALGO_MAX_DESC_LEN - 2, ctx);
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

/* 标签输入（可选，逗号分隔），超长时提示并重新输入 */
static AlgoStatus wizard_input_tags(Problem *p) {
    while (p->tag_count == 0) {
        console_print_newline();
        console_print_primary("题目标签 (逗号分隔，可选): \n");
        console_print_prompt("❯ ");

        char tags_buf[512] = {0};
        AlgoStatus status =
            console_input_string("", tags_buf, sizeof(tags_buf));
        if (status == ALGO_ERR_EOF) return status;
        if (status == ALGO_ERR_STR_TOO_LONG) {
            console_log_error("标签输入过长，请重新输入");
            continue;
        }

        if (tags_buf[0] != '\0') {
            status = problem_parse_tags(p, tags_buf);
            if (status == ALGO_ERR_TAG_FULL) {
                console_log_error("标签数量不能超过 %d 个，请重新输入",
                                  ALGO_MAX_TAGS);
                memset(p->tags, 0, sizeof(p->tags));
                p->tag_count = 0;
                continue;
            }
            if (status == ALGO_ERR_STR_TOO_LONG) {
                console_log_error("单个标签过长 (最多 %d 字符)，请重新输入",
                                  ALGO_MAX_TAG_LEN - 2);
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

/* 笔记输入（可选），fgets 先行，:e 拉起编辑器 */
static AlgoStatus wizard_input_notes(Problem *p, RuntimeContext *ctx) {
    if (p->notes[0] != '\0') return ALGO_SUCCESS;
    return wizard_input_text_field("题目笔记", p->notes,
                                    sizeof(p->notes), ALGO_MAX_NOTES_LEN - 2, ctx);
}


/* --- 交互式向导主流程 ------------------------------------------------------- */

/* 交互式题目创建向导：依次引导用户输入标题、描述、难度、掌握度、标签和笔记 */
AlgoStatus wizard_fill_problem(Problem *p, RuntimeContext *ctx) {
    AlgoStatus status;

    status = wizard_input_title(p);
    if (status != ALGO_SUCCESS) return status;

    status = wizard_input_description(p, ctx);
    if (status != ALGO_SUCCESS) return status;

    status = wizard_input_difficulty(p);
    if (status != ALGO_SUCCESS) return status;

    status = wizard_input_mastery(p);
    if (status != ALGO_SUCCESS) return status;

    status = wizard_input_tags(p);
    if (status != ALGO_SUCCESS) return status;

    status = wizard_input_notes(p, ctx);
    if (status != ALGO_SUCCESS) return status;

    return ALGO_SUCCESS;
}
