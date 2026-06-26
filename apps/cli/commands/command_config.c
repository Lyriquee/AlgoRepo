/**
 * @file command_config.c
 * @brief 配置管理命令实现。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include "command_config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <algorepo/console/console_ui.h>
#include <algorepo/utils/utils_str.h>

#include "command.h"
#include "command_common.h"


/* --- 私有函数声明 --------------------------------------------------------- */

static AlgoStatus  save_current_config(RuntimeContext *ctx, const char *changed_key);
static const char *auto_backup_str(int val);
static const char *backup_count_str(int val);
static const char *path_str(const char *val);


/* --- 公开接口实现 --------------------------------------------------------- */

/* 配置命令主入口：处理 list/get/set/reset 子命令 */
AlgoStatus cmd_config(int argc, char *argv[], RuntimeContext *ctx) {
    if (argc < 2) {
        console_log_error("用法: algo config <list|get|set|reset> [key] [value]");
        return ALGO_ERR_REQUIRED;
    }

    const char *sub = argv[1];

    if (strcmp(sub, "list") == 0 || strcmp(sub, "ls") == 0) {
        console_print_title("当前配置项:");
        console_print_kv("editor", ctx->config.editor);
        console_print_kv("default_language", ctx->config.default_language);
        console_print_kv("color", ctx->config.color);
        console_print_kv("auto_backup", auto_backup_str(ctx->config.auto_backup));
        console_print_kv("backup_count", backup_count_str(ctx->config.backup_count));
        console_print_kv("db_path", path_str(ctx->config.db_path));
        console_print_kv("solutions_path", path_str(ctx->config.solutions_path));
        return ALGO_SUCCESS;
    }

    if (strcmp(sub, "get") == 0 || strcmp(sub, "g") == 0) {
        if (argc < 3) {
            console_log_error("用法: algo config get <key>");
            return ALGO_ERR_REQUIRED;
        }
        const char *key = argv[2];
        if (strcmp(key, "editor") == 0) console_print_role(COLOR_ROLE_VALUE, "%s\n", ctx->config.editor);
        else if (strcmp(key, "default_language") == 0) console_print_role(COLOR_ROLE_VALUE, "%s\n", ctx->config.default_language);
        else if (strcmp(key, "color") == 0) console_print_role(COLOR_ROLE_VALUE, "%s\n", ctx->config.color);
        else if (strcmp(key, "auto_backup") == 0) console_print_role(COLOR_ROLE_VALUE, "%d\n", ctx->config.auto_backup);
        else if (strcmp(key, "backup_count") == 0) console_print_role(COLOR_ROLE_VALUE, "%d\n", ctx->config.backup_count);
        else if (strcmp(key, "db_path") == 0) console_print_role(COLOR_ROLE_VALUE, "%s\n", ctx->config.db_path);
        else if (strcmp(key, "solutions_path") == 0) console_print_role(COLOR_ROLE_VALUE, "%s\n", ctx->config.solutions_path);
        else {
            console_log_error("未知配置键: %s", key);
            return ALGO_ERR_ARG;
        }
        return ALGO_SUCCESS;
    }

    if (strcmp(sub, "set") == 0 || strcmp(sub, "s") == 0) {
        if (argc < 4) {
            console_log_error("用法: algo config set <key> <value>");
            return ALGO_ERR_REQUIRED;
        }
        const char *key = argv[2];
        const char *value = argv[3];

        if (strcmp(key, "editor") == 0) {
            utils_strcpy_safe(ctx->config.editor, value, sizeof(ctx->config.editor));
        } else if (strcmp(key, "default_language") == 0) {
            utils_strcpy_safe(ctx->config.default_language, value, sizeof(ctx->config.default_language));
        } else if (strcmp(key, "color") == 0) {
            if (strcmp(value, "auto") != 0 && strcmp(value, "never") != 0 && strcmp(value, "always") != 0) {
                console_log_error("无效颜色模式: %s，请使用 auto/never/always", value);
                return ALGO_ERR_ARG;
            }
            utils_strcpy_safe(ctx->config.color, value, sizeof(ctx->config.color));
            console_set_color_mode(value);
        } else if (strcmp(key, "auto_backup") == 0) {
            ctx->config.auto_backup = (value[0] != '\0') ? atoi(value) : -1;
        } else if (strcmp(key, "backup_count") == 0) {
            ctx->config.backup_count = (value[0] != '\0') ? atoi(value) : 0;
        } else if (strcmp(key, "db_path") == 0) {
            utils_strcpy_safe(ctx->config.db_path, value, sizeof(ctx->config.db_path));
        } else if (strcmp(key, "solutions_path") == 0) {
            utils_strcpy_safe(ctx->config.solutions_path, value, sizeof(ctx->config.solutions_path));
        } else {
            console_log_error("未知配置键: %s", key);
            return ALGO_ERR_ARG;
        }

        AlgoStatus st = save_current_config(ctx, key);
        if (st != ALGO_SUCCESS) { print_status_error("保存配置", st); return st; }
        console_log_success("配置已更新: %s = %s", key, value);
        return ALGO_SUCCESS;
    }

    if (strcmp(sub, "reset") == 0) {
        bool confirmed = false;
        AlgoStatus cf = console_confirm("确认重置所有配置?", &confirmed);
        if (cf != ALGO_SUCCESS) return cf;
        if (!confirmed) { console_log_info("已取消"); return ALGO_SUCCESS; }

        if (ctx->global_mode) {
            config_init(&ctx->config);
        } else {
            config_init_empty(&ctx->config);
            // 本地重置：保留相对路径默认值
            utils_strcpy_safe(ctx->config.db_path,
                             ALGO_META_DIR "/" ALGO_DB_FILENAME,
                             sizeof(ctx->config.db_path));
            utils_strcpy_safe(ctx->config.solutions_path,
                             ALGO_SOLUTIONS_DIR,
                             sizeof(ctx->config.solutions_path));
        }
        AlgoStatus st = config_save(&ctx->config, ctx->config_path);
        if (st != ALGO_SUCCESS) { print_status_error("保存配置", st); return st; }

        if (!ctx->global_mode) {
            AlgoConfig local_cfg = ctx->config;
            AlgoConfig global_cfg;
            char global_path[ALGO_MAX_PATH_LEN];
            int w = snprintf(global_path, sizeof(global_path), "%s/%s/%s",
                             ctx->home_dir, ALGO_META_DIR, ALGO_CONFIG_FILENAME);
            if (w < 0 || (size_t)w >= sizeof(global_path)) {
                console_log_error("全局配置路径过长");
                return ALGO_ERR_RANGE;
            }
            if (config_load(&global_cfg, global_path) != ALGO_SUCCESS) {
                config_init(&global_cfg);
            }
            config_merge(&ctx->config, &local_cfg, &global_cfg);
        }
        console_log_success("配置已重置为默认值");
        return ALGO_SUCCESS;
    }

    console_log_error("未知子命令: %s，请使用 list/get/set/reset", sub);
    return ALGO_ERR_ARG;
}

/* 配置命令帮助信息 */
void help_config(void) {
    help_view_print_usage("config", "<subcommand> [key] [value]");
    help_view_print_description("管理工具配置");
    help_view_print_help_section("子命令 (别名):");
    help_view_print_help_option("list, ls",             NULL,           "列出当前所有配置项");
    help_view_print_help_option("get, g",               "<key>",        "获取指定配置项的值");
    help_view_print_help_option("set, s",               "<key> <value>","设置指定配置项的值");
    help_view_print_help_option("reset",                NULL,           "重置所有配置为默认值\n");
    help_view_print_help_section("配置键:");
    help_view_print_help_option("editor",               NULL,           "默认编辑器");
    help_view_print_help_option("default_language",     NULL,           "默认编程语言");
    help_view_print_help_option("auto_backup",          NULL,           "自动备份 (空=继承/0=关/1=开)");
    help_view_print_help_option("backup_count",         NULL,           "备份保留份数 (空=继承, 默认20)");
    help_view_print_help_option("color",                NULL,           "颜色模式 (auto/never/always)");
    help_view_print_help_option("db_path",              NULL,           "数据库文件路径 (支持相对路径)");
    help_view_print_help_option("solutions_path",       NULL,           "题解代码目录 (支持相对路径)\n");
    help_view_print_help_section("示例:");
    help_view_print_help_example("algo config list");
    help_view_print_help_example("algo config set editor code");
    help_view_print_help_example("algo config get editor");
}


/* --- 私有函数实现 --------------------------------------------------------- */

/* 将当前配置保存到对应的配置文件（全局/本地） */
static AlgoStatus save_current_config(RuntimeContext *ctx, const char *changed_key) {
    if (ctx->global_mode) {
        return config_save(&ctx->config, ctx->config_path);
    }

    AlgoConfig local_cfg;
    AlgoStatus  st = config_load(&local_cfg, ctx->config_path);
    if (st != ALGO_SUCCESS) {
        config_init_empty(&local_cfg);
    }

    if (changed_key) {
        if (strcmp(changed_key, "editor") == 0) {
            utils_strcpy_safe(local_cfg.editor, ctx->config.editor, sizeof(local_cfg.editor));
        } else if (strcmp(changed_key, "default_language") == 0) {
            utils_strcpy_safe(local_cfg.default_language, ctx->config.default_language, sizeof(local_cfg.default_language));
        } else if (strcmp(changed_key, "color") == 0) {
            utils_strcpy_safe(local_cfg.color, ctx->config.color, sizeof(local_cfg.color));
        } else if (strcmp(changed_key, "auto_backup") == 0) {
            local_cfg.auto_backup = ctx->config.auto_backup;
        } else if (strcmp(changed_key, "backup_count") == 0) {
            local_cfg.backup_count = ctx->config.backup_count;
        } else if (strcmp(changed_key, "db_path") == 0) {
            utils_strcpy_safe(local_cfg.db_path, ctx->config.db_path, sizeof(local_cfg.db_path));
        } else if (strcmp(changed_key, "solutions_path") == 0) {
            utils_strcpy_safe(local_cfg.solutions_path, ctx->config.solutions_path, sizeof(local_cfg.solutions_path));
        }
    }

    return config_save(&local_cfg, ctx->config_path);
}

/* 格式化 auto_backup 显示值 */
static const char *auto_backup_str(int val) {
    if (val == -1) return "(继承)";
    return val ? "1 (开启)" : "0 (关闭)";
}

/* 格式化 backup_count 显示值 */
static const char *backup_count_str(int val) {
    static char buf[32];
    if (val <= 0) return "(继承)";
    snprintf(buf, sizeof(buf), "%d", val);
    return buf;
}

/* 格式化路径显示值 */
static const char *path_str(const char *val) {
    return (val && val[0]) ? val : "(默认)";
}
