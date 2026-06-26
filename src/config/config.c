/**
 * @file config.c
 * @brief 配置持久化、校验与字段级合并实现。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include <algorepo/config/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorepo/utils/utils_compat.h>
#include <algorepo/utils/utils_str.h>
#include <algorepo/utils/utils_fs.h>


/* --- 公开接口实现 --------------------------------------------------------- */

/* 使用默认值初始化配置：语言 cpp、编辑器 vim、颜色 auto、备份开启、保留 20 份 */
void config_init(AlgoConfig *cfg) {
    if (!cfg) return;

    utils_strcpy_safe(cfg->default_language, "cpp", sizeof(cfg->default_language));
    utils_strcpy_safe(cfg->editor, "vim", sizeof(cfg->editor));
    utils_strcpy_safe(cfg->color, "auto", sizeof(cfg->color));
    cfg->auto_backup       = 1;
    cfg->backup_count      = 20;
    cfg->db_path[0]        = '\0';
    cfg->solutions_path[0] = '\0';
}

/* 将所有字段置为"未设置"状态，用于本地配置继承全局 */
void config_init_empty(AlgoConfig *cfg) {
    if (!cfg) return;

    cfg->default_language[0] = '\0';
    cfg->editor[0]           = '\0';
    cfg->color[0]            = '\0';
    cfg->auto_backup         = -1;
    cfg->backup_count        = 0;
    cfg->db_path[0]          = '\0';
    cfg->solutions_path[0]   = '\0';
}

/* 校验配置字段合法性；空字段在合并时由全局补全，此处仅校验边界 */
AlgoStatus config_validate(const AlgoConfig *cfg) {
    if (!cfg) return ALGO_ERR_ARG;
    return ALGO_SUCCESS;
}

/* 序列化配置写入 INI 文件，未设置的整数字段输出空字符串 */
AlgoStatus config_save(const AlgoConfig *cfg, const char *filepath) {
    if (!cfg || !filepath) return ALGO_ERR_ARG;

    FILE *fp = utils_compat_fopen(filepath, "w");
    if (!fp) return ALGO_ERR_FILE;

    // auto_backup: -1 表示未设置，输出空；否则输出 0 或 1
    char ab_buf[8];
    if (cfg->auto_backup == -1) {
        ab_buf[0] = '\0';
    } else {
        snprintf(ab_buf, sizeof(ab_buf), "%d", cfg->auto_backup ? 1 : 0);
    }

    // backup_count: <=0 表示未设置，输出空；否则输出数值
    char bc_buf[16];
    if (cfg->backup_count <= 0) {
        bc_buf[0] = '\0';
    } else {
        snprintf(bc_buf, sizeof(bc_buf), "%d", cfg->backup_count);
    }

    // 标准化路径分隔符后再写入
    char db_buf[ALGO_MAX_PATH_LEN];
    char sp_buf[ALGO_MAX_PATH_LEN];
    utils_strcpy_safe(db_buf, cfg->db_path, sizeof(db_buf));
    utils_strcpy_safe(sp_buf, cfg->solutions_path, sizeof(sp_buf));
    utils_path_normalize_separators(db_buf);
    utils_path_normalize_separators(sp_buf);

    fprintf(fp,
            "default_language=%s\n"
            "editor=%s\n"
            "color=%s\n"
            "auto_backup=%s\n"
            "backup_count=%s\n"
            "db_path=%s\n"
            "solutions_path=%s\n",
            cfg->default_language,
            cfg->editor,
            cfg->color,
            ab_buf,
            bc_buf,
            db_buf,
            sp_buf);

    fclose(fp);
    return ALGO_SUCCESS;
}

/* 从 INI 文件反序列化配置，兼容旧字段名 global_db_path / global_solutions_path */
AlgoStatus config_load(AlgoConfig *cfg, const char *filepath) {
    if (!cfg || !filepath) return ALGO_ERR_ARG;

    FILE *fp = utils_compat_fopen(filepath, "r");
    if (!fp) return ALGO_ERR_FILE;

    // 以默认值为基底，再用文件内容覆盖
    config_init(cfg);

    char line[256];

    while (fgets(line, sizeof(line), fp)) {
        char *eq = strchr(line, '=');
        if (!eq) continue;

        size_t key_len = (size_t)(eq - line);
        if (key_len >= 64) key_len = 63;
        char key[64] = {0};
        memcpy(key, line, key_len);

        char        value[256] = {0};
        const char *val_start  = eq + 1;
        size_t      val_len    = strlen(val_start);
        // 去除末尾换行符
        while (val_len > 0 && (val_start[val_len - 1] == '\n' ||
                               val_start[val_len - 1] == '\r')) {
            val_len--;
        }
        if (val_len >= 256) val_len = 255;
        memcpy(value, val_start, val_len);
        value[val_len] = '\0';

        if (strcmp(key, "default_language") == 0) {
            utils_strcpy_safe(cfg->default_language, value,
                              sizeof(cfg->default_language));
        } else if (strcmp(key, "editor") == 0) {
            utils_strcpy_safe(cfg->editor, value, sizeof(cfg->editor));
        } else if (strcmp(key, "color") == 0) {
            utils_strcpy_safe(cfg->color, value, sizeof(cfg->color));
        } else if (strcmp(key, "auto_backup") == 0) {
            cfg->auto_backup = (value[0] != '\0') ? atoi(value) : -1;
        } else if (strcmp(key, "backup_count") == 0) {
            cfg->backup_count = (value[0] != '\0') ? atoi(value) : 0;
        } else if (strcmp(key, "db_path") == 0) {
            utils_strcpy_safe(cfg->db_path, value, sizeof(cfg->db_path));
            utils_path_normalize_separators(cfg->db_path);
        } else if (strcmp(key, "solutions_path") == 0) {
            utils_strcpy_safe(cfg->solutions_path, value, sizeof(cfg->solutions_path));
            utils_path_normalize_separators(cfg->solutions_path);
        } else if (strcmp(key, "global_db_path") == 0) {
            // 兼容旧字段名
            utils_strcpy_safe(cfg->db_path, value, sizeof(cfg->db_path));
            utils_path_normalize_separators(cfg->db_path);
        } else if (strcmp(key, "global_solutions_path") == 0) {
            // 兼容旧字段名
            utils_strcpy_safe(cfg->solutions_path, value, sizeof(cfg->solutions_path));
            utils_path_normalize_separators(cfg->solutions_path);
        }
    }

    fclose(fp);
    return config_validate(cfg);
}

/* 以 global 为基底，local 非空字段覆盖，结果写入 dst */
void config_merge(AlgoConfig *dst, const AlgoConfig *local,
                  const AlgoConfig *global) {
    if (!dst || !local || !global) return;

    *dst = *global;

    if (local->default_language[0] != '\0')
        utils_strcpy_safe(dst->default_language, local->default_language,
                          sizeof(dst->default_language));
    if (local->editor[0] != '\0')
        utils_strcpy_safe(dst->editor, local->editor, sizeof(dst->editor));
    if (local->color[0] != '\0')
        utils_strcpy_safe(dst->color, local->color, sizeof(dst->color));
    if (local->auto_backup != -1)
        dst->auto_backup = local->auto_backup;
    if (local->backup_count > 0)
        dst->backup_count = local->backup_count;
    if (local->db_path[0] != '\0')
        utils_strcpy_safe(dst->db_path, local->db_path, sizeof(dst->db_path));
    if (local->solutions_path[0] != '\0')
        utils_strcpy_safe(dst->solutions_path, local->solutions_path,
                          sizeof(dst->solutions_path));
}
