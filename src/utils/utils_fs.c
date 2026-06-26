/**
 * @file utils_fs.c
 * @brief 跨平台文件系统检测、拷贝与路径处理工具实现。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include <algorepo/utils/utils_fs.h>

#include <string.h>
#include <stdio.h>

#include <algorepo/utils/utils_compat.h>

/* --- 文件操作 ------------------------------------------------------------- */

/* 检查指定路径的文件是否存在 */
bool utils_file_exists(const char *path) {
    return utils_compat_path_exists(path);
}

/* 跨平台二进制文件拷贝（支持大文件流式读写） */
AlgoStatus utils_copy_file(const char *src, const char *dest) {
    if (!src || !dest) return ALGO_ERR_ARG;

    /* 源与目标相同视为幂等成功 */
    if (strcmp(src, dest) == 0) return ALGO_SUCCESS;

    FILE *source = utils_compat_fopen(src, "rb");
    if (!source) return ALGO_ERR_FILE;

    FILE *target = utils_compat_fopen(dest, "wb");
    if (!target) {
        fclose(source);
        return ALGO_ERR_FILE;
    }

    /* 8 KiB 流式缓冲区，兼顾大文件性能与内存开销 */
    char   buffer[8192];
    size_t nread;

    while ((nread = fread(buffer, 1, sizeof(buffer), source)) > 0) {
        if (fwrite(buffer, 1, nread, target) != nread) {
            fclose(source);
            fclose(target);
            return ALGO_ERR_FILE;
        }
    }

    /* 读取过程中若发生流错误则报告 */
    AlgoStatus res = ferror(source) ? ALGO_ERR_FILE : ALGO_SUCCESS;

    fclose(source);
    fclose(target);

    return res;
}

/* 获取文件路径的扩展名（含点号），无扩展名返回空字符串 */
const char *utils_get_file_ext(const char *path) {
    if (!path || path[0] == '\0') {
        return "";
    }

    const char *dot = strrchr(path, '.');

    /* 无后缀文件以及 ".gitignore" 类隐藏文件均返回空字符串 */
    if (!dot || dot == path) {
        return "";
    }

    return dot;
}

/* --- 路径处理 ------------------------------------------------------------- */

/* 将路径中反斜杠统一替换为正斜杠 */
void utils_path_normalize_separators(char *path) {
    if (!path) return;

    for (; *path; path++) {
        if (*path == '\\') *path = '/';
    }
}

/* 判断路径是否为绝对路径 */
bool utils_path_is_absolute(const char *path) {
    if (!path) return false;

    /* Unix 绝对路径或以反斜杠开头的 Windows 路径 */
    if (path[0] == '/' || path[0] == '\\') return true;

    /* Windows 盘符路径（如 C:\...） */
    size_t len = strlen(path);
    if (len >= 2 && path[1] == ':') return true;

    return false;
}
