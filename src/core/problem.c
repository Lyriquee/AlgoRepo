/**
 * @file problem.c
 * @brief 题目生命周期操作、标签管理与元数据设置。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include <algorepo/core/problem.h>

#include <string.h>

#include <algorepo/utils/utils_str.h>


/* --- 公开接口实现 --------------------------------------------------------- */

/* 将题目结构体初始化为零并设置默认值 */
void problem_init(Problem *p) {
    if (!p) return;

    memset(p, 0, sizeof(Problem));
    p->id      = -1;  // 临时或未分配的对象
    p->diff    = ALGO_DIFF_UNKNOWN;
    p->mastery = ALGO_MASTERY_UNKNOWN;
    p->created_at    = 0;   // 时间戳未设置
}

/* 校验难度枚举值是否在合法范围内 */
bool problem_difficulty_valid(Difficulty diff) {
    return diff >= ALGO_DIFF_UNKNOWN && diff <= ALGO_DIFF_EXTREME;
}

/* 校验掌握度枚举值是否在合法范围内 */
bool problem_mastery_valid(Mastery mastery) {
    return mastery >= ALGO_MASTERY_UNKNOWN && mastery <= ALGO_MASTERY_EXPERT;
}

/* 校验枚举值后设置题目难度 */
AlgoStatus problem_set_difficulty(Problem *p, Difficulty diff) {
    if (!p) return ALGO_ERR_ARG;

    if (!problem_difficulty_valid(diff)) {
        return ALGO_ERR_INVALID_ENUM;
    }

    p->diff = diff;

    return ALGO_SUCCESS;
}

/* 校验枚举值后设置题目掌握度 */
AlgoStatus problem_set_mastery(Problem *p, Mastery mastery) {
    if (!p) return ALGO_ERR_ARG;

    if (!problem_mastery_valid(mastery)) {
        return ALGO_ERR_INVALID_ENUM;
    }

    p->mastery = mastery;

    return ALGO_SUCCESS;
}

/* 检查题目是否包含指定标签 */
bool problem_has_tag(const Problem *p, const char *tag) {
    if (!p || !tag) return false;

    for (size_t i = 0; i < p->tag_count; i++) {
        if (strcmp(p->tags[i], tag) == 0) return true;
    }
    return false;
}

/* 向题目添加标签（幂等，自动去重）；标签满时返回 ALGO_ERR_TAG_FULL */
AlgoStatus problem_add_tag(Problem *p, const char *tag) {
    if (!p || !tag || tag[0] == '\0') return ALGO_ERR_ARG;

    if (problem_has_tag(p, tag)) return ALGO_SUCCESS;
    if (p->tag_count >= ALGO_MAX_TAGS) return ALGO_ERR_TAG_FULL;

    size_t tag_len = strlen(tag);
    if (tag_len >= ALGO_MAX_TAG_LEN) return ALGO_ERR_STR_TOO_LONG;
    memcpy(p->tags[p->tag_count], tag, tag_len);
    p->tags[p->tag_count][tag_len] = '\0';
    p->tag_count++;

    return ALGO_SUCCESS;
}

/* 根据名称删除标签；未找到返回 ALGO_ERR_NOT_FOUND */
AlgoStatus problem_remove_tag(Problem *p, const char *tag) {
    if (!p || !tag) return ALGO_ERR_ARG;

    // 记录待删除标签的下标，-1 表示未找到
    int target_idx = -1;

    // 扫描所有标签进行匹配
    for (size_t i = 0; i < p->tag_count; i++) {
        if (strcmp(p->tags[i], tag) == 0) {
            target_idx = (int)i;
            break;
        }
    }

    if (target_idx == -1) return ALGO_ERR_NOT_FOUND;

    // 将后续标签前移
    for (size_t i = (size_t)target_idx; i < p->tag_count - 1; i++) {
        memcpy(p->tags[i], p->tags[i + 1], ALGO_MAX_TAG_LEN);
    }

    // 清空最后一个槽位
    memset(p->tags[p->tag_count - 1], 0, ALGO_MAX_TAG_LEN);
    p->tag_count--;

    return ALGO_SUCCESS;
}

/* 解析逗号分隔的标签字符串并逐个追加到题目 */
AlgoStatus problem_parse_tags(Problem *p, const char *tags) {
    if (!p || !tags) return ALGO_ERR_ARG;
    char buf[256];
    AlgoStatus st = utils_strcpy_safe(buf, tags, sizeof(buf));
    if (st != ALGO_SUCCESS && st != ALGO_ERR_STR_TOO_LONG) return st;

    char *saveptr = NULL;
#ifdef _WIN32
    char *tok = strtok_s(buf, ",", &saveptr);
#else
    char *tok = strtok_r(buf, ",", &saveptr);
#endif

    while (tok) {
        utils_trim(tok);
        if (tok[0] != '\0') {
            // 尝试追加标签；容量已满时返回 ALGO_ERR_TAG_FULL
            AlgoStatus add_st = problem_add_tag(p, tok);
            if (add_st != ALGO_SUCCESS) return add_st;
        }
#ifdef _WIN32
        tok = strtok_s(NULL, ",", &saveptr);
#else
        tok = strtok_r(NULL, ",", &saveptr);
#endif
    }
    return ALGO_SUCCESS;
}
