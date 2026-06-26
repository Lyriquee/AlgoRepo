/**
 * @file runtime.h
 * @brief 应用程序运行时上下文与生命周期管理。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include <algorepo/config/config.h>
#include <algorepo/core/problem_collection.h>


/* --- RuntimeContext 结构体 ------------------------------------------------- */

/**
 * @brief 应用程序运行时上下文。
 *
 * 聚合配置、题库和路径状态，贯穿整个命令生命周期。
 */
typedef struct {
    bool global_mode;                        ///< 全局模式标志
    bool workspace_loaded;                   ///< 工作区装载状态
    bool no_color;                           ///< 禁用彩色输出
    bool is_repl_active;                     ///< REPL 交互模式激活

    char workspace_root[ALGO_MAX_PATH_LEN];  ///< 工作区根路径
    char home_dir[ALGO_MAX_PATH_LEN];        ///< 用户家目录

    char db_path[ALGO_MAX_PATH_LEN];         ///< 数据库文件完整路径
    char solutions_path[ALGO_MAX_PATH_LEN];  ///< 题解目录完整路径
    char config_path[ALGO_MAX_PATH_LEN];     ///< 配置文件完整路径

    AlgoConfig        config;                ///< 当前系统配置
    ProblemCollection pcol;                  ///< 题库实例
} RuntimeContext;


/* --- 生命周期 --------------------------------------------------------------- */

/**
 * @brief 初始化运行时上下文。
 *
 * 根据 global_mode 从全局目录或本地工作区解析路径。
 *
 * @param[out] ctx  待初始化的运行时上下文。
 * @return
 * - ALGO_SUCCESS      初始化成功。
 * - ALGO_ERR_MEM      内存分配失败。
 */
AlgoStatus runtime_init(RuntimeContext *ctx, bool global_mode);

/**
 * @brief 装载运行时环境。
 *
 * 依次装载配置文件与数据库，就绪后标记 workspace_loaded。
 *
 * @return
 * - ALGO_SUCCESS      装载成功。
 * - ALGO_ERR_FILE     配置文件或数据库文件未找到。
 * - ALGO_ERR_MEM      内存分配失败。
 */
AlgoStatus runtime_load(RuntimeContext *ctx);

/**
 * @brief 释放运行时持有的资源。
 *
 * 释放题库内存并清空上下文字段。
 *
 * @return
 * - ALGO_SUCCESS      释放成功。
 */
AlgoStatus runtime_destroy(RuntimeContext *ctx);


/* --- 持久化 ----------------------------------------------------------------- */

/**
 * @brief 立即将当前数据库持久化落盘。
 *
 * 用于变更后避免进程崩溃导致数据丢失。
 *
 * @return
 * - ALGO_SUCCESS      保存成功。
 * - ALGO_ERR_FILE     无法打开或创建数据库文件。
 * - ALGO_ERR_IO       写入过程中发生 I/O 错误。
 */
AlgoStatus runtime_save_db(RuntimeContext *ctx);


/* --- 工作区 ----------------------------------------------------------------- */

/**
 * @brief 在给定路径创建工作区目录结构。
 *
 * 创建 .algorepo 目录及其子目录，随后生成初始配置文件。
 *
 * @return
 * - ALGO_SUCCESS      创建成功。
 * - ALGO_ERR_FILE     目录创建失败或路径不可写。
 * - ALGO_ERR_IO       写入配置文件时发生 I/O 错误。
 */
AlgoStatus runtime_create_workspace(RuntimeContext *ctx, const char *path);

/**
 * @brief 刷新全部路径缓存。
 *
 * 根据当前 global_mode、workspace_root 和 config 重新解析 db_path、
 * solutions_path 和 config_path。在切换全局/本地模式后必须调用。
 */
void runtime_refresh_path_cache(RuntimeContext *ctx);
