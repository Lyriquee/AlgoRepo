/**
 * @file command_edit.c
 * @brief 编辑题目命令实现。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include "command_edit.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorepo/console/console_ui.h>
#include <algorepo/service/problem_service.h>
#include <algorepo/utils/utils_time.h>
#include <algorepo/utils/utils_str.h>
#include <algorepo/utils/utils_fs.h>

#include "command.h"
#include "command_common.h"
#include "views/problem_view.h"
#include "views/wizard_view.h"


/* --- 私有函数声明 --------------------------------------------------------- */

/* 灵活查找位置参数中的题目 ID，返回 -1 未找到，-2 找到多个 */
static int find_positional_id(int argc, char *argv[], int start);


/* --- 公开接口实现 --------------------------------------------------------- */

/* 编辑命令处理：修改题目内容或元数据 */
AlgoStatus cmd_edit(int argc, char *argv[], RuntimeContext *ctx) {
    bool interactive = false;
    int  new_mastery = -1, new_diff = -1;
    char new_title[ALGO_MAX_TITLE_LEN] = {0};
    char new_tags[512] = {0};
    char new_notes[ALGO_MAX_NOTES_LEN] = {0};

    static const struct option long_opts[] = {
        {"interactive", no_argument,       0, 'i'},
        {"mastery",     required_argument, 0, 'm'},
        {"title",       required_argument, 0, 't'},
        {"tags",        required_argument, 0, 'g'},
        {"notes",       required_argument, 0, 'n'},
        {"diff",        required_argument, 0, 'd'},
        {0, 0, 0, 0}};

    optind = 1; opterr = 0;
    int c;
    while ((c = getopt_long(argc, argv, "im:t:g:n:d:", long_opts, NULL)) != -1) {
        switch (c) {
            case 'i': interactive = true; break;
            case 'm': if (parse_mastery(optarg, &new_mastery) != ALGO_SUCCESS) { console_log_error("无效掌握度"); return ALGO_ERR_ARG; } break;
            case 't': wizard_copy_text_field(new_title, sizeof(new_title), optarg, "标题"); break;
            case 'g': wizard_copy_text_field(new_tags, sizeof(new_tags), optarg, "标签"); break;
            case 'n': wizard_copy_text_field(new_notes, sizeof(new_notes), optarg, "笔记"); break;
            case 'd': if (parse_difficulty(optarg, &new_diff) != ALGO_SUCCESS) { console_log_error("无效难度"); return ALGO_ERR_ARG; } break;
            case '?': return handle_getopt_error(optopt, "im:t:g:n:d:", argv, optind);
            default: return ALGO_ERR_ARG;
        }
    }

    int id = find_positional_id(argc, argv, optind);
    if (id == -1) { console_log_error("缺少题目 ID"); return ALGO_ERR_REQUIRED; }
    if (id == -2) { console_log_error("发现多个 ID"); return ALGO_ERR_ARG; }

    Problem *p = pcol_get_by_id(&ctx->pcol, id);
    if (!p) { console_log_error("题目不存在: #%d", id); return ALGO_ERR_NOT_FOUND; }

    bool changed = false;

    if (new_title[0]) {
        AlgoStatus st = service_update_title(ctx, id, new_title);
        if (st != ALGO_SUCCESS) { print_status_error("修改标题", st); return st; }
        changed = true;
    }
    if (new_mastery >= 0) { service_update_mastery(ctx, id, (Mastery)new_mastery); changed = true; }
    if (new_diff >= 0) { service_update_difficulty(ctx, id, (Difficulty)new_diff); changed = true; }
    if (new_notes[0]) { service_update_notes(ctx, id, new_notes); changed = true; }
    if (new_tags[0]) {
        memset(p->tags, 0, sizeof(p->tags)); p->tag_count = 0;
        AlgoStatus st = problem_parse_tags(p, new_tags);
        if (st != ALGO_SUCCESS) { console_log_error("标签设置失败"); return st; }
        changed = true;
    }

    if (interactive) {
        problem_view_print_detail(p);
        console_log_info("直接回车保留原值");
        char buf[ALGO_MAX_DESC_LEN] = {0};
        char buf2[64];

        // 标题
        console_print_newline();
        console_print_primary("题目标题：\n");
        console_print_prompt("> ");
        if (console_input_string("", buf, sizeof(buf)) != ALGO_SUCCESS) return ALGO_ERR_EOF;
        if (buf[0]) {
            AlgoStatus st2 = service_update_title(ctx, id, buf);
            if (st2 != ALGO_SUCCESS) { print_status_error("修改标题", st2); return st2; }
            changed = true;
        }

        // 难度
        console_print_newline();
        print_difficulty_menu();
        console_print_prompt("> ");
        if (console_input_string("", buf2, sizeof(buf2)) == ALGO_SUCCESS && buf2[0]) {
            int v; if (parse_difficulty(buf2, &v) == ALGO_SUCCESS) { service_update_difficulty(ctx, id, (Difficulty)v); changed = true; }
        }

        // 掌握度
        console_print_newline();
        print_mastery_menu();
        console_print_prompt("> ");
        if (console_input_string("", buf2, sizeof(buf2)) == ALGO_SUCCESS && buf2[0]) {
            int v; if (parse_mastery(buf2, &v) == ALGO_SUCCESS) { service_update_mastery(ctx, id, (Mastery)v); changed = true; }
        }

        // 标签
        console_print_newline();
        console_print_primary("题目标签 (逗号分隔): \n");
        console_print_prompt("> ");
        if (console_input_string("", buf, sizeof(buf)) != ALGO_SUCCESS) return ALGO_ERR_EOF;
        if (buf[0]) { memset(p->tags, 0, sizeof(p->tags)); p->tag_count = 0; problem_parse_tags(p, buf); changed = true; }

        // 笔记
        console_print_newline();
        console_print_primary("题目笔记：\n");
        console_print_prompt("> ");
        if (console_input_string("", buf, sizeof(buf)) != ALGO_SUCCESS) return ALGO_ERR_EOF;
        if (buf[0]) { service_update_notes(ctx, id, buf); changed = true; }
    }

    if (!changed && !interactive) {
        // 默认：用编辑器打开源码
        if (p->solution_path[0] && utils_file_exists(p->solution_path) && ctx->config.editor[0]) {
            char cmd[ALGO_MAX_PATH_LEN * 2 + 64];
            snprintf(cmd, sizeof(cmd), "%s \"%s\"", ctx->config.editor, p->solution_path);
            system(cmd);
        } else {
            console_log_info("使用 -i 交互编辑元数据");
        }
    }

    if (changed) {
        p->created_at = utils_get_now_timestamp();
        console_log_success("题目 #%d 已更新", id);
    }
    return ALGO_SUCCESS;
}

/* 编辑命令帮助信息 */
void help_edit(void) {
    help_view_print_usage("edit", "<id> [options]");
    help_view_print_description("编辑题目内容或元数据");
    help_view_print_help_section("选项:");
    help_view_print_help_option("-h, --help",       NULL,      "显示此帮助信息");
    help_view_print_help_option("-i, --interactive", NULL,     "进入交互模式修改元数据");
    help_view_print_help_option("-m, --mastery",    "<0-4>",  "修改掌握度");
    help_view_print_help_option("-t, --title",      "<str>",  "修改标题");
    help_view_print_help_option("-g, --tags",       "<a,b>",  "覆盖标签");
    help_view_print_help_option("-n, --notes",      "<str>",  "修改笔记");
    help_view_print_help_option("-d, --diff",       "<0-4>",  "修改难度\n");
    help_view_print_help_section("示例:");
    help_view_print_help_example("algo edit 1 -m 3");
    help_view_print_help_example("algo edit 1 -i");
}


/* --- 私有函数实现 --------------------------------------------------------- */

/* 灵活查找位置参数中的题目 ID，返回 -1 未找到，-2 找到多个 */
static int find_positional_id(int argc, char *argv[], int start) {
    int id = -1;
    for (int i = start; i < argc; i++) {
        int candidate;
        if (parse_id_arg(argv[i], &candidate) == ALGO_SUCCESS) {
            if (id == -1) id = candidate;
            else return -2;
        }
    }
    return id;
}
