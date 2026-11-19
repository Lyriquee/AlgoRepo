/**
 * @file runtime.c
 * @brief 运行时上下文初始化、工作区发现与文件持久化。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include <algorepo/runtime/runtime.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorepo/storage/storage.h>
#include <algorepo/utils/utils_compat.h>
#include <algorepo/utils/utils_str.h>
#include <algorepo/utils/utils_fs.h>


/* --- 私有函数声明 ----------------------------------------------------------- */

static AlgoStatus find_local_workspace_root(char *buffer, size_t size);
static bool       is_home_directory(const char *cwd, const char *home);
static void       ensure_meta_dir_exists(const RuntimeContext *ctx);
static void       ensure_remaining_dirs_exist(const RuntimeContext *ctx);
static AlgoStatus runtime_resolve_config_path(const RuntimeContext *ctx, char *buf, size_t size);
static AlgoStatus runtime_resolve_db_path(const RuntimeContext *ctx, char *buf, size_t size);
static AlgoStatus runtime_resolve_solutions_path(const RuntimeContext *ctx, char *buf, size_t size);
void              runtime_refresh_path_cache(RuntimeContext *ctx);


/* --- 公开接口实现 ---------------------------------------------------------- */

/* 初始化上下文结构体，设置全局模式并清空内存 */
AlgoStatus runtime_init(RuntimeContext *ctx, bool global_mode) {
    if (!ctx) return ALGO_ERR_ARG;

    memset(ctx, 0, sizeof(RuntimeContext));
    ctx->global_mode = global_mode;

    config_init(&ctx->config);
    if (pcol_init(&ctx->pcol) != ALGO_SUCCESS) return ALGO_ERR_MEM;

    utils_compat_get_user_home(ctx->home_dir, sizeof(ctx->home_dir));
    utils_path_normalize_separators(ctx->home_dir);

    return ALGO_SUCCESS;
}

/* 发现工作区、装载并合并配置、打开数据库 */
AlgoStatus runtime_load(RuntimeContext *ctx) {
    if (!ctx) return ALGO_ERR_ARG;

    // 解析物理工作区根目录
    if (!ctx->global_mode) {
        char cwd[ALGO_MAX_PATH_LEN];
        if (utils_compat_getcwd(cwd, sizeof(cwd)) && is_home_directory(cwd, ctx->home_dir)) {
            /* 家目录下未显式传 -g：拒绝回退到全局模式，交由上层处理 */
            return ALGO_ERR_NOT_FOUND;
        } else {
            AlgoStatus status = find_local_workspace_root(ctx->workspace_root, sizeof(ctx->workspace_root));
            if (status != ALGO_SUCCESS) {
                /* 未找到本地工作区：拒绝回退到全局模式，交由上层处理 */
                return ALGO_ERR_NOT_FOUND;
            } else {
                utils_path_normalize_separators(ctx->workspace_root);
            }
        }
    }

    // 确保元数据目录存在
    ensure_meta_dir_exists(ctx);

    // 在家目录下构建全局配置路径
    char global_config_path[ALGO_MAX_PATH_LEN];
    int  w = snprintf(global_config_path, sizeof(global_config_path),
                      "%s/%s/%s", ctx->home_dir, ALGO_META_DIR, ALGO_CONFIG_FILENAME);
    if (w < 0 || (size_t)w >= sizeof(global_config_path)) return ALGO_ERR_RANGE;
    utils_path_normalize_separators(global_config_path);

    runtime_resolve_config_path(ctx, ctx->config_path, sizeof(ctx->config_path));

    // 装载全局配置，缺失时创建默认配置
    AlgoConfig global_cfg;
    config_init(&global_cfg);

    if (config_load(&global_cfg, global_config_path) != ALGO_SUCCESS) {
        config_init(&global_cfg);
        config_save(&global_cfg, global_config_path);
    }

    // 装载本地配置并与全局合并
    if (!ctx->global_mode) {
        AlgoConfig local_cfg;
        config_init_empty(&local_cfg);

        if (config_load(&local_cfg, ctx->config_path) != ALGO_SUCCESS) {
            config_init_empty(&local_cfg);
            // 本地工作区默认使用相对路径
            utils_strcpy_safe(local_cfg.db_path,
                              ALGO_META_DIR "/" ALGO_DB_FILENAME,
                              sizeof(local_cfg.db_path));
            utils_strcpy_safe(local_cfg.solutions_path,
                              ALGO_SOLUTIONS_DIR,
                              sizeof(local_cfg.solutions_path));
            config_save(&local_cfg, ctx->config_path);
        }
        config_merge(&ctx->config, &local_cfg, &global_cfg);
    } else {
        ctx->config = global_cfg;
    }

    // 刷新缓存的路径快照
    runtime_refresh_path_cache(ctx);

    // 按需创建其余目录
    ensure_remaining_dirs_exist(ctx);

    // 从解析后的数据库路径装载题库
    if (storage_load(&ctx->pcol, ctx->db_path) != ALGO_SUCCESS) {
        pcol_clear(&ctx->pcol);
    }

    ctx->workspace_loaded = true;
    return ALGO_SUCCESS;
}

/* 将题库序列化落盘 */
AlgoStatus runtime_save_db(RuntimeContext *ctx) {
    if (!ctx || !ctx->workspace_loaded) return ALGO_SUCCESS;
    return storage_save(&ctx->pcol, ctx->db_path);
}

/* 自动保存数据库并释放所有资源 */
AlgoStatus runtime_destroy(RuntimeContext *ctx) {
    if (!ctx || !ctx->workspace_loaded) return ALGO_SUCCESS;
    storage_save(&ctx->pcol, ctx->db_path);
    pcol_free(&ctx->pcol);
    ctx->workspace_loaded = false;
    return ALGO_SUCCESS;
}

/* 在指定路径初始化工作区目录、配置与数据库 */
AlgoStatus runtime_create_workspace(RuntimeContext *ctx, const char *path) {
    if (!ctx || !path) return ALGO_ERR_ARG;

    if (ctx->global_mode) {
        utils_strcpy_safe(ctx->workspace_root, ctx->home_dir, sizeof(ctx->workspace_root));
    } else {
        char cwd[ALGO_MAX_PATH_LEN] = {0};
        (void)utils_compat_getcwd(cwd, sizeof(cwd));

        if (cwd[0] != '\0' && is_home_directory(cwd, ctx->home_dir)) {
            ctx->global_mode = true;
            utils_strcpy_safe(ctx->workspace_root, ctx->home_dir, sizeof(ctx->workspace_root));
        } else {
            if (utils_path_is_absolute(path)) {
                utils_strcpy_safe(ctx->workspace_root, path, sizeof(ctx->workspace_root));
            } else if (strcmp(path, ".") == 0) {
                utils_strcpy_safe(ctx->workspace_root, cwd, sizeof(ctx->workspace_root));
            } else {
                int w2 = snprintf(ctx->workspace_root, sizeof(ctx->workspace_root), "%s/%s", cwd, path);
                if (w2 < 0 || (size_t)w2 >= sizeof(ctx->workspace_root))
                    utils_strcpy_safe(ctx->workspace_root, cwd, sizeof(ctx->workspace_root));
            }
            ctx->global_mode = false;
            utils_path_normalize_separators(ctx->workspace_root);
        }
    }

    ensure_meta_dir_exists(ctx);
    runtime_resolve_config_path(ctx, ctx->config_path, sizeof(ctx->config_path));

    // 生成初始配置文件
    AlgoConfig cfg;
    if (ctx->global_mode) {
        config_init(&cfg);
    } else {
        config_init_empty(&cfg);
        // 本地工作区默认使用相对路径
        utils_strcpy_safe(cfg.db_path,
                          ALGO_META_DIR "/" ALGO_DB_FILENAME,
                          sizeof(cfg.db_path));
        utils_strcpy_safe(cfg.solutions_path,
                          ALGO_SOLUTIONS_DIR,
                          sizeof(cfg.solutions_path));
    }
    config_save(&cfg, ctx->config_path);
    ctx->config = cfg;

    runtime_refresh_path_cache(ctx);
    ensure_remaining_dirs_exist(ctx);
    storage_save(&ctx->pcol, ctx->db_path);

    return ALGO_SUCCESS;
}


/* --- 私有函数实现 ---------------------------------------------------------- */

/* 根据工作区根目录构建绝对配置文件路径 */
static AlgoStatus runtime_resolve_config_path(const RuntimeContext *ctx, char *buf, size_t size) {
    if (!ctx || !buf || size == 0) return ALGO_ERR_ARG;

    const char *root = ctx->global_mode ? ctx->home_dir : ctx->workspace_root;
    int         w    = snprintf(buf, size, "%s/%s/%s", root, ALGO_META_DIR, ALGO_CONFIG_FILENAME);
    if (w < 0 || (size_t)w >= size) return ALGO_ERR_RANGE;
    utils_path_normalize_separators(buf);
    return ALGO_SUCCESS;
}

/* 解析绝对数据库路径，优先使用配置值回退到默认 */
static AlgoStatus runtime_resolve_db_path(const RuntimeContext *ctx, char *buf, size_t size) {
    if (!ctx || !buf || size == 0) return ALGO_ERR_ARG;

    if (ctx->config.db_path[0] != '\0') {
        if (!utils_path_is_absolute(ctx->config.db_path)) {
            const char *root = ctx->global_mode ? ctx->home_dir : ctx->workspace_root;
            int         w    = snprintf(buf, size, "%s/%s", root, ctx->config.db_path);
            if (w < 0 || (size_t)w >= size) return ALGO_ERR_RANGE;
        } else {
            utils_strcpy_safe(buf, ctx->config.db_path, size);
        }
        utils_path_normalize_separators(buf);
        return ALGO_SUCCESS;
    }

    const char *root = ctx->global_mode ? ctx->home_dir : ctx->workspace_root;
    int         w    = snprintf(buf, size, "%s/%s/%s", root, ALGO_META_DIR, ALGO_DB_FILENAME);
    if (w < 0 || (size_t)w >= size) return ALGO_ERR_RANGE;
    utils_path_normalize_separators(buf);
    return ALGO_SUCCESS;
}

/* 解析绝对题解目录路径，支持回退 */
static AlgoStatus runtime_resolve_solutions_path(const RuntimeContext *ctx, char *buf, size_t size) {
    if (!ctx || !buf || size == 0) return ALGO_ERR_ARG;

    if (ctx->config.solutions_path[0] != '\0') {
        if (!utils_path_is_absolute(ctx->config.solutions_path)) {
            const char *root = ctx->global_mode ? ctx->home_dir : ctx->workspace_root;
            int         w    = snprintf(buf, size, "%s/%s", root, ctx->config.solutions_path);
            if (w < 0 || (size_t)w >= size) return ALGO_ERR_RANGE;
        } else {
            utils_strcpy_safe(buf, ctx->config.solutions_path, size);
        }
        utils_path_normalize_separators(buf);
        return ALGO_SUCCESS;
    }

    const char *root = ctx->global_mode ? ctx->home_dir : ctx->workspace_root;
    int         w;
    if (ctx->global_mode) {
        w = snprintf(buf, size, "%s/%s/%s", root, ALGO_META_DIR, ALGO_SOLUTIONS_DIR);
    } else {
        w = snprintf(buf, size, "%s/%s", root, ALGO_SOLUTIONS_DIR);
    }
    if (w < 0 || (size_t)w >= size) return ALGO_ERR_RANGE;
    utils_path_normalize_separators(buf);
    return ALGO_SUCCESS;
}

/* 重新解析所有缓存的路径快照 */
void runtime_refresh_path_cache(RuntimeContext *ctx) {
    runtime_resolve_config_path(ctx, ctx->config_path, sizeof(ctx->config_path));
    runtime_resolve_db_path(ctx, ctx->db_path, sizeof(ctx->db_path));
    runtime_resolve_solutions_path(ctx, ctx->solutions_path, sizeof(ctx->solutions_path));
}

/* 从当前目录向上查找带标记目录的工作区根目录 */
static AlgoStatus find_local_workspace_root(char *buffer, size_t size) {
    if (!buffer || size == 0) return ALGO_ERR_ARG;
    if (!utils_compat_getcwd(buffer, size)) return ALGO_ERR_FILE;

    /* 获取家目录作为边界，避免越过家目录继续向上搜索 */
    char home[ALGO_MAX_PATH_LEN] = {0};
    utils_compat_get_user_home(home, sizeof(home));

    while (1) {
        /* 到达家目录边界：不再向上查找，家目录 .algorepo 属于全局而非本地工作区 */
        if (home[0] && strcmp(buffer, home) == 0) break;

        char test_path[ALGO_MAX_PATH_LEN];
        int  w = snprintf(test_path, sizeof(test_path), "%s/%s", buffer, ALGO_META_DIR);
        if (w < 0 || (size_t)w >= sizeof(test_path)) break;

        if (utils_compat_path_exists(test_path)) return ALGO_SUCCESS;

        char *slash = utils_compat_get_last_slash(buffer);
        if (!slash || slash == buffer || (slash - buffer == 2 && buffer[1] == ':')) {
            break;
        }
        *slash = '\0';
    }
    return ALGO_ERR_NOT_FOUND;
}

/* 判断路径是否等于用户家目录 */
static bool is_home_directory(const char *cwd, const char *home) {
    if (!cwd || !home) return false;
    return strcmp(cwd, home) == 0;
}

/* 缺失时创建隐藏元数据目录 */
static void ensure_meta_dir_exists(const RuntimeContext *ctx) {
    char        path[ALGO_MAX_PATH_LEN];
    const char *root = ctx->global_mode ? ctx->home_dir : ctx->workspace_root;
    int         w    = snprintf(path, sizeof(path), "%s/%s", root, ALGO_META_DIR);
    if (w < 0 || (size_t)w >= sizeof(path)) return;
    utils_compat_make_dir(path);
}

/* 按需创建题解目录和数据库父目录 */
static void ensure_remaining_dirs_exist(const RuntimeContext *ctx) {
    // 确保题解源码目录存在
    utils_compat_make_dir(ctx->solutions_path);

    // 提取数据库父目录并创建（如需要）
    char db_dir[ALGO_MAX_PATH_LEN];
    utils_strcpy_safe(db_dir, ctx->db_path, sizeof(db_dir));

    char *slash = utils_compat_get_last_slash(db_dir);
    if (slash && slash != db_dir) {
        *slash = '\0';
        utils_compat_make_dir(db_dir);
    }
}
