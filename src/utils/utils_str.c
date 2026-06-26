/**
 * @file utils_str.c
 * @brief 字符串安全操作、规范化与校验工具实现。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include <algorepo/utils/utils_str.h>

#include <ctype.h>
#include <string.h>

/* --- 公开接口实现 --------------------------------------------------------- */

/* 安全复制字符串到目标缓冲区，截断时返回 ALGO_ERR_STR_TOO_LONG */
AlgoStatus utils_strcpy_safe(char *dest, const char *src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) return ALGO_ERR_ARG;

    size_t i;
    for (i = 0; i < dest_size - 1 && src[i] != '\0'; ++i) dest[i] = src[i];
    dest[i] = '\0';

    /* 若循环因 dest_size 耗尽而终止但 src 还有内容，说明发生截断 */
    return (src[i] != '\0') ? ALGO_ERR_STR_TOO_LONG : ALGO_SUCCESS;
}

/* 将输入字符串转换为文件友好的 slug 格式并保留 UTF-8 字符 */
void utils_slugify(const char *input, char *output, size_t output_size) {
    if (!output || output_size == 0) return;

    /* 输入为 NULL 时回退为默认名称 */
    if (!input) {
        utils_strcpy_safe(output, "untitled", output_size);
        return;
    }

    size_t j = 0;
    int    prev_dash = 0;

    for (size_t i = 0; input[i] != '\0' && j < output_size - 1; ++i) {
        unsigned char c = (unsigned char)input[i];

        /* 保留非 ASCII 字节（UTF-8 多字节序列），不转换 */
        if (c >= 0x80) {
            output[j++] = (char)c;
            prev_dash = 0;
            continue;
        }

        /* ASCII 字母统一转小写 */
        if (isalnum(c)) {
            output[j++] = (char)tolower(c);
            prev_dash = 0;
            continue;
        }

        /* 空白和常用分隔符替换为连字符，合并连续分隔符，避免前导连字符 */
        if ((c == ' ' || c == '-' || c == '_') && !prev_dash && j > 0) {
            output[j++] = '-';
            prev_dash = 1;
        }
    }

    /* 移除末尾多余的连字符 */
    if (j > 0 && output[j - 1] == '-') --j;

    output[j] = '\0';

    /* 结果为空时回退为默认名称 */
    if (j == 0) utils_strcpy_safe(output, "untitled", output_size);
}

/* 去除字符串首尾空白字符（原地修改） */
void utils_trim(char *str) {
    if (!str || str[0] == '\0') return;

    /* 跳过前导空白 */
    char *start = str;
    while (isspace((unsigned char)*start)) ++start;

    /* 若存在前导空白，将剩余内容前移覆盖 */
    if (start != str) memmove(str, start, strlen(start) + 1);

    /* 移除尾部空白 */
    size_t len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1]))
        str[--len] = '\0';
}

/* 检查字符串是否为空（NULL 或首字符为 '\0'） */
bool utils_str_empty(const char *s) {
    return !s || s[0] == '\0';
}

/* 去除行尾换行符（\n 和 \r）*/
void utils_trim_newline(char *line) {
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
        line[--len] = '\0';
}


