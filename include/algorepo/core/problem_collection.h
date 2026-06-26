/**
 * @file problem_collection.h
 * @brief 题库动态数组结构及增删查改操作。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stddef.h>

#include <algorepo/core/problem.h>


/* --- Constants ------------------------------------------------------------ */

/* 题库动态数组初始容量 */
#define PCOL_INITIAL_CAPACITY 8


/* --- ProblemCollection --------------------------------------------------- */

/**
 * @brief 题库动态数组。
 *
 * 管理 Problem 对象的增删查改，支持自动扩容和 ID 自增分配。
 */
typedef struct {
    Problem *items;      ///< 题目数组
    size_t   size;       ///< 当前元素数量
    size_t   capacity;   ///< 已分配容量
    int      next_id;    ///< 下一个分配的自增 ID
} ProblemCollection;


/* --- 生命周期 -------------------------------------------------------------- */

/**
 * @brief 初始化题库结构体。
 *
 * 分配初始容量并将字段置零。
 *
 * @return
 * - ALGO_SUCCESS   初始化成功。
 * - ALGO_ERR_MEM   内存分配失败。
 */
AlgoStatus pcol_init(ProblemCollection *pcol);

/**
 * @brief 释放题库占用的所有内存。
 *
 * 释放内部题目数组，并将所有字段置零。
 */
void pcol_free(ProblemCollection *pcol);

/**
 * @brief 清空题库中所有数据。
 *
 * 将 size 和 next_id 置零，但保留已分配的内存容量。
 */
void pcol_clear(ProblemCollection *pcol);


/* --- 容量管理 -------------------------------------------------------------- */

/**
 * @brief 为题库预分配指定容量的内存空间。
 *
 * 若当前容量已满足要求则直接返回成功，不做缩容。
 *
 * @return
 * - ALGO_SUCCESS   预分配成功或容量已满足。
 * - ALGO_ERR_MEM   内存分配失败。
 */
AlgoStatus pcol_reserve(ProblemCollection *pcol, size_t capacity);


/* --- 增删查改 -------------------------------------------------------------- */

/**
 * @brief 向题库添加一个题目副本。
 *
 * 自动分配唯一 ID，容量不足时自动扩容。
 *
 * @return
 * - ALGO_SUCCESS   添加成功。
 * - ALGO_ERR_MEM   扩容时内存分配失败。
 */
AlgoStatus pcol_add(ProblemCollection *pcol, const Problem *p);

/**
 * @brief 根据下标获取题目指针。
 *
 * @return 指向对应题目的指针，越界返回 NULL。
 */
Problem *pcol_get(const ProblemCollection *pcol, size_t index);

/**
 * @brief 根据 ID 查找题目。
 *
 * @return 指向对应题目的指针，未找到返回 NULL。
 */
Problem *pcol_get_by_id(const ProblemCollection *pcol, int id);

/**
 * @brief 根据 ID 删除题目。
 *
 * 删除后会将后续元素前移以保持数组紧凑。
 *
 * @return
 * - ALGO_SUCCESS       删除成功。
 * - ALGO_ERR_NOT_FOUND 未找到对应 ID 的题目。
 */
AlgoStatus pcol_remove_by_id(ProblemCollection *pcol, int id);


/* --- 排序 ----------------------------------------------------------------- */

/**
 * @brief 按 ID 升序排列题库中所有题目。
 */
void pcol_sort_by_id(ProblemCollection *pcol);
