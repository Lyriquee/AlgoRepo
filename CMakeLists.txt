cmake_minimum_required(VERSION 3.18)
project(AlgoRepo LANGUAGES C)


# 不支持MSVC编译, 虽然后面有MSVC判断
if(MSVC)
    message(FATAL_ERROR
        "MSVC is not supported. Please use MinGW-w64 (GCC)."
    )
endif()

# ==============================================================================
# 全局构建配置
# ==============================================================================
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ==============================================================================
# 全局编译警告
# ==============================================================================
add_library(algo_warnings INTERFACE)
if(MSVC)
    target_compile_options(algo_warnings INTERFACE /W4 /utf-8)
    target_compile_definitions(algo_warnings INTERFACE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(algo_warnings INTERFACE -Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

# ==============================================================================
# 全局公共头文件
# ==============================================================================
add_library(algo_headers INTERFACE)
target_include_directories(algo_headers INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

# 分层依赖图
#                  表示层           ┐
#             (apps/cli,tui)        > apps
#                    │              ┘
#                    ▼
#                  应用层           ┐
#                 service           │
#                    │              │
#                    ▼              │
#              基础设施适配器       │
#            runtime / storage      > src
#                    │              │
#                    ▼              │
#                  领域层           │
#                   core            │
#                    │              │
#                    ▼              │
#               基础基础设施        │
#         utils / console / config  ┘

# ==============================================================================
# 核心业务模块（Domain / Infrastructure / Application）
# ==============================================================================
add_subdirectory(src)

# ==============================================================================
# 用户界面模块（CLI / TUI Frontends）
# ==============================================================================
add_subdirectory(apps)

# ==============================================================================
# 程序启动器（顶层 main() 入口，选择 CLI / TUI 前端）
# ==============================================================================
add_executable(algo apps/main.c)
target_link_libraries(algo PRIVATE algo::cli algo::tui)

# ==============================================================================
# Release 模式（产物优化与瘦身）
# ==============================================================================
if(MSVC)
    target_compile_options(algo PRIVATE $<$<CONFIG:Release>:/Oi /Gy>)
    target_link_options(algo PRIVATE $<$<CONFIG:Release>:/OPT:REF /OPT:ICF>)
else()
    target_link_options(algo PRIVATE $<$<CONFIG:Release>:-s>)
endif()

# ==============================================================================
# Unity 单元测试（暂不使用)
# ==============================================================================

# add_library(unity STATIC third_party/unity/src/unity.c)
# target_include_directories(unity PUBLIC third_party/unity/src)
#
# option(BUILD_TESTING "Build tests" OFF)
#
# if(BUILD_TESTING)
#     enable_testing()
#
#     file(GLOB_RECURSE TEST_SOURCES tests/*.c)
#
#     add_executable(tests ${TEST_SOURCES})
#
#     target_link_libraries(tests PRIVATE
#         unity
#         algo::core
#         algo::utils
#         algo::storage
#         algo::config
#     )
#
#     # target_include_directories(tests PRIVATE tests/) # 递归加入头文件（不用）
#     target_include_directories(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
#
#     add_test(NAME all_tests COMMAND tests)
# endif()
