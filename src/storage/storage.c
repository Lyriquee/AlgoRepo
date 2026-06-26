/**
 * @file storage.c
 * @brief 持久化存储抽象层实现：参数校验后委托给 storage_file 具体实现。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 *
 * 本层仅做参数合法性校验，所有实际 I/O 操作委托至 storage_file 模块。
 * 未来引入新存储后端（如网络存储、嵌入式数据库）时，在此处根据 URI
 * scheme 路由至对应实现即可，调用方接口保持不变。
 */

#include <algorepo/storage/storage.h>
#include <algorepo/storage/storage_file.h>

/* --- 公开接口实现 --------------------------------------------------------- */

/* 将题库序列化到指定 URI */
AlgoStatus storage_save(const ProblemCollection *pcol, const char *uri) {
    if (!pcol || !uri) return ALGO_ERR_ARG;
    return storage_file_save(pcol, uri);
}

/* 从指定 URI 反序列化题库 */
AlgoStatus storage_load(ProblemCollection *pcol, const char *uri) {
    if (!pcol || !uri) return ALGO_ERR_ARG;
    return storage_file_load(pcol, uri);
}
