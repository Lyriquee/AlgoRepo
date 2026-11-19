/**
 * @file command.c
 * @brief CLI 命令注册表实现。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include "command.h"

#include <string.h>

#include <algorepo/storage/storage_file.h>
#include <algorepo/console/console_ui.h>

#include "commands/command_init.h"
#include "commands/command_add.h"
#include "commands/command_rm.h"
#include "commands/command_list.h"
#include "commands/command_show.h"
#include "commands/command_edit.h"
#include "commands/command_tag.h"
#include "commands/command_search.h"
#include "commands/command_stats.h"
#include "commands/command_config.h"
#include "commands/command_help.h"
#include "repl/repl_cmd.h"


/* --- 私有函数声明 --------------------------------------------------------- */

static const Command *command_find_in(const Command *table, int count,
                                      const char *name);


/* --- 命令注册表 ---------------------------------------------------------- */

/**
 * @brief 核心业务命令表（直出模式和 REPL 均可用）。
 */
static const Command CORE_COMMANDS[] = {
    {"init",    "i",   false, cmd_init,    help_init,    "初始化工作区"},
    {"add",     "a",   true,  cmd_add,     help_add,     "添加新题目"},
    {"rm",      "r",   true,  cmd_rm,      help_rm,      "删除指定 ID 的题目"},
    {"list",    "ls",  true,  cmd_list,    help_list,    "列出题目并支持筛选"},
    {"show",    "sh",  true,  cmd_show,    help_show,    "查看单题详情"},
    {"edit",    "e",   true,  cmd_edit,    help_edit,    "编辑题目元数据"},
    {"tag",     "tg",  true,  cmd_tag,     help_tag,     "管理题目标签"},
    {"search",  "s",   true,  cmd_search,  help_search,  "关键词搜索题目"},
    {"stats",   "st",  true,  cmd_stats,   help_stats,   "显示题库统计信息"},
    {"config",  "cfg", true,  cmd_config,  help_config,  "管理工具配置"},
    {"help",    "h",   false, cmd_help,    help_help,    "显示帮助信息"},
    {"version", "v",   false, cmd_version, help_version, "显示版本信息"},
};

static const int CORE_COUNT =
    (int)(sizeof(CORE_COMMANDS) / sizeof(CORE_COMMANDS[0]));

/**
 * @brief REPL 专属命令表（仅交互式 Shell 内可用）。
 */
static const Command REPL_COMMANDS[] = {
    {"global", ":g", false, cmd_global, help_global, "切换至全局题库模式"},
    {"local",  ":l", false, cmd_local,  help_local,  "切换至本地工作区模式"},
    {"save",   ":w", true,  cmd_save,   help_save,   "手动将内存数据持久化落盘"},
    {"clear",  ":c", false, cmd_clear,  help_clear,  "清除终端屏幕显示"},
};

static const int REPL_COUNT =
    (int)(sizeof(REPL_COMMANDS) / sizeof(REPL_COMMANDS[0]));


/* --- 命令查找 ------------------------------------------------------------- */

/* 在 CORE + REPL 两表中查找（REPL 调度用） */
const Command *command_find(const char *name) {
    const Command *c = command_find_in(CORE_COMMANDS, CORE_COUNT, name);
    return c ? c : command_find_in(REPL_COMMANDS, REPL_COUNT, name);
}

/* 仅在 CORE 表中查找（直出模式用） */
const Command *command_find_core(const char *name) {
    return command_find_in(CORE_COMMANDS, CORE_COUNT, name);
}

/* 仅在 REPL 表中查找 */
const Command *command_find_repl(const char *name) {
    return command_find_in(REPL_COMMANDS, REPL_COUNT, name);
}

/* 判断命令是否属于 REPL 专属 */
bool command_is_repl_only(const char *name) {
    return command_find_in(REPL_COMMANDS, REPL_COUNT, name) != NULL;
}


/* --- 命令表访问器 ---------------------------------------------------------- */

/* 获取 CORE 表条目数 */
int command_core_count(void) {
    return CORE_COUNT;
}

/* 按索引获取 CORE 命令描述符 */
const Command *command_core_get(int index) {
    if (index < 0 || index >= CORE_COUNT) return NULL;
    return &CORE_COMMANDS[index];
}

/* 获取 REPL 表条目数 */
int command_repl_count(void) {
    return REPL_COUNT;
}

/* 按索引获取 REPL 命令描述符 */
const Command *command_repl_get(int index) {
    if (index < 0 || index >= REPL_COUNT) return NULL;
    return &REPL_COMMANDS[index];
}


/* --- 私有函数实现 --------------------------------------------------------- */

/* 在单个表中按名称或别名查找 */
static const Command *command_find_in(const Command *table, int count,
                                      const char *name) {
    if (!name) return NULL;

    for (int i = 0; i < count; i++) {
        if (strcmp(name, table[i].name) == 0 ||
            (table[i].alias && strcmp(name, table[i].alias) == 0))
            return &table[i];
    }

    return NULL;
}
