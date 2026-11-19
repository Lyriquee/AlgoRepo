/**
 * @file problem.h
 * @brief 题目完整数据模型及相关操作函数。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <algorepo/core/problem_types.h>


/* --- Problem Struct ------------------------------------------------------- */

/**
 * @brief 题目完整数据模型。
 *
 * 包含题目元数据、标签、笔记以及源码路径等全部信息。
 */
typedef struct {
    // 基本信息
    int        id;                         ///< 唯一标识符
    char       title[ALGO_MAX_TITLE_LEN];  ///< 题目标题
    char       desc[ALGO_MAX_DESC_LEN];    ///< 题目描述
    Difficulty diff;                       ///< 难度等级
    Mastery    mastery;                    ///< 掌握程度

    // 标签与笔记
    size_t tag_count;                              ///< 当前标签数量
    char   tags[ALGO_MAX_TAGS][ALGO_MAX_TAG_LEN];  ///< 标签列表
    char   notes[ALGO_MAX_NOTES_LEN];              ///< 个人笔记

    // 文件与时间
    char    solution_path[ALGO_MAX_PATH_LEN];  ///< 源码文件路径
    int64_t created_at;                        ///< 创建时间（Unix 时间戳，秒）
} Problem;


/* --- 生命周期与初始化 ------------------------------------------------------------- */

/**
 * @brief 初始化题目对象为默认状态。
 *
 * 将所有字段置零，难度设为未知，掌握程度设为未知。
 *
 * @param p 待初始化的题目对象指针。
 */
void problem_init(Problem *p);


/* --- 属性校验 ----------------------------------------------------------------- */

/**
 * @brief 判断难度枚举值是否合法。
 *
 * @return true 表示合法，false 表示非法。
 */
bool problem_difficulty_valid(Difficulty diff);

/**
 * @brief 判断掌握程度枚举值是否合法。
 *
 * @return true 表示合法，false 表示非法。
 */
bool problem_mastery_valid(Mastery mastery);


/* --- 属性设置 ----------------------------------------------------------------- */

/**
 * @brief 设置题目难度。
 *
 * @param p    题目对象指针。
 * @param diff 目标难度等级，必须为合法枚举值。
 * @return
 * - ALGO_SUCCESS           设置成功。
 * - ALGO_ERR_INVALID_ENUM  传入的难度值非法。
 */
AlgoStatus problem_set_difficulty(Problem *p, Difficulty diff);

/**
 * @brief 设置题目掌握程度。
 *
 * @param p       题目对象指针。
 * @param mastery 目标掌握程度，必须为合法枚举值。
 * @return
 * - ALGO_SUCCESS           设置成功。
 * - ALGO_ERR_INVALID_ENUM  传入的掌握程度值非法。
 */
AlgoStatus problem_set_mastery(Problem *p, Mastery mastery);


/* --- 标签管理 ----------------------------------------------------------------- */

/**
 * @brief 判断题目是否包含指定标签。
 *
 * @param tag 待查找的标签字符串（精确匹配，区分大小写）。
 * @return true 表示包含该标签，false 表示不包含。
 */
bool problem_has_tag(const Problem *p, const char *tag);

/**
 * @brief 向题目添加标签。
 *
 * 标签自动去重，重复添加不会产生副作用。
 *
 * @param p   题目对象指针。
 * @param tag 待添加的标签字符串。
 * @return
 * - ALGO_SUCCESS       添加成功或标签已存在。
 * - ALGO_ERR_TAG_FULL  标签数量已达上限。
 */
AlgoStatus problem_add_tag(Problem *p, const char *tag);

/**
 * @brief 删除指定标签。
 *
 * @param p   题目对象指针。
 * @param tag 待删除的标签字符串。
 * @return
 * - ALGO_SUCCESS       删除成功。
 * - ALGO_ERR_NOT_FOUND 未找到该标签。
 */
AlgoStatus problem_remove_tag(Problem *p, const char *tag);

/**
 * @brief 解析逗号分隔的标签字符串并追加到题目。
 *
 * 字符串按逗号切分后逐个调用 problem_add_tag，自动跳过空白。
 *
 * @param p    题目对象指针。
 * @param tags 逗号分隔的标签字符串。
 * @return
 * - ALGO_SUCCESS       全部标签添加成功。
 * - ALGO_ERR_TAG_FULL  部分或全部标签因数量上限未能添加。
 */
AlgoStatus problem_parse_tags(Problem *p, const char *tags);
