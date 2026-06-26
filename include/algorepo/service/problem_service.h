/**
 * @file problem_service.h
 * @brief 应用服务层：题目增删改查的业务用例编排。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 *
 * 服务层封装对题库的原子操作，组合多个底层调用（实体校验、路径生成、
 * 文件拷贝、题库增删）形成完整业务用例，并实现失败时的自动回滚。
 */

#pragma once

#include <algorepo/core/problem_types.h>
#include <algorepo/core/problem.h>
#include <algorepo/runtime/runtime.h>

/* --- Problem CRUD ---------------------------------------------------------- */

/**
 * @brief 添加新题目到当前工作区。
 *
 * 自动分配 ID、记录创建时间戳、生成题解文件路径。若提供 source_file
 * 则将其内容拷贝到题解目录。
 *
 * 失败回滚：若文件拷贝失败，已添加至题库的题目记录会被自动回滚删除。
 *
 * @param ctx         运行时上下文。
 * @param p           待添加的题目实体（ID、时间戳、路径由服务自动填充）。
 * @param source_file 可选源文件路径，为 NULL 或空串时不执行拷贝。
 * @return
 * - ALGO_SUCCESS            添加成功。
 * - ALGO_ERR_REQUIRED       题目标题为空。
 * - ALGO_ERR_INVALID_ENUM   难度或掌握程度数值非法。
 * - ALGO_ERR_FILE           源文件拷贝失败。
 * - ALGO_ERR_RANGE          生成的路径超过缓冲区。
 * - ALGO_ERR_MEM            内存分配失败。
 */
AlgoStatus service_add_problem(RuntimeContext *ctx, Problem *p,
                               const char *source_file);

/**
 * @brief 从当前工作区删除指定 ID 的题目。
 *
 * @param ctx       运行时上下文。
 * @param id        待删除的题目 ID。
 * @param keep_file true 保留磁盘上的源文件，false 同时删除。
 * @return
 * - ALGO_SUCCESS       删除成功。
 * - ALGO_ERR_NOT_FOUND 未找到对应 ID 的题目。
 * - ALGO_ERR_ARG       参数非法（ctx 为 NULL）。
 */
AlgoStatus service_remove_problem(RuntimeContext *ctx, int id, bool keep_file);

/* --- Property Updates ------------------------------------------------------ */

/**
 * @brief 更新题目的掌握程度。
 *
 * @param ctx     运行时上下文。
 * @param id      题目 ID。
 * @param mastery 新的掌握程度。
 * @return
 * - ALGO_SUCCESS           更新成功。
 * - ALGO_ERR_NOT_FOUND     未找到对应题目。
 * - ALGO_ERR_INVALID_ENUM  掌握程度枚举值非法。
 * - ALGO_ERR_ARG           参数非法（ctx 为 NULL）。
 */
AlgoStatus service_update_mastery(RuntimeContext *ctx, int id, Mastery mastery);

/**
 * @brief 更新题目的难度等级。
 *
 * @param ctx  运行时上下文。
 * @param id   题目 ID。
 * @param diff 新的难度等级。
 * @return
 * - ALGO_SUCCESS           更新成功。
 * - ALGO_ERR_NOT_FOUND     未找到对应题目。
 * - ALGO_ERR_INVALID_ENUM  难度枚举值非法。
 * - ALGO_ERR_ARG           参数非法（ctx 为 NULL）。
 */
AlgoStatus service_update_difficulty(RuntimeContext *ctx, int id,
                                     Difficulty diff);

/**
 * @brief 更新题目标题（同步更新磁盘源文件路径并重命名）。
 *
 * 更新标题会触发路径重新生成。若旧文件存在则自动拷贝至新路径后删除
 * 旧文件。失败时自动回滚标题和路径至更新前的状态。
 *
 * @param ctx   运行时上下文。
 * @param id    题目 ID。
 * @param title 新标题字符串。
 * @return
 * - ALGO_SUCCESS       更新成功。
 * - ALGO_ERR_NOT_FOUND 未找到对应题目。
 * - ALGO_ERR_REQUIRED  标题为空。
 * - ALGO_ERR_RANGE     路径超过缓冲区。
 * - ALGO_ERR_FILE      文件拷贝/重命名失败。
 * - ALGO_ERR_ARG       参数非法（ctx 为 NULL）。
 */
AlgoStatus service_update_title(RuntimeContext *ctx, int id, const char *title);

/**
 * @brief 更新题目的个人笔记。
 *
 * @param ctx   运行时上下文。
 * @param id    题目 ID。
 * @param notes 新的笔记内容，为 NULL 时返回 ALGO_ERR_ARG。
 * @return
 * - ALGO_SUCCESS       更新成功。
 * - ALGO_ERR_NOT_FOUND 未找到对应题目。
 * - ALGO_ERR_ARG       参数非法（ctx 或 notes 为 NULL）。
 */
AlgoStatus service_update_notes(RuntimeContext *ctx, int id, const char *notes);
