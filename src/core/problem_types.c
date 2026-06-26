/**
 * @file problem_types.c
 * @brief 枚举值到中文字符串的转换实现。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include <algorepo/core/problem_types.h>


/* --- 私有函数声明 --------------------------------------------------------- */


/* --- 公开接口实现 --------------------------------------------------------- */

/* 将 AlgoStatus 枚举转换为中文描述字符串 */
const char *algo_status_to_string(AlgoStatus status) {
    switch (status) {
        // 执行成功
        case ALGO_SUCCESS:           return "操作成功";

        // 系统与环境级错误
        case ALGO_ERR_FILE:          return "文件打开失败";
        case ALGO_ERR_IO:            return "I/O 读写错误";
        case ALGO_ERR_MEM:           return "内存分配失败";
        case ALGO_ERR_NOT_FOUND:     return "目标不存在";

        // 数据校验与格式错误
        case ALGO_ERR_FORMAT:        return "文件格式非法";
        case ALGO_ERR_EOF:           return "文件意外结束";
        case ALGO_ERR_ARG:           return "参数非法";
        case ALGO_ERR_INVALID_ENUM:  return "无效枚举值";
        case ALGO_ERR_REQUIRED:      return "缺少必填字段";

        // 容量与边界错误
        case ALGO_ERR_RANGE:         return "数值超出范围";
        case ALGO_ERR_TAG_FULL:      return "标签数量已满";
        case ALGO_ERR_STR_TOO_LONG:  return "字符串过长";

        default:                     return "未知错误";
    }
}

/* 将 Difficulty 枚举转换为中文描述字符串 */
const char *algo_difficulty_to_string(Difficulty diff) {
    switch (diff) {
        case ALGO_DIFF_UNKNOWN: return "未知";
        case ALGO_DIFF_EASY:    return "容易";
        case ALGO_DIFF_MEDIUM:  return "中等";
        case ALGO_DIFF_HARD:    return "困难";
        case ALGO_DIFF_EXTREME: return "极难";
        default:                return "未知";
    }
}

/* 将 Mastery 枚举转换为中文描述字符串 */
const char *algo_mastery_to_string(Mastery mastery) {
    switch (mastery) {
        case ALGO_MASTERY_UNKNOWN:  return "未知";
        case ALGO_MASTERY_LEARNING: return "了解";
        case ALGO_MASTERY_FAMILIAR: return "熟悉";
        case ALGO_MASTERY_MASTERED: return "掌握";
        case ALGO_MASTERY_EXPERT:   return "精通";
        default:                    return "未设置";
    }
}


/* --- 私有函数实现 --------------------------------------------------------- */
