/**
 * @file storage.h
 * @brief 持久化存储抽象层：统一序列化与反序列化接口。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 *
 * 本模块是题库持久化的顶层抽象，调用方通过 storage_save / storage_load
 * 操作题库，无需感知底层存储后端（当前为本地二进制文件，未来可扩展为
 * 网络存储、数据库等）。接口内部完成参数校验后委托至具体实现。
 */

#pragma once

#include <algorepo/core/problem_types.h>
#include <algorepo/core/problem_collection.h>

/* --- Storage Interface ----------------------------------------------------- */

/**
 * @brief 将题库序列化到指定 URI 的持久化存储中。
 *
 * 当前实现委托给 storage_file_save，使用本地二进制文件格式。
 * 调用方无需关心底层存储机制，未来可透明替换为其他后端。
 *
 * @param pcol 待序列化的题库。
 * @param uri  目标文件路径。
 * @return
 * - ALGO_SUCCESS   保存成功。
 * - ALGO_ERR_FILE  无法打开或创建文件。
 * - ALGO_ERR_IO    写入过程中发生 I/O 错误。
 * - ALGO_ERR_ARG   参数非法。
 */
AlgoStatus storage_save(const ProblemCollection *pcol, const char *uri);

/**
 * @brief 从指定 URI 的持久化存储中反序列化题库。
 *
 * 当前实现委托给 storage_file_load，读取本地二进制文件格式。
 * 调用前需通过 pcol_init 初始化 pcol。
 *
 * @param pcol 待填充的题库（调用前需已初始化）。
 * @param uri  源文件路径。
 * @return
 * - ALGO_SUCCESS       加载成功。
 * - ALGO_ERR_NOT_FOUND 文件不存在。
 * - ALGO_ERR_FILE      文件无法打开。
 * - ALGO_ERR_IO        I/O 读取错误。
 * - ALGO_ERR_FORMAT    文件格式非法（魔数或版本不匹配）。
 * - ALGO_ERR_MEM       内存分配失败。
 * - ALGO_ERR_ARG       参数非法。
 */
AlgoStatus storage_load(ProblemCollection *pcol, const char *uri);
