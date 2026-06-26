/**
 * @file storage_file.h
 * @brief 二进制文件持久化：魔数校验、版本管理及结构体序列化。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 *
 * 定义本地二进制存储格式的文件头结构（魔数 + 版本 + 计数）以及
 * 将题库完整序列化/反序列化的接口。文件格式版本号 ALGO_FILE_VERSION
 * 在结构体布局变更时递增，旧版本文件的读取需在此处添加兼容逻辑。
 */

#pragma once

#include <stdint.h>

#include <algorepo/core/problem_types.h>
#include <algorepo/core/problem_collection.h>

/* --- Binary Format Constants ----------------------------------------------- */

/** @brief 文件格式识别魔数 "ARDB"（AlgoRepo DataBase）。 */
#define ALGO_FILE_MAGIC "ARDB"

/** @brief 当前文件格式版本号。
 *
 * 版本演进历史：
 * - v1: 初始格式
 * - v2: Problem 结构体增加 notes 字段
 * - v3: Problem 结构体增加 solution_path 与 created_at 字段
 */
#define ALGO_FILE_VERSION 3

/* --- AlgoFileHeader Struct ------------------------------------------------- */

/**
 * @brief 序列化题库的二进制文件头结构。
 *
 * 位于文件起始位置，包含魔数标识、格式版本和题目计数。
 * 读取时通过校验魔数与版本号确保文件完整性。
 */
typedef struct {
    char     magic[4];   ///< 魔数字符串 "ARDB"
    uint32_t version;    ///< 文件格式版本号
    uint32_t count;      ///< 题目数量
} AlgoFileHeader;

/* --- Binary File I/O ------------------------------------------------------- */

/**
 * @brief 将题库序列化为本地二进制文件。
 *
 * 写入流程：文件头（魔数 + 版本 + 计数）→ 题目结构体数组批量写入。
 *
 * @param pcol     待序列化的题库。
 * @param filepath 目标文件路径（UTF-8 编码）。
 * @return
 * - ALGO_SUCCESS   序列化成功。
 * - ALGO_ERR_FILE  无法打开或创建文件。
 * - ALGO_ERR_IO    写入过程中发生 I/O 错误。
 * - ALGO_ERR_ARG   参数非法（pcol 或 filepath 为 NULL）。
 */
AlgoStatus storage_file_save(const ProblemCollection *pcol,
                             const char *filepath);

/**
 * @brief 从本地二进制文件反序列化题库。
 *
 * 读取流程：文件头校验（魔数 + 版本）→ 清空题库 → 预分配容量 →
 * 逐条读取题目结构体 → 恢复自增 ID 起点。
 *
 * @param pcol     待填充的题库（调用前需已通过 pcol_init 初始化）。
 * @param filepath 源文件路径（UTF-8 编码）。
 * @return
 * - ALGO_SUCCESS       加载成功。
 * - ALGO_ERR_NOT_FOUND 文件不存在。
 * - ALGO_ERR_IO        I/O 读取错误。
 * - ALGO_ERR_EOF       文件意外结束（实际题目数不足 header.count）。
 * - ALGO_ERR_FORMAT    魔数不匹配或版本号非法。
 * - ALGO_ERR_MEM       内存分配失败。
 * - ALGO_ERR_ARG       参数非法（pcol 或 filepath 为 NULL）。
 */
AlgoStatus storage_file_load(ProblemCollection *pcol, const char *filepath);
