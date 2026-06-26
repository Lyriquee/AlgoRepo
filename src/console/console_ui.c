/**
 * @file console_ui.c
 * @brief 控制台UI实现。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include <algorepo/console/console_ui.h>

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorepo/utils/utils_compat.h>

/* --- 内部类型与状态 --------------------------------------------------------- */

/* --- ANSI Color Codes ------------------------------------------------------ */

#define CONSOLE_COLOR_RESET          "\x1b[0m"
#define CONSOLE_COLOR_RED            "\x1b[0;31m"
#define CONSOLE_COLOR_BRIGHT_RED     "\x1b[1;31m"
#define CONSOLE_COLOR_GREEN          "\x1b[0;32m"
#define CONSOLE_COLOR_BRIGHT_GREEN   "\x1b[1;32m"
#define CONSOLE_COLOR_YELLOW         "\x1b[0;33m"
#define CONSOLE_COLOR_BRIGHT_YELLOW  "\x1b[1;33m"
#define CONSOLE_COLOR_BLUE           "\x1b[0;34m"
#define CONSOLE_COLOR_BRIGHT_BLUE    "\x1b[1;34m"
#define CONSOLE_COLOR_MAGENTA        "\x1b[0;35m"
#define CONSOLE_COLOR_BRIGHT_MAGENTA "\x1b[1;35m"
#define CONSOLE_COLOR_CYAN           "\x1b[0;36m"
#define CONSOLE_COLOR_BRIGHT_CYAN    "\x1b[1;36m"
#define CONSOLE_COLOR_GRAY           "\x1b[0;90m"
#define CONSOLE_COLOR_WHITE          "\x1b[0;37m"
#define CONSOLE_COLOR_BRIGHT_WHITE   "\x1b[1;37m"

/* 颜色输出模式 */
typedef enum {
    COLOR_MODE_AUTO,
    COLOR_MODE_NEVER,
    COLOR_MODE_ALWAYS
} ColorMode;

/* 全局颜色模式，默认自动检测终端 */
static ColorMode g_color_mode = COLOR_MODE_AUTO;

/* --- 私有函数声明 --------------------------------------------------------- */

static FILE       *console_get_stream(ConsoleLogLevel level);
static const char *console_get_level_color(ConsoleLogLevel level);
static const char *console_get_level_prefix(ConsoleLogLevel level);
static void        console_flush_stdin(void);
static void        console_vprintf_color(const char *color, const char *fmt,
                                         va_list args);
static void        console_vlog(ConsoleLogLevel level, const char *fmt,
                                va_list args);

/* --- 公开接口实现 --------------------------------------------------------- */

/* 启用或禁用 ANSI 颜色输出 */
void console_set_color_enabled(bool enabled) {
    g_color_mode = enabled ? COLOR_MODE_ALWAYS : COLOR_MODE_NEVER;
}

/* 根据模式字符串设置颜色输出策略 */
void console_set_color_mode(const char *mode) {
    if (!mode) return;

    if (strcmp(mode, "always") == 0)
        g_color_mode = COLOR_MODE_ALWAYS;
    else if (strcmp(mode, "never") == 0)
        g_color_mode = COLOR_MODE_NEVER;
    else
        g_color_mode = COLOR_MODE_AUTO;
}

/* 返回当前颜色模式的可读字符串 */
const char *console_get_color_mode(void) {
    switch (g_color_mode) {
        case COLOR_MODE_ALWAYS: return "always";
        case COLOR_MODE_NEVER:  return "never";
        default:                return "auto";
    }
}

/* 判断当前是否应输出 ANSI 转义序列 */
bool console_should_use_color(void) {
    switch (g_color_mode) {
        case COLOR_MODE_ALWAYS: return true;
        case COLOR_MODE_NEVER:  return false;
        case COLOR_MODE_AUTO:
        default:                return utils_compat_isatty_stdout();
    }
}

/* 以指定颜色打印格式化文本 */
void console_printf_color(const char *color, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    console_vprintf_color(color, fmt, args);
    va_end(args);
}

void console_print_text(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (fmt) vfprintf(stdout, fmt, args);
    va_end(args);
}

void console_print_newline(void) {
    fputc('\n', stdout);
}

void console_print_spaces(int count) {
    while (count-- > 0) fputc(' ', stdout);
}

/* --- 语义角色 → ANSI 颜色映射 --------------------------------------------- */

/* 角色到 ANSI 颜色码的静态映射表 */
static const char *console_role_color(ConsoleColorRole role) {
    switch (role) {
        case COLOR_ROLE_PRIMARY:   return CONSOLE_COLOR_BRIGHT_CYAN;
        case COLOR_ROLE_SECONDARY: return CONSOLE_COLOR_GRAY;
        case COLOR_ROLE_MUTED:     return CONSOLE_COLOR_GRAY;
        case COLOR_ROLE_ACCENT:    return CONSOLE_COLOR_BRIGHT_YELLOW;
        case COLOR_ROLE_SUCCESS:   return CONSOLE_COLOR_BRIGHT_GREEN;
        case COLOR_ROLE_WARNING:   return CONSOLE_COLOR_BRIGHT_YELLOW;
        case COLOR_ROLE_ERROR:     return CONSOLE_COLOR_BRIGHT_RED;
        case COLOR_ROLE_VALUE:     return CONSOLE_COLOR_WHITE;
        case COLOR_ROLE_ID:        return CONSOLE_COLOR_GRAY;
        case COLOR_ROLE_TITLE:     return CONSOLE_COLOR_BRIGHT_WHITE;
        case COLOR_ROLE_TAG:       return CONSOLE_COLOR_WHITE;
        case COLOR_ROLE_DATE:      return CONSOLE_COLOR_GRAY;
        case COLOR_ROLE_PARAM:     return CONSOLE_COLOR_CYAN;
        case COLOR_ROLE_DESC:      return CONSOLE_COLOR_GRAY;
        case COLOR_ROLE_EXAMPLE:   return CONSOLE_COLOR_GREEN;
        case COLOR_ROLE_EASY:      return CONSOLE_COLOR_BRIGHT_GREEN;
        case COLOR_ROLE_MEDIUM:    return CONSOLE_COLOR_BRIGHT_YELLOW;
        case COLOR_ROLE_HARD:      return CONSOLE_COLOR_BRIGHT_RED;
        case COLOR_ROLE_EXTREME:   return CONSOLE_COLOR_BRIGHT_MAGENTA;
        case COLOR_ROLE_SPECIAL:   return CONSOLE_COLOR_BRIGHT_MAGENTA;
        case COLOR_ROLE_DIVIDER:   return CONSOLE_COLOR_GRAY;
        case COLOR_ROLE_PROMPT:    return CONSOLE_COLOR_BRIGHT_CYAN;
        case COLOR_ROLE_BAR_FILL:  return CONSOLE_COLOR_BRIGHT_BLUE;
        case COLOR_ROLE_BAR_EMPTY:        return CONSOLE_COLOR_GRAY;
        case COLOR_ROLE_MASTERY_NONE:     return CONSOLE_COLOR_GRAY;
        case COLOR_ROLE_MASTERY_LEARNING: return CONSOLE_COLOR_BRIGHT_BLUE;
        case COLOR_ROLE_MASTERY_FAMILIAR: return CONSOLE_COLOR_CYAN;
        case COLOR_ROLE_MASTERY_MASTERED: return CONSOLE_COLOR_BRIGHT_CYAN;
        case COLOR_ROLE_MASTERY_EXPERT:   return CONSOLE_COLOR_BRIGHT_WHITE;
        default:                          return NULL;
    }
}

/* 以指定语义角色打印格式化文本 */
void console_print_role(ConsoleColorRole role, const char *fmt, ...) {
    va_list args; va_start(args, fmt);
    console_vprintf_color(console_role_color(role), fmt, args);
    va_end(args);
}

/* 仅输出指定角色的 ANSI 起始序列 */
void console_begin_role(ConsoleColorRole role) {
    const char *c = console_role_color(role);
    if (c && console_should_use_color()) printf("%s", c);
}

/* 输出 ANSI 重置序列 */
void console_reset_color(void) {
    if (console_should_use_color()) printf("%s", CONSOLE_COLOR_RESET);
}

/* --- 便利打印函数（委托到角色系统） --------------------------------------- */

/* 以亮青色打印主要文本 */
void console_print_primary(const char *fmt, ...) {
    va_list args; va_start(args, fmt);
    console_vprintf_color(console_role_color(COLOR_ROLE_PRIMARY), fmt, args);
    va_end(args);
}

/* 以灰色打印次要文本 */
void console_print_secondary(const char *fmt, ...) {
    va_list args; va_start(args, fmt);
    console_vprintf_color(console_role_color(COLOR_ROLE_SECONDARY), fmt, args);
    va_end(args);
}

/* 以青色打印弱化文本 */
void console_print_muted(const char *fmt, ...) {
    va_list args; va_start(args, fmt);
    console_vprintf_color(console_role_color(COLOR_ROLE_MUTED), fmt, args);
    va_end(args);
}

/* 以亮品红色打印强调文本 */
void console_print_accent(const char *fmt, ...) {
    va_list args; va_start(args, fmt);
    console_vprintf_color(console_role_color(COLOR_ROLE_ACCENT), fmt, args);
    va_end(args);
}

/* 以亮绿色打印交互提示符 */
void console_print_prompt(const char *fmt, ...) {
    va_list args; va_start(args, fmt);
    console_vprintf_color(console_role_color(COLOR_ROLE_PROMPT), fmt, args);
    va_end(args);
}

/* 以亮绿色打印成功状态文本 */
void console_print_success(const char *fmt, ...) {
    va_list args; va_start(args, fmt);
    console_vprintf_color(console_role_color(COLOR_ROLE_SUCCESS), fmt, args);
    va_end(args);
}

/* 以亮红色打印错误文本，不带日志前缀 */
void console_print_error_raw(const char *fmt, ...) {
    va_list args; va_start(args, fmt);
    console_vprintf_color(console_role_color(COLOR_ROLE_ERROR), fmt, args);
    va_end(args);
}

/* --- 日志输出 -------------------------------------------------------------- */

/* 打印信息级别日志消息 */
void console_log_info(const char *fmt, ...) {
    va_list args; va_start(args, fmt);
    console_vlog(CONSOLE_LOG_INFO, fmt, args);
    va_end(args);
}

/* 打印成功级别日志消息 */
void console_log_success(const char *fmt, ...) {
    va_list args; va_start(args, fmt);
    console_vlog(CONSOLE_LOG_SUCCESS, fmt, args);
    va_end(args);
}

/* 打印警告级别日志消息 */
void console_log_warn(const char *fmt, ...) {
    va_list args; va_start(args, fmt);
    console_vlog(CONSOLE_LOG_WARN, fmt, args);
    va_end(args);
}

/* 打印错误级别日志消息 */
void console_log_error(const char *fmt, ...) {
    va_list args; va_start(args, fmt);
    console_vlog(CONSOLE_LOG_ERROR, fmt, args);
    va_end(args);
}

/* 打印致命错误级别日志消息 */
void console_log_fatal(const char *fmt, ...) {
    va_list args; va_start(args, fmt);
    console_vlog(CONSOLE_LOG_FATAL, fmt, args);
    va_end(args);
}

/* 从标准输入读取一行字符串，去除换行符 */
AlgoStatus console_input_string(const char *prompt, char *buffer,
                                size_t size) {
    if (!prompt || !buffer || size == 0) return ALGO_ERR_ARG;

    console_print_primary("%s", prompt);

    if (!fgets(buffer, (int)size, stdin)) return ALGO_ERR_EOF;

    // 去除行尾换行符，若未找到换行说明输入超长
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    } else if (len == size - 1) {
        console_flush_stdin();
        return ALGO_ERR_STR_TOO_LONG;
    }

    return ALGO_SUCCESS;
}

/* 从标准输入读取 [min, max] 范围内的整数 */
AlgoStatus console_input_int(const char *prompt, int *value, int min,
                             int max) {
    char buf[32];
    AlgoStatus status = console_input_string(prompt, buf, sizeof(buf));
    if (status != ALGO_SUCCESS) return status;

    char *endptr = NULL;
    long  v      = strtol(buf, &endptr, 10);

    if (endptr == buf || *endptr != '\0') return ALGO_ERR_ARG;
    if (v < min || v > max) return ALGO_ERR_RANGE;

    *value = (int)v;
    return ALGO_SUCCESS;
}

/* 提示用户进行是/否确认，以 'y'/'Y' 开头为正 */
AlgoStatus console_confirm(const char *prompt, bool *confirmed) {
    if (!prompt || !confirmed) return ALGO_ERR_ARG;

    char buf[8];
    console_print_primary("%s [y/N]: ", prompt);

    if (!fgets(buf, sizeof(buf), stdin)) return ALGO_ERR_EOF;

    // 若输入超长，丢弃 stdin 余量防止污染后续读取
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] != '\n') console_flush_stdin();

    *confirmed = (tolower((unsigned char)buf[0]) == 'y');
    return ALGO_SUCCESS;
}

/* 以亮白色打印标题行（委托到角色系统） */
void console_print_title(const char *title) {
    if (!title) return;
    console_print_role(COLOR_ROLE_TITLE, "%s\n", title);
}

/* 以角色主色打印对齐的键值对（委托到角色系统） */
void console_print_kv(const char *key, const char *value) {
    if (!key || !value) return;
    console_begin_role(COLOR_ROLE_PRIMARY);
    printf("%-18s", key);
    console_reset_color();
    console_print_role(COLOR_ROLE_MUTED, " = ");
    console_print_role(COLOR_ROLE_VALUE, "%s\n", value);
}

/* 打印灰色水平分隔线 */
void console_print_divider(void) {
    console_print_separator('-', 80);
}

void console_print_separator(char ch, int width) {
    if (width <= 0) width = 80;
    console_begin_role(COLOR_ROLE_DIVIDER);
    for (int i = 0; i < width; i++) putchar(ch);
    console_reset_color();
    putchar('\n');
}

/* --- 私有函数实现 --------------------------------------------------------- */

/* 返回与给定日志级别对应的输出流：warn 及以下用 stderr，其余用 stdout */
static FILE *console_get_stream(ConsoleLogLevel level) {
    return (level <= CONSOLE_LOG_WARN) ? stderr : stdout;
}

/* 返回与给定日志级别对应的 ANSI 颜色代码 */
static const char *console_get_level_color(ConsoleLogLevel level) {
    switch (level) {
        case CONSOLE_LOG_FATAL:   return CONSOLE_COLOR_BRIGHT_RED;
        case CONSOLE_LOG_ERROR:   return CONSOLE_COLOR_RED;
        case CONSOLE_LOG_WARN:    return CONSOLE_COLOR_BRIGHT_YELLOW;
        case CONSOLE_LOG_INFO:    return CONSOLE_COLOR_BRIGHT_BLUE;
        case CONSOLE_LOG_SUCCESS: return CONSOLE_COLOR_BRIGHT_GREEN;
        default:                  return "";
    }
}

/* 返回与给定日志级别对应的可读前缀 */
static const char *console_get_level_prefix(ConsoleLogLevel level) {
    switch (level) {
        case CONSOLE_LOG_FATAL:   return "fatal:";
        case CONSOLE_LOG_ERROR:   return "error:";
        case CONSOLE_LOG_WARN:    return "warning:";
        case CONSOLE_LOG_INFO:    return "info:";
        case CONSOLE_LOG_SUCCESS: return "success:";
        default:                  return "";
    }
}

/* 丢弃标准输入中直到下一个换行符为止的剩余字符 */
static void console_flush_stdin(void) {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF);
}

/* 以可选 ANSI 颜色包装打印格式化字符串到标准输出 */
static void console_vprintf_color(const char *color, const char *fmt,
                                  va_list args) {
    bool use_color = console_should_use_color();
    if (use_color && color && color[0] != '\0') fputs(color, stdout);
    vfprintf(stdout, fmt, args);
    if (use_color && color && color[0] != '\0')
        fputs(CONSOLE_COLOR_RESET, stdout);
}

/* 核心日志输出例程：带级别前缀和颜色将消息写入对应流 */
static void console_vlog(ConsoleLogLevel level, const char *fmt,
                         va_list args) {
    FILE       *stream    = console_get_stream(level);
    bool        use_color = console_should_use_color();
    const char *color     = use_color ? console_get_level_color(level) : "";
    const char *prefix    = console_get_level_prefix(level);

    if (prefix[0] != '\0') {
        if (use_color)
            fprintf(stream, "%s%s%s ", color, prefix,
                    CONSOLE_COLOR_RESET);
        else
            fprintf(stream, "%s ", prefix);
    }
    if (fmt) vfprintf(stream, fmt, args);
    fprintf(stream, "\n");
    fflush(stream);
}
