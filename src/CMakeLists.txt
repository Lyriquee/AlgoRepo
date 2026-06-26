# ==============================================================================
# AlgoRepo Core Modules

# 本目录包含 AlgoRepo 的核心业务实现，不包含任何 CLI / TUI 界面代码
#
# 架构分层：
#
#   Foundation Infrastructure
#   ├── utils      通用工具库（字符串、路径、时间、跨平台兼容）
#   ├── console    终端输出与 ANSI 控制
#   └── config     配置文件读写与管理
#
#   Domain
#   └── core       Problem / ProblemCollection 等领域模型

#   Infrastructure Adapter
#   ├── storage    领域对象持久化（文件系统适配）
#   └── runtime    工作区、运行环境与上下文管理
#
#   Application
#   └── service    应用服务与业务用例编排
#
#
#   - core 不依赖 storage/runtime/service
#   - storage 不依赖 service
#   - runtime 不依赖 service
#   - service 不直接操作文件系统
#   - CLI/TUI 只能通过 service 访问业务能力

# 依赖关系：
#
#                               service
#                                   │
#                                   ▼
#                               runtime
#                              ╱       ╲
#                             ▼         ▼
#                        storage      config
#                            │
#                            ▼
#                           core
#                            │
#                            ▼
#                           utils
#
#          console ───────> utils
#
#

# ==============================================================================

# ==============================================================================
# 基础设施层（提供跨平台、文件系统、终端等最底层能力）
# ==============================================================================

# 通用工具库 （字符串、路径、时间、跨平台兼容等底层能力）
add_library(algo_utils STATIC utils/utils_compat.c utils/utils_time.c utils/utils_str.c utils/utils_fs.c)
target_link_libraries(algo_utils PUBLIC algo_headers algo_warnings)
target_include_directories(algo_utils PRIVATE utils)
add_library(algo::utils ALIAS algo_utils)

# 控制台表现支持库（ANSI 颜色、日志输出、终端输入等）
add_library(algo_console STATIC console/console_ui.c)
target_link_libraries(algo_console PUBLIC algo::utils)
target_include_directories(algo_console PRIVATE console)
add_library(algo::console ALIAS algo_console)

# 配置系统库（配置文件加载、保存、校验）
add_library(algo_config STATIC config/config.c)
target_link_libraries(algo_config PUBLIC algo::utils)
target_include_directories(algo_config PRIVATE config)
add_library(algo::config ALIAS algo_config)

# ==============================================================================
# 领域层（核心业务数据模型与领域规则）
# ==============================================================================

# 领域核心模型库 （Problem / ProblemCollection 等纯领域对象）
add_library(algo_core STATIC core/problem_types.c core/problem.c core/problem_collection.c)
target_link_libraries(algo_core PUBLIC algo::utils)
target_include_directories(algo_core PRIVATE core)
add_library(algo::core ALIAS algo_core)

# ==============================================================================
# 基础设施适配层（将领域对象接入文件系统、运行环境等外部世界）
# ==============================================================================

# 存储持久化库（二进制序列化、本地文件读写）
add_library(algo_storage STATIC storage/storage.c storage/storage_file.c)
target_link_libraries(algo_storage PUBLIC algo::core algo::utils)
target_include_directories(algo_storage PRIVATE storage)
add_library(algo::storage ALIAS algo_storage)

# 运行时环境库（工作区定位、路径解析、上下文生命周期管理）
add_library(algo_runtime STATIC runtime/runtime.c)
target_link_libraries(algo_runtime PUBLIC algo::core PRIVATE algo::config algo::storage)
target_include_directories(algo_runtime PRIVATE runtime)
add_library(algo::runtime ALIAS algo_runtime)

# ==============================================================================
# 应用服务层（编排业务用例流程，协调 Runtime / Storage / Domain）
# ==============================================================================

# 业务服务库（添加题目、删除题目、查询题目等应用级用例）
add_library(algo_service STATIC service/problem_service.c service/problem_query.c)
target_link_libraries(algo_service PUBLIC algo::runtime)
target_include_directories(algo_service PRIVATE service)
add_library(algo::service ALIAS algo_service)
