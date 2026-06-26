/**
 * @file console_ui.h
 * @brief 控制台彩色输出、日志、格式化打印与用户交互输入。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include <algorepo/core/problem_types.h>

/* --- Compiler Attributes --------------------------------------------------- */

#if defined(__has_attribute)
#if __has_attribute(format)
#define ALGO_PRINTF_LIKE(fmt_idx, first_arg) \
    __attribute__((__format__(__printf__, fmt_idx, first_arg)))
#else
#define ALGO_PRINTF_LIKE(fmt_idx, first_arg)
#endif
#elif defined(_MSC_VER)
#define ALGO_PRINTF_LIKE(fmt_idx, first_arg)
#else
#define ALGO_PRINTF_LIKE(fmt_idx, first_arg)
#endif


/* --- ConsoleLogLevel Enum -------------------------------------------------- */

/**
 * @brief 控制台日志输出的严重级别。
 *
 * 影响日志消息的输出流向（stdout/stderr）以及对应的 ANSI 颜色。
 */
typedef enum {
    CONSOLE_LOG_FATAL = 0,  ///< 致命错误，输出至 stderr
    CONSOLE_LOG_ERROR,      ///< 一般错误，输出至 stderr
    CONSOLE_LOG_WARN,       ///< 警告，输出至 stderr
    CONSOLE_LOG_INFO,       ///< 信息，输出至 stdout
    CONSOLE_LOG_SUCCESS     ///< 操作成功，输出至 stdout
} ConsoleLogLevel;


/* --- 颜色配置 -------------------------------------------------------------- */

/**
 * @brief 启用或禁用 ANSI 彩色输出。
 *
 * 等效于将颜色模式设为 "always" 或 "never"。
 */
void console_set_color_enabled(bool enabled);

/**
 * @brief 设置颜色输出模式。
 *
 * @param mode "auto" 自动检测终端、"never" 禁用、"always" 强制启用。
 *             为 NULL 时无操作。
 */
void console_set_color_mode(const char *mode);

/**
 * @brief 获取当前颜色模式字符串。
 *
 * @return "auto"、"never" 或 "always"。
 */
const char *console_get_color_mode(void);

/**
 * @brief 查询当前是否应输出 ANSI 颜色。
 *
 * 供视图层在自行包装颜色时判断，避免在管道/重定向场景下输出转义序列。
 *
 * @return true 若当前应输出 ANSI 颜色。
 */
bool console_should_use_color(void);


/* --- 日志函数 -------------------------------------------------------------- */

/**
 * @brief 打印信息级别日志消息。
 */
void console_log_info(const char *fmt, ...) ALGO_PRINTF_LIKE(1, 2);

/**
 * @brief 打印成功级别日志消息。
 */
void console_log_success(const char *fmt, ...) ALGO_PRINTF_LIKE(1, 2);

/**
 * @brief 打印警告级别日志消息。
 */
void console_log_warn(const char *fmt, ...) ALGO_PRINTF_LIKE(1, 2);

/**
 * @brief 打印错误级别日志消息。
 */
void console_log_error(const char *fmt, ...) ALGO_PRINTF_LIKE(1, 2);

/**
 * @brief 打印致命错误级别日志消息。
 */
void console_log_fatal(const char *fmt, ...) ALGO_PRINTF_LIKE(1, 2);


/* --- ConsoleColorRole ------------------------------------------------------- */

/**
 * @brief 语义颜色角色枚举。
 *
 * 视图层通过角色而非硬编码 ANSI 码请求颜色，由 console_ui 统一映射到终端转义序列。
 * 当终端不支持颜色或 --no-color 时，所有角色输出纯文本。
 */
typedef enum {
    COLOR_ROLE_PRIMARY,      ///< 主色 — 选项名、键名、字段标签
    COLOR_ROLE_SECONDARY,    ///< 灰色 — 别名、辅助信息
    COLOR_ROLE_MUTED,        ///< 暗灰 — 行号、弱化文本
    COLOR_ROLE_ACCENT,       ///< 强调色 — 命令名、强调项
    COLOR_ROLE_SUCCESS,      ///< 成功状态
    COLOR_ROLE_WARNING,      ///< 警告状态
    COLOR_ROLE_ERROR,        ///< 错误状态
    COLOR_ROLE_VALUE,        ///< 普通值、路径、描述正文
    COLOR_ROLE_ID,           ///< 题目 ID
    COLOR_ROLE_TITLE,        ///< 亮白 — 题目标题、分隔标题
    COLOR_ROLE_TAG,          ///< 蓝灰色 — 标签文本
    COLOR_ROLE_DATE,         ///< 灰色 — 日期文本
    COLOR_ROLE_PARAM,        ///< 参数占位符
    COLOR_ROLE_DESC,         ///< 灰色 — 描述文本
    COLOR_ROLE_EXAMPLE,      ///< 示例文本
    COLOR_ROLE_EASY,         ///< 简单难度
    COLOR_ROLE_MEDIUM,       ///< 中等难度
    COLOR_ROLE_HARD,         ///< 困难难度
    COLOR_ROLE_EXTREME,      ///< 极难难度
    COLOR_ROLE_SPECIAL,      ///< 特殊/强提醒标记
    COLOR_ROLE_DIVIDER,      ///< 灰色 — 分隔线
    COLOR_ROLE_PROMPT,       ///< 交互提示符
    COLOR_ROLE_BAR_FILL,     ///< 统计条填充
    COLOR_ROLE_BAR_EMPTY,    ///< 统计条空位
    COLOR_ROLE_MASTERY_NONE,     ///< 深灰色 — 未设置
    COLOR_ROLE_MASTERY_LEARNING, ///< 浅蓝色 — 了解
    COLOR_ROLE_MASTERY_FAMILIAR, ///< 标准蓝色 — 熟悉
    COLOR_ROLE_MASTERY_MASTERED, ///< 深蓝色 — 掌握
    COLOR_ROLE_MASTERY_EXPERT,   ///< 青色 — 精通
} ConsoleColorRole;


/* --- 格式化彩色打印 -------------------------------------------------------- */

/**
 * @brief 以指定语义角色打印格式化文本。
 *
 * 视图层首选 API。内部根据角色查表选择 ANSI 颜色，--no-color 时输出纯文本。
 */
void console_print_role(ConsoleColorRole role, const char *fmt, ...)
    ALGO_PRINTF_LIKE(2, 3);

/**
 * @brief 打印普通无色文本。
 *
 * 用于视图层输出空格、换行和不需要语义颜色的固定文本，避免直接散落 printf。
 */
void console_print_text(const char *fmt, ...) ALGO_PRINTF_LIKE(1, 2);

/**
 * @brief 打印一个换行。
 */
void console_print_newline(void);

/**
 * @brief 打印 count 个空格。
 */
void console_print_spaces(int count);

/**
 * @brief 仅输出指定角色的 ANSI 起始序列（不输出文本）。
 *
 * 用于需要精细控制输出格式的场景（如难度标签和掌握度标签）。
 * 通常与 console_reset_color 配对使用。
 */
void console_begin_role(ConsoleColorRole role);

/**
 * @brief 输出 ANSI 重置序列（仅当颜色启用时）。
 */
void console_reset_color(void);

/**
 * @brief 以指定 ANSI 颜色打印格式化文本。
 *
 * 颜色为 NULL 或空字符串时不附加任何转义序列。
 */
void console_printf_color(const char *color, const char *fmt, ...)
    ALGO_PRINTF_LIKE(2, 3);

/**
 * @brief 以亮青色打印主要文本（选项名称、键值对键、字段标签）。
 */
void console_print_primary(const char *fmt, ...) ALGO_PRINTF_LIKE(1, 2);

/**
 * @brief 以灰色打印次要文本（别名、辅助信息）。
 */
void console_print_secondary(const char *fmt, ...) ALGO_PRINTF_LIKE(1, 2);

/**
 * @brief 以青色打印弱化文本（行号、元信息）。
 */
void console_print_muted(const char *fmt, ...) ALGO_PRINTF_LIKE(1, 2);

/**
 * @brief 以亮品红打印强调文本（命令名称、高亮项）。
 */
void console_print_accent(const char *fmt, ...) ALGO_PRINTF_LIKE(1, 2);


/* --- 用户交互输入 ---------------------------------------------------------- */

/**
 * @brief 从标准输入读取一行字符串，显示彩色提示。
 *
 * 自动去除行尾换行符。输入超出缓冲区大小时丢弃多余字符。
 *
 * @param buffer 输出缓冲区，调用方保证非 NULL 且 size > 0。
 * @param size   缓冲区大小（字节）。
 * @return
 * - ALGO_SUCCESS            读取成功。
 * - ALGO_ERR_EOF            读取到 EOF。
 * - ALGO_ERR_STR_TOO_LONG   输入超出缓冲区大小。
 * - ALGO_ERR_ARG            prompt、buffer 为 NULL 或 size 为 0。
 */
AlgoStatus console_input_string(const char *prompt, char *buffer, size_t size);

/**
 * @brief 从标准输入读取 [min, max] 范围内的整数。
 *
 * @param value 输出解析结果，调用方保证非 NULL。
 * @return
 * - ALGO_SUCCESS   读取并解析成功。
 * - ALGO_ERR_EOF   读取到 EOF。
 * - ALGO_ERR_ARG   输入格式非法（非数字或多余字符）。
 * - ALGO_ERR_RANGE 数值超出 [min, max] 范围。
 */
AlgoStatus console_input_int(const char *prompt, int *value, int min, int max);

/**
 * @brief 提示用户进行是/否确认。
 *
 * 以 'y' 或 'Y' 开头的输入视为确认，其余视为否定。
 *
 * @param confirmed 输出确认结果，调用方保证非 NULL。
 * @return
 * - ALGO_SUCCESS   读取并解析成功。
 * - ALGO_ERR_EOF   读取到 EOF。
 * - ALGO_ERR_ARG   prompt 或 confirmed 为 NULL。
 */
AlgoStatus console_confirm(const char *prompt, bool *confirmed);


/* --- 布局辅助 -------------------------------------------------------------- */

/**
 * @brief 以亮白色打印标题行并换行。
 */
void console_print_title(const char *title);

/**
 * @brief 以列对齐方式打印键值对。
 *
 * 键固定 18 字符宽度左对齐并以主色显示。
 */
void console_print_kv(const char *key, const char *value);

/**
 * @brief 打印水平分隔线（灰色，40 字符宽）。
 */
void console_print_divider(void);

/**
 * @brief 使用指定字符和宽度打印分隔线。
 */
void console_print_separator(char ch, int width);

/**
 * @brief 以亮绿色打印交互提示符。
 */
void console_print_prompt(const char *fmt, ...) ALGO_PRINTF_LIKE(1, 2);

/**
 * @brief 以亮绿色打印成功状态文本。
 */
void console_print_success(const char *fmt, ...) ALGO_PRINTF_LIKE(1, 2);

/**
 * @brief 以亮红色打印错误文本（无日志级别前缀）。
 */
void console_print_error_raw(const char *fmt, ...) ALGO_PRINTF_LIKE(1, 2);
