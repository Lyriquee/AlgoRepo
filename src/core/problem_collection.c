/**
 * @file problem_collection.c
 * @brief 动态数组题库存储：添加、查询、删除、排序与生命周期管理。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include <algorepo/core/problem_collection.h>

#include <stdlib.h>
#include <string.h>


/* --- 私有函数声明 --------------------------------------------------------------- */

static int compare_problem_id(const void *lhs, const void *rhs);


/* --- 公开接口实现 --------------------------------------------------------------- */

/* 初始化题库结构体，清零并设置 next_id 起始值 */
AlgoStatus pcol_init(ProblemCollection *pcol) {
    if (!pcol) return ALGO_ERR_ARG;

    pcol->items    = NULL;
    pcol->size     = 0;
    pcol->capacity = 0;
    pcol->next_id  = 1;

    return ALGO_SUCCESS;
}

/* 为题库预分配指定容量的内存空间 */
AlgoStatus pcol_reserve(ProblemCollection *pcol, size_t capacity) {
    if (!pcol) return ALGO_ERR_ARG;

    if (capacity <= pcol->capacity) return ALGO_SUCCESS;

    Problem *new_items = realloc(pcol->items, capacity * sizeof(Problem));

    if (!new_items) return ALGO_ERR_MEM;

    pcol->items    = new_items;
    pcol->capacity = capacity;

    return ALGO_SUCCESS;
}

/* 向题库添加一个题目副本，自动分配递增 ID */
AlgoStatus pcol_add(ProblemCollection *pcol, const Problem *p) {
    if (!pcol || !p) return ALGO_ERR_ARG;

    if (pcol->size >= pcol->capacity) {
        size_t new_capacity =
            (pcol->capacity == 0)
                ? PCOL_INITIAL_CAPACITY
                : pcol->capacity * 2;

        AlgoStatus status = pcol_reserve(pcol, new_capacity);

        if (status != ALGO_SUCCESS) return status;
    }

    Problem copy = *p;
    copy.id      = pcol->next_id++;

    pcol->items[pcol->size++] = copy;

    return ALGO_SUCCESS;
}

/* 根据下标获取题目指针，越界返回 NULL */
Problem *pcol_get(const ProblemCollection *pcol, size_t index) {
    if (!pcol) return NULL;

    if (index >= pcol->size) return NULL;

    return &pcol->items[index];
}

/* 根据 ID 查找题目，未找到返回 NULL */
Problem *pcol_get_by_id(const ProblemCollection *pcol, int id) {
    if (!pcol) return NULL;

    for (size_t i = 0; i < pcol->size; ++i) {
        if (pcol->items[i].id == id) {
            return &pcol->items[i];
        }
    }

    return NULL;
}

/* 根据 ID 删除题目，后续元素前移，未找到返回 ALGO_ERR_NOT_FOUND */
AlgoStatus pcol_remove_by_id(ProblemCollection *pcol, int id) {
    if (!pcol) return ALGO_ERR_ARG;

    for (size_t i = 0; i < pcol->size; ++i) {
        if (pcol->items[i].id == id) {
            memmove(&pcol->items[i], &pcol->items[i + 1],
                    (pcol->size - i - 1) * sizeof(Problem));

            pcol->size--;

            return ALGO_SUCCESS;
        }
    }

    return ALGO_ERR_NOT_FOUND;
}

/* 按 ID 升序排列题库中所有题目 */
void pcol_sort_by_id(ProblemCollection *pcol) {
    if (!pcol || pcol->size < 2) return;

    qsort(pcol->items, pcol->size, sizeof(Problem), compare_problem_id);
}

/* 清空题库中所有数据，保留已分配内存，重置 next_id */
void pcol_clear(ProblemCollection *pcol) {
    if (!pcol) return;

    pcol->size    = 0;
    pcol->next_id = 1;
}

/* 释放题库占用的所有内存并重置为初始状态 */
void pcol_free(ProblemCollection *pcol) {
    if (!pcol) return;

    free(pcol->items);

    pcol->items    = NULL;
    pcol->size     = 0;
    pcol->capacity = 0;
    pcol->next_id  = 1;
}


/* --- 私有函数实现 --------------------------------------------------------------- */

/* qsort 比较回调：按 Problem.id 升序 */
static int compare_problem_id(const void *lhs, const void *rhs) {
    const Problem *a = (const Problem *)lhs;
    const Problem *b = (const Problem *)rhs;

    return a->id - b->id;
}
