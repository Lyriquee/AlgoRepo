/**
 * @file problem_service.c
 * @brief 应用服务层实现：题目增删改查的业务用例编排。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 *
 * 服务层封装对题库的原子操作，组合多个底层调用（实体校验、路径生成、
 * 文件拷贝、题库增删）形成完整业务用例。所有修改类操作在失败时尽最大
 * 努力回滚至调用前状态。
 */

#include <algorepo/service/problem_service.h>

#include <stdio.h>
#include <string.h>

#include <algorepo/utils/utils_compat.h>
#include <algorepo/utils/utils_time.h>
#include <algorepo/utils/utils_fs.h>
#include <algorepo/utils/utils_str.h>

/* --- 私有函数声明 ---------------------------------------------------------- */

/* 根据题目属性生成规范的题解文件路径 */
static AlgoStatus service_make_solution_path(const RuntimeContext *ctx,
                                             Problem *p, const char *ext);

/* 通过拷贝+删除实现跨平台安全的文件重命名 */
static AlgoStatus service_rename_solution_file(const char *old_path,
                                               const char *new_path);

/* --- 公开接口实现 --------------------------------------------------------- */

/* 添加新题目 */
AlgoStatus service_add_problem(RuntimeContext *ctx, Problem *p,
                               const char *source_file) {
    if (!ctx || !p) return ALGO_ERR_ARG;

    /* 校验必填字段 */
    if (p->title[0] == '\0') {
        return ALGO_ERR_REQUIRED;
    }

    if (!problem_difficulty_valid(p->diff)) {
        return ALGO_ERR_INVALID_ENUM;
    }

    if (!problem_mastery_valid(p->mastery)) {
        return ALGO_ERR_INVALID_ENUM;
    }

    // 自动填充元数据
    p->created_at = utils_get_now_timestamp();
    p->id = ctx->pcol.next_id;

    // 从源文件路径提取扩展名，用于生成题解文件路径
    const char *ext =
        (source_file && source_file[0]) ? utils_get_file_ext(source_file)
                                        : ".c";
    if (ext[0] == '\0') ext = ".c";

    AlgoStatus status = service_make_solution_path(ctx, p, ext);
    if (status != ALGO_SUCCESS) {
        return status;
    }

    // 将题目加入题库
    status = pcol_add(&ctx->pcol, p);
    if (status != ALGO_SUCCESS) {
        return status;
    }

    // 拷贝源文件到题解目录（可选）
    if (source_file && source_file[0]) {
        status = utils_copy_file(source_file, p->solution_path);
        if (status != ALGO_SUCCESS) {
            // 文件拷贝失败，回滚已添加的题目记录
            pcol_remove_by_id(&ctx->pcol, p->id);
            return status;
        }
    }

    return ALGO_SUCCESS;
}

/* 删除题目 */
AlgoStatus service_remove_problem(RuntimeContext *ctx, int id, bool keep_file) {
    if (!ctx) return ALGO_ERR_ARG;

    Problem *p = pcol_get_by_id(&ctx->pcol, id);
    if (!p) return ALGO_ERR_NOT_FOUND;

    // 删除磁盘上的源文件（除非指定保留）
    if (!keep_file && p->solution_path[0] != '\0') {
        utils_compat_remove_file(p->solution_path);
    }

    return pcol_remove_by_id(&ctx->pcol, id);
}

/* 更新掌握程度 */
AlgoStatus service_update_mastery(RuntimeContext *ctx, int id, Mastery mastery) {
    if (!ctx) return ALGO_ERR_ARG;
    if (!problem_mastery_valid(mastery)) {
        return ALGO_ERR_INVALID_ENUM;
    }

    Problem *p = pcol_get_by_id(&ctx->pcol, id);
    if (!p) return ALGO_ERR_NOT_FOUND;

    return problem_set_mastery(p, mastery);
}

/* 更新难度 */
AlgoStatus service_update_difficulty(RuntimeContext *ctx, int id,
                                     Difficulty diff) {
    if (!ctx) return ALGO_ERR_ARG;
    if (!problem_difficulty_valid(diff)) {
        return ALGO_ERR_INVALID_ENUM;
    }

    Problem *p = pcol_get_by_id(&ctx->pcol, id);
    if (!p) return ALGO_ERR_NOT_FOUND;

    return problem_set_difficulty(p, diff);
}

/* 更新标题（同时更新物理路径并重命名磁盘文件） */
AlgoStatus service_update_title(RuntimeContext *ctx, int id,
                                const char *title) {
    if (!ctx) return ALGO_ERR_ARG;
    if (!title || title[0] == '\0') {
        return ALGO_ERR_REQUIRED;
    }

    Problem *p = pcol_get_by_id(&ctx->pcol, id);
    if (!p) return ALGO_ERR_NOT_FOUND;

    // 保存旧标题与旧路径用于可能的回滚
    char old_title[ALGO_MAX_TITLE_LEN];
    char old_path[ALGO_MAX_PATH_LEN];
    utils_strcpy_safe(old_title, p->title, sizeof(old_title));
    utils_strcpy_safe(old_path, p->solution_path, sizeof(old_path));

    // 更新标题
    utils_strcpy_safe(p->title, title, ALGO_MAX_TITLE_LEN);

    // 从旧路径中提取扩展名，避免读取即将被覆写的 p->solution_path
    const char *ext = utils_get_file_ext(old_path);
    if (ext[0] == '\0') ext = ".c";

    // 基于新标题重新生成物理路径
    AlgoStatus status = service_make_solution_path(ctx, p, ext);
    if (status != ALGO_SUCCESS) {
        // 回滚标题
        utils_strcpy_safe(p->title, old_title, sizeof(p->title));
        return status;
    }

    // 将磁盘上的旧文件迁移至新路径
    if (old_path[0] != '\0' && utils_file_exists(old_path)) {
        status = service_rename_solution_file(old_path, p->solution_path);
        if (status != ALGO_SUCCESS) {
            /* 回滚标题与路径 */
            utils_strcpy_safe(p->title, old_title, sizeof(p->title));
            utils_strcpy_safe(p->solution_path, old_path,
                              sizeof(p->solution_path));
            return status;
        }
    }

    return ALGO_SUCCESS;
}

/* 更新备注 */
AlgoStatus service_update_notes(RuntimeContext *ctx, int id,
                                const char *notes) {
    if (!ctx || !notes) return ALGO_ERR_ARG;

    Problem *p = pcol_get_by_id(&ctx->pcol, id);
    if (!p) return ALGO_ERR_NOT_FOUND;

    utils_strcpy_safe(p->notes, notes, ALGO_MAX_NOTES_LEN);
    return ALGO_SUCCESS;
}

/* --- 私有函数实现 --------------------------------------------------------- */

/* 生成规范的 Solution 物理路径 */
static AlgoStatus service_make_solution_path(const RuntimeContext *ctx,
                                             Problem *p, const char *ext) {
    if (!ctx || !p || !ext) return ALGO_ERR_ARG;

    char slug[128] = {0};
    utils_slugify(p->title, slug, sizeof(slug));

    // solutions_path 由 runtime_load 缓存，此处直接使用
    int written = snprintf(p->solution_path, sizeof(p->solution_path),
                           "%s/%03d_%s%s", ctx->solutions_path, p->id, slug,
                           ext);

    if (written < 0 || (size_t)written >= sizeof(p->solution_path)) {
        return ALGO_ERR_RANGE;
    }

    return ALGO_SUCCESS;
}

/* 通过拷贝+删除实现跨平台安全的文件重命名 */
static AlgoStatus service_rename_solution_file(const char *old_path,
                                               const char *new_path) {
    if (!old_path || !new_path) return ALGO_ERR_ARG;
    if (strcmp(old_path, new_path) == 0) return ALGO_SUCCESS;

    // 先拷贝内容到新路径（Windows 下跨目录 rename 可能失败）
    AlgoStatus st = utils_copy_file(old_path, new_path);
    if (st != ALGO_SUCCESS) return st;

    // 拷贝成功后安全删除旧文件
    utils_compat_remove_file(old_path);
    return ALGO_SUCCESS;
}
