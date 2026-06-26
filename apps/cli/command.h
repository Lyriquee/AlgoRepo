/**
 * @file command.h
 * @brief CLI 命令注册表、查找接口与共用帮助打印函数。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <algorepo/runtime/runtime.h>


/* --- Command Types -------------------------------------------------------- */

/** @brief 命令处理函数签名。 */
typedef AlgoStatus (*CommandHandler)(int argc, char *argv[],
                                     RuntimeContext *ctx);

/** @brief 帮助信息打印函数签名。 */
typedef void (*HelpHandler)(void);


/* --- Command Struct -------------------------------------------------------- */

/**
 * @brief CLI 命令描述符。
 *
 * 每个命令包含名称、别名、工作区依赖标志、处理函数和帮助函数。
 */
typedef struct {
    const char    *name;               ///< 主命令名
    const char    *alias;              ///< 短别名
    bool           require_workspace;  ///< 是否需要已加载的工作区
    CommandHandler handler;            ///< 命令处理回调
    HelpHandler    help_fn;            ///< 帮助信息打印回调
    const char    *desc;               ///< 命令描述
} Command;


/* --- Command Lookup -------------------------------------------------------- */

/**
 * @brief 在两个命令表中查找命令（先 CORE 再 REPL）。
 *
 * @return 匹配的命令描述符指针，未找到返回 NULL。
 */
const Command *command_find(const char *name);

/**
 * @brief 仅在 CORE 表中查找命令（直出模式使用）。
 *
 * @return 匹配的命令描述符指针，未找到返回 NULL。
 */
const Command *command_find_core(const char *name);

/**
 * @brief 仅在 REPL 表中查找命令。
 *
 * @return 匹配的命令描述符指针，未找到返回 NULL。
 */
const Command *command_find_repl(const char *name);

/**
 * @brief 判断命令名是否属于 REPL 专属表。
 *
 * @return true 若该命令仅在 REPL 模式下可用。
 */
bool command_is_repl_only(const char *name);


/* --- Command Table Accessors ---------------------------------------------- */

/**
 * @brief 获取 CORE 命令表条目数。
 *
 * @return CORE 命令表中已注册的命令数量。
 */
int command_core_count(void);

/**
 * @brief 按索引获取 CORE 命令描述符。
 *
 * @return 命令描述符指针，索引越界返回 NULL。
 */
const Command *command_core_get(int index);

/**
 * @brief 获取 REPL 命令表条目数。
 *
 * @return REPL 命令表中已注册的命令数量。
 */
int command_repl_count(void);

/**
 * @brief 按索引获取 REPL 命令描述符。
 *
 * @return 命令描述符指针，索引越界返回 NULL。
 */
const Command *command_repl_get(int index);
