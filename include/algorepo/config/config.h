/**
 * @file config.h
 * @brief 核心配置数据模型及序列化、合并操作。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include <algorepo/core/problem_types.h>

/* --- Path Constants ------------------------------------------------------- */

/* 元数据目录名 */
#define ALGO_META_DIR          ".algorepo"
/* 数据库文件名 */
#define ALGO_DB_FILENAME       "repo.db"
/* 配置文件名 */
#define ALGO_CONFIG_FILENAME   "config.ini"
/* 题解源码存放目录名 */
#define ALGO_SOLUTIONS_DIR     "solutions"

/* --- AlgoConfig Struct ---------------------------------------------------- */

/**
 * @brief 核心配置数据模型。
 *
 * 支持全局配置与本地配置层级继承，未设置字段以特殊值表示。
 */
typedef struct {
    char default_language[16];           ///< 默认编程语言
    char editor[64];                     ///< 编辑器命令
    char color[8];                       ///< 终端配色方案
    int  auto_backup;                    ///< 自动备份：-1=未设置，0=关闭，1=开启
    int  backup_count;                   ///< 备份保留份数：0=未设置，>0=实际份数
    char db_path[ALGO_MAX_PATH_LEN];     ///< 数据库文件路径，空字符串=使用默认
    char solutions_path[ALGO_MAX_PATH_LEN]; ///< 题解目录路径，空字符串=使用默认
} AlgoConfig;

/* --- 生命周期 --------------------------------------------------------- */

/**
 * @brief 使用默认值填充配置结构体。
 */
void config_init(AlgoConfig *cfg);

/**
 * @brief 将所有字段置为"未设置"状态。
 *
 * 本地配置继承全局时使用此函数初始化空配置。
 */
void config_init_empty(AlgoConfig *cfg);

/* --- 校验与持久化 --------------------------------------------------- */

/**
 * @brief 校验配置字段合法性。
 *
 * 允许空值表示继承，仅对非空字段进行校验。
 *
 * @return
 * - ALGO_SUCCESS       所有已设置字段均合法。
 * - ALGO_ERR_ARG       某字段值非法。
 * - ALGO_ERR_RANGE     数值字段超出允许范围。
 */
AlgoStatus config_validate(const AlgoConfig *cfg);

/**
 * @brief 序列化配置并写入 INI 文件。
 *
 * 未设置字段输出空值，以便加载时可识别为继承。
 *
 * @return
 * - ALGO_SUCCESS   写入成功。
 * - ALGO_ERR_FILE  无法创建或打开文件。
 * - ALGO_ERR_IO    写入过程中发生 I/O 错误。
 */
AlgoStatus config_save(const AlgoConfig *cfg, const char *filepath);

/**
 * @brief 从 INI 文件反序列化配置。
 *
 * 文件不存在时返回错误且不修改 cfg。
 *
 * @return
 * - ALGO_SUCCESS   读取并解析成功。
 * - ALGO_ERR_FILE  文件不存在。
 * - ALGO_ERR_IO    读取过程中发生 I/O 错误。
 * - ALGO_ERR_FORMAT 文件格式非法。
 */
AlgoStatus config_load(AlgoConfig *cfg, const char *filepath);

/* --- 合并 --------------------------------------------------------------- */

/**
 * @brief 配置层级合并。
 *
 * 以 global 为基底，local 中非空字段覆盖对应字段，结果写入 dst。
 *
 * @param dst    输出目标，可与 local 或 global 为同一指针。
 * @param local  本地配置，优先级高于 global。
 * @param global 全局配置，作为默认基底。
 */
void config_merge(AlgoConfig *dst, const AlgoConfig *local,
                  const AlgoConfig *global);
