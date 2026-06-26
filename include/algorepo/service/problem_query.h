/**
 * @file problem_query.h
 * @brief 题目查询服务：按难度/掌握度/标签/关键字筛选及统计。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 *
 * 提供对题库的多维度查询与聚合统计。所有查询函数将匹配结果追加到
 * 调用方提供的 result 集合中（调用前需已初始化），返回匹配数量。
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include <algorepo/core/problem_collection.h>
#include <algorepo/runtime/runtime.h>

/* --- ProblemStats Struct --------------------------------------------------- */

/**
 * @brief 题库聚合统计信息。
 *
 * 按难度等级分类计数，并统计已掌握题目数量（MASTERED 或 EXPERT）。
 */
typedef struct {
    size_t total;           ///< 题目总数

    size_t easy_count;      ///< 简单题目数
    size_t medium_count;    ///< 中等题目数
    size_t hard_count;      ///< 困难题目数
    size_t extreme_count;   ///< 极难题目数

    size_t mastered_count;  ///< 已掌握题目数（MASTERED 或 EXPERT）
} ProblemStats;

/* --- Query Functions ------------------------------------------------------- */

/**
 * @brief 按难度等级筛选题目。
 *
 * @param ctx    运行时上下文。
 * @param diff   目标难度等级。
 * @param result 输出结果集合（调用前需已通过 pcol_init 初始化）。
 * @return 匹配的题目数量。
 */
size_t query_by_difficulty(const RuntimeContext *ctx, Difficulty diff,
                           ProblemCollection *result);

/**
 * @brief 按掌握程度筛选题目。
 *
 * @param ctx     运行时上下文。
 * @param mastery 目标掌握程度。
 * @param result  输出结果集合（调用前需已通过 pcol_init 初始化）。
 * @return 匹配的题目数量。
 */
size_t query_by_mastery(const RuntimeContext *ctx, Mastery mastery,
                        ProblemCollection *result);

/**
 * @brief 按标签精确筛选题目。
 *
 * @param ctx    运行时上下文。
 * @param tag    目标标签字符串（精确匹配，区分大小写）。
 * @param result 输出结果集合（调用前需已通过 pcol_init 初始化）。
 * @return 匹配的题目数量。
 */
size_t query_by_tag(const RuntimeContext *ctx, const char *tag,
                    ProblemCollection *result);

/**
 * @brief 按关键字模糊搜索题目（标题和描述字段）。
 *
 * @param ctx     运行时上下文。
 * @param keyword 搜索关键字，为 NULL 或空串时返回 0。
 * @param result  输出结果集合（调用前需已通过 pcol_init 初始化）。
 * @return 匹配的题目数量。
 */
size_t query_by_keyword(const RuntimeContext *ctx, const char *keyword,
                        ProblemCollection *result);

/**
 * @brief 计算题库聚合统计信息。
 *
 * @param ctx   运行时上下文。
 * @param stats 输出统计结构体，为 NULL 时无操作。
 */
void query_stats(const RuntimeContext *ctx, ProblemStats *stats);

/* --- Keyword Matching Helpers ---------------------------------------------- */

/**
 * @brief 大小写敏感关键字匹配。
 *
 * 搜索范围：标题、描述、笔记、源文件路径、标签。
 *
 * @param p       待搜索的题目。
 * @param keyword 关键字（NULL 或空字符串视为匹配所有）。
 * @return true 若关键字匹配题目的任一字段。
 */
bool query_matches_keyword(const Problem *p, const char *keyword);

/**
 * @brief 大小写不敏感关键字匹配。
 *
 * 搜索范围同 query_matches_keyword。内部使用 tolower 逐字符比较。
 *
 * @param p       待搜索的题目。
 * @param keyword 关键字（NULL 或空字符串视为匹配所有）。
 * @return true 若关键字匹配题目的任一字段。
 */
bool query_matches_keyword_ic(const Problem *p, const char *keyword);

/**
 * @brief 按字段限定范围的关键字匹配（支持标题/描述限定及大小写控制）。
 *
 * @param p           待搜索的题目。
 * @param keyword     关键字（NULL 或空字符串视为匹配所有）。
 * @param title_only  仅搜索标题字段。
 * @param desc_only   仅搜索描述和笔记字段。
 * @param ignore_case 是否忽略大小写。
 * @return true 若关键字匹配指定字段。
 */
bool query_matches_keyword_field(const Problem *p, const char *keyword,
                                 bool title_only, bool desc_only,
                                 bool ignore_case);
