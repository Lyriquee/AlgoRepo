/**
 * @file command_tag.c
 * @brief 标签管理命令实现。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include "command_tag.h"

#include <stdio.h>
#include <string.h>

#include <algorepo/console/console_ui.h>
#include <algorepo/service/problem_query.h>
#include <algorepo/utils/utils_str.h>

#include "command_common.h"
#include "views/problem_view.h"


/* --- 私有函数声明 --------------------------------------------------------- */

static AlgoStatus do_tag_list(RuntimeContext *ctx, const char *filter_tag);
static AlgoStatus do_tag_rename(ProblemCollection *pcol, const char *old_name, const char *new_name);
static AlgoStatus do_tag_delete(ProblemCollection *pcol, const char *tag);
static AlgoStatus do_tag_filter(RuntimeContext *ctx, const char *tag);


/* --- 公开接口实现 --------------------------------------------------------- */

/* 显示 tag 子命令的帮助信息 */
void help_tag(void) {
    help_view_print_usage("tag", "<action> [参数]");
    help_view_print_description("管理题目标签系统");
    help_view_print_help_section("动作 & 别名:");
    help_view_print_help_option("list, ls",      "[tag]",       "列出所有标签");
    help_view_print_help_option("add, a",        "<id>[,<id>...] <tag>",  "为指定题目添加标签");
    help_view_print_help_option("rm, r",         "<id>[,<id>...] <tag>",  "移除指定题目的某个标签");
    help_view_print_help_option("set, s",        "<id>[,<id>...] <tags>", "覆盖设置标签");
    help_view_print_help_option("clear, c",      "<id>[,<id>...]",        "清空指定题目的所有标签");
    help_view_print_help_option("show, sh",      "<id>[,<id>...]",        "查看指定题目当前拥有的所有标签");
    help_view_print_help_option("rename, mv",    "<old> <new>", "全局重命名标签");
    help_view_print_help_option("delete, purge", "<tag>",       "全局彻底删除某个标签\n");
    help_view_print_help_section("选项:");
    help_view_print_help_option("-h, --help",    NULL,          "显示此帮助信息\n");
    help_view_print_help_section("示例:");
    help_view_print_help_example("algo tag array");
    help_view_print_help_example("algo tag add 1 dp");
    help_view_print_help_example("algo tag mv ptr two-ptr");
}

/* tag 命令入口：根据 action 参数分发到对应的处理逻辑 */
AlgoStatus cmd_tag(int argc, char *argv[], RuntimeContext *ctx) {
    if (argc < 2) return do_tag_list(ctx, NULL);

    const char *action = argv[1];

    if (strcmp(action, "list") == 0 || strcmp(action, "ls") == 0)
        return do_tag_list(ctx, argc > 2 ? argv[2] : NULL);

    if (strcmp(action, "add") == 0 || strcmp(action, "a") == 0) {
        if (argc < 4) { console_log_error("用法: algo tag add <id>[,<id>...] <tag>"); return ALGO_ERR_REQUIRED; }
        int ids[256], n = 0;
        if (parse_comma_ids(argv[2], ids, &n, 256) != ALGO_SUCCESS) return ALGO_ERR_ARG;
        int added = 0, skipped[256], ns = 0;
        for (int i = 0; i < n; i++) {
            Problem *p = pcol_get_by_id(&ctx->pcol, ids[i]);
            if (!p) { console_log_error("题目不存在: #%d", ids[i]); continue; }
            if (problem_has_tag(p, argv[3])) { skipped[ns++] = ids[i]; continue; }
            AlgoStatus s = problem_add_tag(p, argv[3]);
            if (s != ALGO_SUCCESS) { console_log_error("#%d 添加标签失败", ids[i]); }
            else added++;
        }
        if (added > 0) console_log_success("已为 %d 道题目添加标签: %s", added, argv[3]);
        if (ns > 0) {
            char idstr[1024] = {0};
            int pos = 0;
            for (int i = 0; i < ns && pos < (int)sizeof(idstr) - 8; i++)
                pos += snprintf(idstr + pos, sizeof(idstr) - pos, "%s#%d", i > 0 ? ", " : "", skipped[i]);
            console_log_info("info: %s 已存在该标签，跳过", idstr);
        }
        return ALGO_SUCCESS;
    }

    if (strcmp(action, "rm") == 0 || strcmp(action, "r") == 0) {
        if (argc < 4) { console_log_error("用法: algo tag rm <id>[,<id>...] <tag>"); return ALGO_ERR_REQUIRED; }
        int ids[256], n = 0;
        if (parse_comma_ids(argv[2], ids, &n, 256) != ALGO_SUCCESS) return ALGO_ERR_ARG;
        int removed = 0;
        for (int i = 0; i < n; i++) {
            Problem *p = pcol_get_by_id(&ctx->pcol, ids[i]);
            if (!p) { console_log_error("题目不存在: #%d", ids[i]); continue; }
            AlgoStatus s = problem_remove_tag(p, argv[3]);
            if (s != ALGO_SUCCESS) {
                console_log_error("#%d 移除标签失败", ids[i]);
            } else {
                removed++;
            }
        }
        if (removed == 0) {
            console_log_error("未能从任何题目移除标签 '%s'", argv[3]);
        } else {
            console_log_success("已从 %d 道题目移除标签: %s", removed, argv[3]);
        }
        return ALGO_SUCCESS;
    }

    if (strcmp(action, "set") == 0 || strcmp(action, "s") == 0) {
        if (argc < 3) { console_log_error("用法: algo tag set <id>[,<id>...] <tags...>"); return ALGO_ERR_REQUIRED; }
        int ids[256], n = 0;
        if (parse_comma_ids(argv[2], ids, &n, 256) != ALGO_SUCCESS) return ALGO_ERR_ARG;
        /* 构建 ID 列表用于确认提示 */
        char idstr[1024] = {0};
        int pos = 0;
        for (int i = 0; i < n && pos < (int)sizeof(idstr) - 8; i++)
            pos += snprintf(idstr + pos, sizeof(idstr) - pos, "%s#%d", i > 0 ? ", " : "", ids[i]);
        char prompt[1280];
        snprintf(prompt, sizeof(prompt), "确认覆盖设置 %s 的标签?", idstr);
        bool confirmed = false;
        if (console_confirm(prompt, &confirmed) != ALGO_SUCCESS || !confirmed) return ALGO_SUCCESS;
        int updated = 0;
        for (int i = 0; i < n; i++) {
            Problem *p = pcol_get_by_id(&ctx->pcol, ids[i]);
            if (!p) { console_log_error("题目不存在: #%d", ids[i]); continue; }
            memset(p->tags, 0, sizeof(p->tags)); p->tag_count = 0;
            for (int j = 3; j < argc; j++) {
                AlgoStatus s = problem_add_tag(p, argv[j]);
                if (s == ALGO_ERR_TAG_FULL) { console_log_error("#%d 标签已满", ids[i]); break; }
                if (s != ALGO_SUCCESS) { console_log_error("#%d 标签设置失败", ids[i]); break; }
            }
            updated++;
        }
        if (updated > 0) console_log_success("已更新 %d 道题目标签", updated);
        return ALGO_SUCCESS;
    }

    if (strcmp(action, "clear") == 0 || strcmp(action, "c") == 0) {
        if (argc < 3) { console_log_error("用法: algo tag clear <id>[,<id>...]"); return ALGO_ERR_REQUIRED; }
        int ids[256], n = 0;
        if (parse_comma_ids(argv[2], ids, &n, 256) != ALGO_SUCCESS) return ALGO_ERR_ARG;
        /* 构建 ID 列表用于确认提示 */
        char idstr[1024] = {0};
        int pos = 0;
        for (int i = 0; i < n && pos < (int)sizeof(idstr) - 8; i++)
            pos += snprintf(idstr + pos, sizeof(idstr) - pos, "%s#%d", i > 0 ? ", " : "", ids[i]);
        char prompt[1280];
        snprintf(prompt, sizeof(prompt), "确认清空 %s 的所有标签?", idstr);
        bool confirmed = false;
        if (console_confirm(prompt, &confirmed) != ALGO_SUCCESS || !confirmed) return ALGO_SUCCESS;
        int cleared = 0;
        for (int i = 0; i < n; i++) {
            Problem *p = pcol_get_by_id(&ctx->pcol, ids[i]);
            if (!p) { console_log_error("题目不存在: #%d", ids[i]); continue; }
            if (p->tag_count > 0) {
                memset(p->tags, 0, sizeof(p->tags)); p->tag_count = 0;
                cleared++;
            }
        }
        if (cleared > 0) console_log_success("已清空 %d 道题目的所有标签", cleared);
        else console_log_info("所有目标题目已无标签，无需清空");
        return ALGO_SUCCESS;
    }

    if (strcmp(action, "show") == 0 || strcmp(action, "sh") == 0) {
        if (argc < 3) { console_log_error("用法: algo tag show <id>[,<id>...]"); return ALGO_ERR_REQUIRED; }
        int ids[256], n = 0;
        if (parse_comma_ids(argv[2], ids, &n, 256) != ALGO_SUCCESS) return ALGO_ERR_ARG;
        for (int i = 0; i < n; i++) {
            Problem *p = pcol_get_by_id(&ctx->pcol, ids[i]);
            if (!p) { console_log_error("题目不存在: #%d", ids[i]); continue; }
            char tbuf[64];
            snprintf(tbuf, sizeof(tbuf), "题目 #%d 标签:", ids[i]);
            console_print_title(tbuf);
            if (p->tag_count == 0) {
                console_print_muted("  (无标签)\n");
            } else {
                for (size_t j = 0; j < p->tag_count; j++) {
                    console_print_muted("  - ");
                    console_print_role(COLOR_ROLE_TAG, "%s\n", p->tags[j]);
                }
            }
        }
        return ALGO_SUCCESS;
    }

    if (strcmp(action, "rename") == 0 || strcmp(action, "mv") == 0) {
        if (argc < 4) { console_log_error("用法: algo tag rename <old> <new>"); return ALGO_ERR_REQUIRED; }
        /* 先统计受影响的题目 */
        int affected[256], na = 0;
        for (size_t i = 0; i < ctx->pcol.size && na < 256; i++) {
            if (problem_has_tag(&ctx->pcol.items[i], argv[2]))
                affected[na++] = ctx->pcol.items[i].id;
        }
        if (na == 0) {
            console_log_error("标签 '%s' 不存在于任何题目中", argv[2]);
            return ALGO_ERR_NOT_FOUND;
        }
        /* 构建 ID 列表字符串 */
        char idstr[1024] = {0};
        int pos = 0;
        for (int i = 0; i < na && pos < (int)sizeof(idstr) - 8; i++)
            pos += snprintf(idstr + pos, sizeof(idstr) - pos, "%s#%d", i > 0 ? ", " : "", affected[i]);
        /* 确认提示 */
        char prompt[1408];
        snprintf(prompt, sizeof(prompt), "确认将标签 '%s' 重命名为 '%s'? (影响 %d 题: %s)", argv[2], argv[3], na, idstr);
        bool confirmed = false;
        if (console_confirm(prompt, &confirmed) != ALGO_SUCCESS || !confirmed) return ALGO_SUCCESS;
        AlgoStatus s = do_tag_rename(&ctx->pcol, argv[2], argv[3]);
        if (s != ALGO_SUCCESS) return s;
        console_log_success("已重命名: '%s' -> '%s' (%d 题: %s)", argv[2], argv[3], na, idstr);
        return ALGO_SUCCESS;
    }

    if (strcmp(action, "delete") == 0 || strcmp(action, "purge") == 0) {
        if (argc < 3) { console_log_error("用法: algo tag delete <tag>"); return ALGO_ERR_REQUIRED; }
        /* 先统计受影响的题目 */
        int affected[256], na = 0;
        for (size_t i = 0; i < ctx->pcol.size && na < 256; i++) {
            if (problem_has_tag(&ctx->pcol.items[i], argv[2]))
                affected[na++] = ctx->pcol.items[i].id;
        }
        if (na == 0) {
            console_log_error("标签 '%s' 不存在于任何题目中", argv[2]);
            return ALGO_ERR_NOT_FOUND;
        }
        /* 构建 ID 列表字符串 */
        char idstr[1024] = {0};
        int pos = 0;
        for (int i = 0; i < na && pos < (int)sizeof(idstr) - 8; i++)
            pos += snprintf(idstr + pos, sizeof(idstr) - pos, "%s#%d", i > 0 ? ", " : "", affected[i]);
        /* 确认提示 */
        char prompt[1408];
        snprintf(prompt, sizeof(prompt), "确认全局删除标签 '%s'? (影响 %d 题: %s)", argv[2], na, idstr);
        bool confirmed = false;
        if (console_confirm(prompt, &confirmed) != ALGO_SUCCESS || !confirmed) return ALGO_SUCCESS;
        AlgoStatus s = do_tag_delete(&ctx->pcol, argv[2]);
        if (s != ALGO_SUCCESS) return s;
        console_log_success("已删除标签 '%s' (%d 题: %s)", argv[2], na, idstr);
        return ALGO_SUCCESS;
    }

    if (strcmp(action, "-h") == 0 || strcmp(action, "--help") == 0) {
        help_tag();
        return ALGO_SUCCESS;
    }

    // 将未知 action 作为标签名进行筛选
    return do_tag_filter(ctx, action);
}


/* --- 私有函数实现 --------------------------------------------------------- */

/* 列出所有标签及统计，若指定 filter_tag 则按标签筛选并列出题目 */
static AlgoStatus do_tag_list(RuntimeContext *ctx, const char *filter_tag) {
    const ProblemCollection *pcol = &ctx->pcol;
    if (filter_tag) {
        ProblemCollection result;
        AlgoStatus st = pcol_init(&result);
        if (st != ALGO_SUCCESS) return st;
        query_by_tag(ctx, filter_tag, &result);
        problem_view_print_list(&result, "没有匹配该标签的题目", false);
        pcol_free(&result);
        return ALGO_SUCCESS;
    }

    char   names[256][ALGO_MAX_TAG_LEN] = {{0}};
    size_t counts[256] = {0};
    size_t used = 0;
    for (size_t i = 0; i < pcol->size; i++) {
        const Problem *p = &pcol->items[i];
        for (size_t j = 0; j < p->tag_count; j++) {
            size_t k = 0;
            for (; k < used; k++) if (strcmp(names[k], p->tags[j]) == 0) break;
            if (k == used && used < 256) {
                utils_strcpy_safe(names[used], p->tags[j], sizeof(names[used]));
                used++;
            }
            if (k < 256) counts[k]++;
        }
    }
    console_print_title("标签统计:");
    if (used == 0) {
        console_print_muted("  -\n");
        return ALGO_SUCCESS;
    }
    for (size_t i = 0; i < used; i++) {
        console_print_spaces(2);
        console_print_role(COLOR_ROLE_TAG, "%-20s", names[i]);
        console_print_role(COLOR_ROLE_VALUE, " %llu\n",
                           (unsigned long long)counts[i]);
    }
    return ALGO_SUCCESS;
}

/* 全局重命名标签：遍历所有题目，将匹配的标签名替换为新名称（静默，输出由调用者负责） */
static AlgoStatus do_tag_rename(ProblemCollection *pcol, const char *old_name, const char *new_name) {
    for (size_t i = 0; i < pcol->size; i++) {
        Problem *p = &pcol->items[i];
        for (size_t j = 0; j < p->tag_count; j++) {
            if (strcmp(p->tags[j], old_name) == 0) {
                utils_strcpy_safe(p->tags[j], new_name, ALGO_MAX_TAG_LEN);
            }
        }
    }
    return ALGO_SUCCESS;
}

/* 全局删除标签：遍历所有题目，移除匹配的标签（静默，输出由调用者负责） */
static AlgoStatus do_tag_delete(ProblemCollection *pcol, const char *tag) {
    for (size_t i = 0; i < pcol->size; i++) {
        Problem *p = &pcol->items[i];
        problem_remove_tag(p, tag);
    }
    return ALGO_SUCCESS;
}

/* 按标签筛选题目列表（将未知 action 当作标签名处理） */
static AlgoStatus do_tag_filter(RuntimeContext *ctx, const char *tag) {
    ProblemCollection result;
    AlgoStatus st = pcol_init(&result);
    if (st != ALGO_SUCCESS) return st;
    query_by_tag(ctx, tag, &result);
    problem_view_print_list(&result, "没有匹配该标签的题目", false);
    pcol_free(&result);
    return ALGO_SUCCESS;
}
