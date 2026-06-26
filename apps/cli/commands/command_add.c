/**
 * @file command_add.c
 * @brief 添加题目命令实现。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include "command_add.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include <algorepo/console/console_ui.h>
#include <algorepo/service/problem_service.h>
#include <algorepo/utils/utils_compat.h>
#include <algorepo/utils/utils_time.h>
#include <algorepo/utils/utils_str.h>
#include <algorepo/utils/utils_fs.h>

#include "command.h"
#include "command_common.h"
#include "views/wizard_view.h"

/* 添加新题目到本地题库 */
AlgoStatus cmd_add(int argc, char *argv[], RuntimeContext *ctx) {
    Problem p;
    problem_init(&p);

    char source_file[ALGO_MAX_PATH_LEN] = {0};
    char lang[16]                       = {0};
    bool batch_mode                     = false;
    bool create_mode                    = false;

    static const struct option long_opts[] = {
        {"title",   required_argument, 0, 't'},
        {"desc",    required_argument, 0, 'D'},
        {"diff",    required_argument, 0, 'd'},
        {"mastery", required_argument, 0, 'm'},
        {"tags",    required_argument, 0, 'g'},
        {"notes",   required_argument, 0, 'n'},
        {"create",  no_argument,       0, 'c'},
        {"lang",    required_argument, 0, 'l'},
        {"yes",     no_argument,       0, 'y'},
        {0, 0, 0, 0}};

    optind = 1; opterr = 0;

    int c;
    while ((c = getopt_long(argc, argv, "t:D:d:m:g:n:cyl:", long_opts, NULL)) != -1) {
        AlgoStatus status = ALGO_SUCCESS;
        int        value  = 0;
        switch (c) {
            case 't':
                status = wizard_copy_text_field(p.title, sizeof(p.title), optarg, "标题");
                break;
            case 'D':
                status = wizard_copy_text_field(p.desc, sizeof(p.desc), optarg, "描述");
                break;
            case 'd':
                status = parse_difficulty(optarg, &value);
                if (status == ALGO_SUCCESS) p.diff = (Difficulty)value;
                else console_log_error("无效难度: %s，请使用 0-4 或中文名称", optarg);
                break;
            case 'm':
                status = parse_mastery(optarg, &value);
                if (status == ALGO_SUCCESS) p.mastery = (Mastery)value;
                else console_log_error("无效掌握度: %s，请使用 0-4 或中文名称", optarg);
                break;
            case 'g':
                status = problem_parse_tags(&p, optarg);
                if (status == ALGO_ERR_TAG_FULL) {
                    console_log_error("标签数量不能超过 %d 个", ALGO_MAX_TAGS);
                }
                break;
            case 'n':
                status = wizard_copy_text_field(p.notes, sizeof(p.notes), optarg, "笔记");
                break;
            case 'c':
                create_mode = true;
                break;
            case 'l':
                utils_strcpy_safe(lang, optarg, sizeof(lang));
                break;
            case 'y':
                batch_mode = true;
                break;
            case '?':
                return handle_getopt_error(optopt, "tDdmg:ncyl", argv, optind);
            default:
                return ALGO_ERR_ARG;
        }
        if (status != ALGO_SUCCESS && c != 'd' && c != 'm') return status;
    }

    // 位置参数：源文件路径
    if (optind < argc) {
        AlgoStatus status = utils_strcpy_safe(source_file, argv[optind], sizeof(source_file));
        if (status != ALGO_SUCCESS) return status;
        optind++;
    }

    if (optind < argc) {
        console_log_error("多余参数: \"%s\"", argv[optind]);
        return ALGO_ERR_ARG;
    }

    if (source_file[0] == '\0' && !create_mode) {
        console_log_error("缺少源文件路径; 使用 -c/--create 可无需文件自动创建");
        return ALGO_ERR_REQUIRED;
    }

    // 源文件存在性检查
    if (source_file[0] != '\0' && !utils_file_exists(source_file)) {
        console_log_error("源文件不存在: %s", source_file);
        return ALGO_ERR_NOT_FOUND;
    }

    // 非交互模式必须提供标题
    if (batch_mode && p.title[0] == '\0') {
        console_log_error("非交互模式必须提供 --title");
        return ALGO_ERR_REQUIRED;
    }

    // 交互模式
    if (!batch_mode) {
        AlgoStatus status = wizard_fill_problem(&p);
        if (status != ALGO_SUCCESS) return status;
    }

    // 预先设置时间戳，供 -c 文件头使用
    p.created_at = utils_get_now_timestamp();

    // -c 模式：自动生成下一个文件路径
    if (create_mode && source_file[0] == '\0') {
        const char *solutions_dir = ctx->solutions_path;

        // 确定文件后缀：-l 选项 > default_language 配置项
        const char *ext = lang[0] ? lang : ctx->config.default_language;

        char slug[128] = {0};
        utils_slugify(p.title, slug, sizeof(slug));
        int w2 = snprintf(source_file, sizeof(source_file), "%s/%03d_%s.%s", solutions_dir, ctx->pcol.next_id, slug, ext);
        if (w2 < 0 || (size_t)w2 >= sizeof(source_file)) {
            console_log_error("生成的源文件路径过长");
            return ALGO_ERR_RANGE;
        }

        FILE *fp = utils_compat_fopen(source_file, "w");
        if (fp) {
            char time_buf[32];
            utils_format_timestamp(p.created_at, time_buf, sizeof(time_buf), true);
            fprintf(fp, "/* %s */\n/* 创建时间: %s */\n", p.title, time_buf);
            fclose(fp);
            console_log_info("已创建: %s", source_file);
        } else {
            console_log_error("无法创建文件: %s", source_file);
            return ALGO_ERR_FILE;
        }

        // 提示是否用编辑器打开（仅在非批处理模式下）
        if (!batch_mode) {
            if (ctx->config.editor[0] != '\0') {
                bool open_editor = false;
                AlgoStatus cf = console_confirm("是否使用编辑器打开文件?", &open_editor);
                if (cf == ALGO_SUCCESS && open_editor) {
                    char cmd[ALGO_MAX_PATH_LEN * 2 + 64];
                    snprintf(cmd, sizeof(cmd), "%s \"%s\"", ctx->config.editor, source_file);
                    int ret = system(cmd);
                    if (ret != 0) {
                        console_log_warn("编辑器返回异常状态: %d", ret);
                    }
                }
            } else {
                console_log_warn("未配置编辑器，请通过 \"algo config set editor <命令>\" 设置");
            }
        }
    }

    // 调用业务层
    AlgoStatus status = service_add_problem(ctx, &p, source_file);
    if (status != ALGO_SUCCESS) {
        print_status_error("添加题目", status);
        return status;
    }

    console_log_success("题目添加成功: #%d %s", p.id, p.title);
    return ALGO_SUCCESS;
}

/* 显示 add 命令帮助信息 */
void help_add(void) {
    help_view_print_usage("add", "[options] <source-file>");
    help_view_print_description("添加新题目到本地题库, 支持通过命令行选项直接指定, 或省略参数以进入交互式向导补全");
    help_view_print_help_section("选项:");
    help_view_print_help_option("-h, --help",           NULL,      "显示此帮助信息");
    help_view_print_help_option("-t, --title",          "<str>",   "题目标题，必填或交互输入");
    help_view_print_help_option("-D, --desc",           "<str>",   "题目描述");
    help_view_print_help_option("-d, --diff",           "<0-4|中文>", "  难度：0未知 1简单 2中等 3困难 4极难(支持中文)");
    help_view_print_help_option("-m, --mastery",        "<0-4|中文>", "  掌握度：0未设置 1了解 2熟悉 3掌握 4精通(支持中文)");
    help_view_print_help_option("-g, --tags",           "<a,b>",   "标签，最多 8 个");
    help_view_print_help_option("-n, --notes",          "<str>",   "笔记");
    help_view_print_help_option("-c, --create",         NULL,      "无需源文件，自动创建");
    help_view_print_help_option("-l, --lang",           "<str>",   "显式指定编程语言（文件后缀），如 cpp/c/py");
    help_view_print_help_option("",                     NULL,      "不指定则使用 default_language 配置项");
    help_view_print_help_option("-y, --yes",            NULL,      "非交互模式，缺少必填字段时直接失败\n");
    help_view_print_help_section("示例:");
    help_view_print_help_example("algo add solution.c -t 两数之和 -d 中等 -m 掌握 -g array,hash -y");
    help_view_print_help_example("algo add -c -t 新题 -d 简单");
    help_view_print_help_example("algo add -c -t 新题 -l py");
}
