/**
 * @file problem_query.c
 * @brief 题目查询服务实现：按难度/掌握度/标签/关键字筛选及统计。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 *
 * 提供对题库的多维度查询与聚合统计。所有查询函数线性遍历题库，
 * 将匹配结果追加到调用方提供的 result 集合中。
 */

#include <algorepo/service/problem_query.h>

#include <ctype.h>
#include <string.h>

#include <algorepo/utils/utils_str.h>

/* --- 私有函数声明 --------------------------------------------------------- */

/* 向结果集合中追加一道题目（保留原始 ID） */
static size_t query_append(ProblemCollection *result, const Problem *p);

/* 大小写不敏感子串查找（逐个字符 tolower 比较） */
static char *stristr(const char *haystack, const char *needle);


/* --- 公开接口实现 --------------------------------------------------------- */

/* 根据指定的难度等级筛选匹配的题目 */
size_t query_by_difficulty(const RuntimeContext *ctx, Difficulty diff,
                           ProblemCollection *result) {
    if (!ctx || !ctx->workspace_loaded || !result) return 0;

    size_t count = 0;
    const ProblemCollection *pcol = &ctx->pcol;

    for (size_t i = 0; i < pcol->size; ++i) {
        if (pcol->items[i].diff == diff) {
            count += query_append(result, &pcol->items[i]);
        }
    }
    return count;
}

/* 根据指定的掌握熟练度筛选匹配的题目 */
size_t query_by_mastery(const RuntimeContext *ctx, Mastery mastery,
                        ProblemCollection *result) {
    if (!ctx || !ctx->workspace_loaded || !result) return 0;

    size_t count = 0;
    const ProblemCollection *pcol = &ctx->pcol;

    for (size_t i = 0; i < pcol->size; ++i) {
        if (pcol->items[i].mastery == mastery) {
            count += query_append(result, &pcol->items[i]);
        }
    }
    return count;
}

/* 根据标签内容精确查找匹配的题目 */
size_t query_by_tag(const RuntimeContext *ctx, const char *tag,
                    ProblemCollection *result) {
    if (!ctx || !ctx->workspace_loaded || !tag || !result) return 0;

    size_t count = 0;
    const ProblemCollection *pcol = &ctx->pcol;

    for (size_t i = 0; i < pcol->size; ++i) {
        if (problem_has_tag(&pcol->items[i], tag)) {
            count += query_append(result, &pcol->items[i]);
        }
    }
    return count;
}

/* 模糊搜索题目标题或描述中包含关键字的题目 */
size_t query_by_keyword(const RuntimeContext *ctx, const char *keyword,
                        ProblemCollection *result) {
    if (!ctx || !ctx->workspace_loaded || !keyword || !result) return 0;

    size_t count = 0;
    const ProblemCollection *pcol = &ctx->pcol;

    for (size_t i = 0; i < pcol->size; ++i) {
        const Problem *p = &pcol->items[i];

        // 关键字同时作用于标题和描述字段
        if (strstr(p->title, keyword) || strstr(p->desc, keyword)) {
            count += query_append(result, p);
        }
    }
    return count;
}

/* 统计当前工作区题库的各种核心量化指标 */
void query_stats(const RuntimeContext *ctx, ProblemStats *stats) {
    if (!ctx || !ctx->workspace_loaded || !stats) return;

    const ProblemCollection *pcol = &ctx->pcol;
    memset(stats, 0, sizeof(ProblemStats));
    stats->total = pcol->size;

    for (size_t i = 0; i < pcol->size; ++i) {
        const Problem *p = &pcol->items[i];

        // 分类累加各个难度的题目总数
        switch (p->diff) {
            case ALGO_DIFF_EASY:    stats->easy_count++;    break;
            case ALGO_DIFF_MEDIUM:  stats->medium_count++;  break;
            case ALGO_DIFF_HARD:    stats->hard_count++;    break;
            case ALGO_DIFF_EXTREME: stats->extreme_count++; break;
            default: break;
        }

        // 只要达到 MASTERED 或 EXPERT 级别即视为已攻克
        if (p->mastery == ALGO_MASTERY_MASTERED ||
            p->mastery == ALGO_MASTERY_EXPERT) {
            stats->mastered_count++;
        }
    }
}


/* --- 关键词匹配 ----------------------------------------------------------- */

/* 大小写敏感关键词匹配 */
bool query_matches_keyword(const Problem *p, const char *keyword) {
    if (utils_str_empty(keyword)) return true;

    if (strstr(p->title, keyword)         != NULL) return true;
    if (strstr(p->desc, keyword)          != NULL) return true;
    if (strstr(p->notes, keyword)         != NULL) return true;
    if (strstr(p->solution_path, keyword) != NULL) return true;

    for (size_t i = 0; i < p->tag_count; i++) {
        if (strstr(p->tags[i], keyword) != NULL) return true;
    }
    return false;
}

// 大小写不敏感关键词匹配
bool query_matches_keyword_ic(const Problem *p, const char *keyword) {
    if (utils_str_empty(keyword)) return true;

    if (stristr(p->title, keyword)         != NULL) return true;
    if (stristr(p->desc, keyword)          != NULL) return true;
    if (stristr(p->notes, keyword)         != NULL) return true;
    if (stristr(p->solution_path, keyword) != NULL) return true;

    for (size_t i = 0; i < p->tag_count; i++) {
        if (stristr(p->tags[i], keyword) != NULL) return true;
    }
    return false;
}

/* 按字段限定范围的关键词匹配 */
bool query_matches_keyword_field(const Problem *p, const char *keyword,
                                 bool title_only, bool desc_only,
                                 bool ignore_case) {
    if (utils_str_empty(keyword)) return true;

    if (title_only) {
        return ignore_case ? (stristr(p->title, keyword) != NULL)
                           : (strstr(p->title, keyword) != NULL);
    }

    if (desc_only) {
        if (ignore_case) {
            return stristr(p->desc, keyword)  != NULL ||
                   stristr(p->notes, keyword) != NULL;
        }
        return strstr(p->desc, keyword)  != NULL ||
               strstr(p->notes, keyword) != NULL;
    }

    // 未限定字段时搜索全部字段
    return ignore_case ? query_matches_keyword_ic(p, keyword)
                       : query_matches_keyword(p, keyword);
}


/* --- 私有函数实现 --------------------------------------------------------- */

/* 向结果集合中追加一道题目。
 *
 * pcol_add 会自动分配新 ID，追加后显式恢复为原始 ID 以保证
 * 查询结果中的题目 ID 与源题库一致。 */
static size_t query_append(ProblemCollection *result, const Problem *p) {
    AlgoStatus st = pcol_add(result, p);
    if (st != ALGO_SUCCESS) return 0;

    /* 恢复原始 ID（pcol_add 会分配新 ID） */
    result->items[result->size - 1].id = p->id;
    return 1;
}

/* 大小写不敏感子串查找。
 *
 * 使用 tolower + unsigned char 逐字符比较，避免扩展 ASCII /
 * UTF-8 首字节的符号扩展问题。 */
static char *stristr(const char *haystack, const char *needle) {
    if (!haystack || !needle) return NULL;

    size_t needle_len = strlen(needle);
    if (needle_len == 0) return (char *)haystack;

    unsigned char first = (unsigned char)tolower((unsigned char)*needle);

    for (; *haystack; haystack++) {
        if ((unsigned char)tolower((unsigned char)*haystack) != first) {
            continue;
        }

        // 首字符匹配，检查剩余字符
        bool match = true;
        for (size_t i = 1; i < needle_len; i++) {
            if (haystack[i] == '\0' ||
                tolower((unsigned char)haystack[i]) !=
                    tolower((unsigned char)needle[i])) {
                match = false;
                break;
            }
        }
        if (match) return (char *)haystack;
    }
    return NULL;
}
