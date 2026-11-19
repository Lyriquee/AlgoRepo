/**
 * @file problem_types.h
 * @brief 题目相关类型定义、状态码、枚举及转换函数。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#pragma once


/* --- Problem 数据结构限制-------------------------------------------------- */

#define ALGO_MAX_TITLE_LEN      256  // 题目标题最大长度(含'\0')，输入层越界拦截
#define ALGO_MAX_DESC_LEN       4096 // 题目描述最大长度(含'\0')
#define ALGO_MAX_NOTES_LEN      8192 // 个人笔记最大长度(仅本地存储)
#define ALGO_MAX_PATH_LEN       512  // 文件路径最大长度(兼容Windows长路径)
#define ALGO_MAX_DATETIME_LEN   32   // 时间字符串长度(YYYY-MM-DD HH:MM:SS)
#define ALGO_MAX_TAGS           8    // 标签数量上限
#define ALGO_MAX_TAG_LEN        32   // 单个标签最大长度(含'\0')


/* --- Status Code ---------------------------------------------------------- */

/**
 * @brief 操作结果状态码枚举。
 *
 * 所有 ALGO 库函数通过此枚举返回操作结果，调用方应检查返回值。
 */
typedef enum {
    ALGO_SUCCESS = 0,       ///< 操作成功
    ALGO_ERR_FILE,          ///< 文件打开失败
    ALGO_ERR_IO,            ///< I/O 读写错误
    ALGO_ERR_MEM,           ///< 内存分配失败
    ALGO_ERR_NOT_FOUND,     ///< 目标不存在

    ALGO_ERR_FORMAT,        ///< 文件格式非法
    ALGO_ERR_EOF,           ///< 文件意外结束
    ALGO_ERR_ARG,           ///< 参数非法
    ALGO_ERR_INVALID_ENUM,  ///< 无效枚举值
    ALGO_ERR_REQUIRED,      ///< 缺少必填字段

    ALGO_ERR_RANGE,         ///< 数值超出范围
    ALGO_ERR_TAG_FULL,      ///< 标签数量已满
    ALGO_ERR_STR_TOO_LONG,  ///< 字符串过长

    ALGO_ERR_UNKNOWN = 99   ///< 未知错误
} AlgoStatus;


/* --- Difficulty ----------------------------------------------------------- */

/**
 * @brief 题目难度等级枚举。
 *
 * 用于标记算法题目的难度，从易到难共五个等级。
 */
typedef enum {
    ALGO_DIFF_UNKNOWN = 0,  ///< 未知
    ALGO_DIFF_EASY,         ///< 容易
    ALGO_DIFF_MEDIUM,       ///< 中等
    ALGO_DIFF_HARD,         ///< 困难
    ALGO_DIFF_EXTREME       ///< 极难
} Difficulty;


/* --- Mastery -------------------------------------------------------------- */

/**
 * @brief 算法掌握程度枚举。
 *
 * 用于标记用户对某道题目的掌握水平，从初学到精通共五个等级。
 */
typedef enum {
    ALGO_MASTERY_UNKNOWN = 0,  ///< 未知
    ALGO_MASTERY_LEARNING,     ///< 了解
    ALGO_MASTERY_FAMILIAR,     ///< 熟悉
    ALGO_MASTERY_MASTERED,     ///< 掌握
    ALGO_MASTERY_EXPERT        ///< 精通
} Mastery;


/* --- Enum to String ------------------------------------------------------- */

/**
 * @brief 将 AlgoStatus 枚举转换为中文描述字符串。
 *
 * @return 对应的中文描述字符串；未知枚举值返回 "未知错误"。
 */
const char *algo_status_to_string(AlgoStatus status);

/**
 * @brief 将 Difficulty 枚举转换为中文描述字符串。
 *
 * @return 对应的中文描述字符串；未知枚举值返回 "未知"。
 */
const char *algo_difficulty_to_string(Difficulty diff);

/**
 * @brief 将 Mastery 枚举转换为中文描述字符串。
 *
 * @return 对应的中文描述字符串；未知枚举值返回 "未知"。
 */
const char *algo_mastery_to_string(Mastery mastery);
