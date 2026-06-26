/**
 * @file storage_file.c
 * @brief 二进制文件持久化实现：魔数校验、版本管理及结构体序列化。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 *
 * 实现题库到本地二进制文件的双向转换。文件格式为：
 *   [魔数 4B][版本 4B][数量 4B][Problem[0]]...[Problem[N-1]]
 *
 * 所有路径均通过 utils_compat_fopen 处理，保证 Windows 下 UTF-8 中文
 * 路径的正确性。
 */

#include <algorepo/storage/storage_file.h>

#include <stdio.h>
#include <string.h>

#include <algorepo/utils/utils_compat.h>

/* 单次加载的题目数量上限，用于防止损坏文件分配过量内存 */
#define STORAGE_FILE_MAX_COUNT 1000000

/* --- 公开接口实现 --------------------------------------------------------- */

/* 将题库序列化并写入二进制文件 */
AlgoStatus storage_file_save(const ProblemCollection *pcol,
                             const char *filepath) {
    if (!pcol || !filepath) return ALGO_ERR_ARG;

    FILE *fp = utils_compat_fopen(filepath, "wb");
    if (!fp) return ALGO_ERR_FILE;

    AlgoStatus      status = ALGO_SUCCESS;
    AlgoFileHeader  header = {0};

    /* 填充文件头 */
    memcpy(header.magic, ALGO_FILE_MAGIC, 4);
    header.version = ALGO_FILE_VERSION;
    header.count   = (uint32_t)pcol->size;

    /* 写入文件头 */
    if (fwrite(&header, sizeof(AlgoFileHeader), 1, fp) != 1) {
        status = ALGO_ERR_IO;
        goto out_close;
    }

    /* 批量写入题目结构体数组 */
    if (header.count > 0) {
        if (fwrite(pcol->items, sizeof(Problem), header.count, fp) !=
            header.count) {
            status = ALGO_ERR_IO;
            goto out_close;
        }
    }

out_close:
    fclose(fp);
    return status;
}

/* 从二进制文件反序列化题库 */
AlgoStatus storage_file_load(ProblemCollection *pcol, const char *filepath) {
    if (!pcol || !filepath) return ALGO_ERR_ARG;

    FILE *fp = utils_compat_fopen(filepath, "rb");
    if (!fp) return ALGO_ERR_NOT_FOUND;

    AlgoStatus      status = ALGO_SUCCESS;
    AlgoFileHeader  header;

    /* 读取文件头 */
    if (fread(&header, sizeof(AlgoFileHeader), 1, fp) != 1) {
        status = ALGO_ERR_IO;
        goto out_close;
    }

    /* 校验魔数 */
    if (memcmp(header.magic, ALGO_FILE_MAGIC, 4) != 0) {
        status = ALGO_ERR_FORMAT;
        goto out_close;
    }

    /* 校验版本号：接受 [1, ALGO_FILE_VERSION] 范围内的所有版本 */
    if (header.version < 1 || header.version > ALGO_FILE_VERSION) {
        status = ALGO_ERR_FORMAT;
        goto out_close;
    }

    /* 防止损坏文件分配过量内存 */
    if (header.count > STORAGE_FILE_MAX_COUNT) {
        status = ALGO_ERR_FORMAT;
        goto out_close;
    }

    /* 清空现有数据并预分配容量 */
    pcol_clear(pcol);

    if (header.count > 0) {
        status = pcol_reserve(pcol, header.count);
        if (status != ALGO_SUCCESS) goto out_close;
    }

    /* 逐条读取题目结构体并追踪最大 ID */
    int max_id = 0;
    for (uint32_t i = 0; i < header.count; i++) {
        Problem temp_prob;
        if (fread(&temp_prob, sizeof(Problem), 1, fp) != 1) {
            status = ALGO_ERR_EOF;
            goto out_close;
        }

        pcol->items[pcol->size++] = temp_prob;
        if (temp_prob.id > max_id) max_id = temp_prob.id;
    }

    /* 恢复自增 ID 起点，确保新题目 ID 不与已有题目冲突 */
    pcol->next_id = max_id + 1;

out_close:
    fclose(fp);
    return status;
}
