/**
 * @file utils_compat.c
 * @brief 跨平台兼容层实现：终端初始化、安全文件 I/O 与环境检测。

 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 *
 * Windows 端通过 Wide Character API 实现 UTF-8 路径安全访问，并配置控制台
 * 编码与 VT100 转义序列。POSIX 端直接透传标准库调用，无需额外处理。
 */

#include <algorepo/utils/utils_compat.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <algorepo/utils/utils_str.h>


/* --- 内部私有状态 --------------------------------------------------------- */

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <direct.h>
#include <io.h>

/* Windows UTF-8 argv 静态生命周期缓存，供清理函数释放 */
static char **g_utf8_argv = NULL;
static int    g_argc      = 0;

#endif // _WIN32

/* --- 公开接口实现 Windows ------------------------------------------------- */

#ifdef _WIN32

/* 平台兼容层初始化：设置 UTF-8 控制台编码、转换宽字符命令行参数、激活 VT100 */
void utils_compat_init(int *argc, char ***argv) {
    if (!argc || !argv) return;

    /* 强制设置控制台输入/输出全局编码为 UTF-8 */
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    /* 获取 Windows 核心层宽字符命令行参数 */
    LPWSTR *w_argv = CommandLineToArgvW(GetCommandLineW(), argc);
    if (!w_argv) return;

    g_argc      = *argc;
    g_utf8_argv = (char **)calloc((size_t)(*argc), sizeof(char *));
    if (!g_utf8_argv) {
        LocalFree(w_argv);
        return;
    }

    /* 逐个将宽字符参数转换为标准 UTF-8 窄字符串 */
    for (int i = 0; i < *argc; i++) {
        int len = WideCharToMultiByte(CP_UTF8, 0, w_argv[i], -1,
                                      NULL, 0, NULL, NULL);

        if (len <= 0) {
            /* 转换失败时填充空字符串，防止上层解析崩溃 */
            g_utf8_argv[i] = (char *)calloc(1, 1);
            continue;
        }

        g_utf8_argv[i] = (char *)malloc((size_t)len);
        if (!g_utf8_argv[i]) {
            /* 内存耗尽时回退为空字符串 */
            g_utf8_argv[i] = (char *)calloc(1, 1);
            continue;
        }

        WideCharToMultiByte(CP_UTF8, 0, w_argv[i], -1,
                            g_utf8_argv[i], len, NULL, NULL);
    }

    *argv = g_utf8_argv;
    LocalFree(w_argv);

    /* 启用 Windows 控制台 ANSI VT100 虚拟终端转义序列（激活颜色输出） */
    HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h_out != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (GetConsoleMode(h_out, &mode)) {
            SetConsoleMode(h_out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
    }
}

/* 释放兼容层接管的 UTF-8 命令行参数内存 */
void utils_compat_cleanup(void) {
    if (!g_utf8_argv) return;

    for (int i = 0; i < g_argc; i++) {
        free(g_utf8_argv[i]);
    }

    free(g_utf8_argv);
    g_utf8_argv = NULL;
    g_argc      = 0;
}

/* UTF-8 编码安全文件打开：将路径和模式转为宽字符后调用 _wfopen */
FILE *utils_compat_fopen(const char *path, const char *mode) {
    if (!path || !mode) return NULL;

    int path_len = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
    int mode_len = MultiByteToWideChar(CP_UTF8, 0, mode, -1, NULL, 0);

    if (path_len <= 0 || mode_len <= 0) return NULL;

    wchar_t *w_path = (wchar_t *)malloc((size_t)path_len * sizeof(wchar_t));
    wchar_t *w_mode = (wchar_t *)malloc((size_t)mode_len * sizeof(wchar_t));

    if (!w_path || !w_mode) {
        free(w_path);
        free(w_mode);
        return NULL;
    }

    MultiByteToWideChar(CP_UTF8, 0, path, -1, w_path, path_len);
    MultiByteToWideChar(CP_UTF8, 0, mode, -1, w_mode, mode_len);

    /* 调用内核级宽字符文件打开，规避中文路径乱码 */
    FILE *fp = _wfopen(w_path, w_mode);

    free(w_path);
    free(w_mode);

    return fp;
}

/* 检查物理路径是否存在：UTF-8 路径转宽字符后调用 _waccess */
bool utils_compat_path_exists(const char *path) {
    if (!path || path[0] == '\0') return false;

    int len = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
    if (len <= 0) return false;

    wchar_t *w_path = (wchar_t *)malloc((size_t)len * sizeof(wchar_t));
    if (!w_path) return false;

    MultiByteToWideChar(CP_UTF8, 0, path, -1, w_path, len);
    bool exists = (_waccess(w_path, 0) == 0);

    free(w_path);
    return exists;
}

/* 创建目录：_wmkdir 返回 0 成功；EEXIST 视为幂等成功 */
bool utils_compat_make_dir(const char *path) {
    if (!path || path[0] == '\0') return false;

    int len = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
    if (len <= 0) return false;

    wchar_t *w_path = (wchar_t *)malloc((size_t)len * sizeof(wchar_t));
    if (!w_path) return false;

    MultiByteToWideChar(CP_UTF8, 0, path, -1, w_path, len);

    /* 全新创建成功或目录已存在均视为成功 */
    bool success = (_wmkdir(w_path) == 0 || errno == EEXIST);

    free(w_path);
    return success;
}

/* 安全获取当前工作目录并转换为 UTF-8 */
char *utils_compat_getcwd(char *buffer, size_t size) {
    if (!buffer || size == 0) return NULL;

    wchar_t *w_cwd = _wgetcwd(NULL, 0);
    if (!w_cwd) return NULL;

    int len = WideCharToMultiByte(CP_UTF8, 0, w_cwd, -1,
                                  NULL, 0, NULL, NULL);
    if (len <= 0 || (size_t)len > size) {
        free(w_cwd);
        return NULL;
    }

    WideCharToMultiByte(CP_UTF8, 0, w_cwd, -1, buffer, len, NULL, NULL);
    free(w_cwd);

    return buffer;
}

/* 线程安全本地时间解析：封装 localtime_s */
int utils_compat_localtime_safe(const time_t *timer, struct tm *out_tm) {
    if (!timer || !out_tm) return -1;

    return (localtime_s(out_tm, timer) == 0) ? 0 : -1;
}

/* 跨平台安全删除文件：UTF-8 路径转宽字符后调用 DeleteFileW */
AlgoStatus utils_compat_remove_file(const char *path) {
    if (!path || path[0] == '\0') return ALGO_ERR_ARG;

    int len = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
    if (len <= 0) return ALGO_ERR_FILE;

    wchar_t *w_path = (wchar_t *)malloc((size_t)len * sizeof(wchar_t));
    if (!w_path) return ALGO_ERR_MEM;

    MultiByteToWideChar(CP_UTF8, 0, path, -1, w_path, len);

    BOOL ok = DeleteFileW(w_path);
    free(w_path);

    return ok ? ALGO_SUCCESS : ALGO_ERR_FILE;
}

/* 跨平台编辑器启动：通过 CreateProcessW 绕过 cmd.exe 转义层 */
AlgoStatus utils_compat_open_editor(const char *editor, const char *path) {
    if (!editor || editor[0] == '\0' || !path || path[0] == '\0')
        return ALGO_ERR_ARG;

    /* 构建命令行：编辑器路径 + 空格 + 双引号包裹的文件路径
     * 编辑器本身也可能是含空格的路径（如 C:\Program Files\...），
     * 因此统一对编辑器也用双引号包裹。 */
    char cmd_line[ALGO_MAX_PATH_LEN * 2 + 64];
    int n = snprintf(cmd_line, sizeof(cmd_line), "\"%s\" \"%s\"", editor, path);
    if (n < 0 || (size_t)n >= sizeof(cmd_line)) return ALGO_ERR_RANGE;

    /* 转为宽字符串供 CreateProcessW 使用 */
    int cmd_len = MultiByteToWideChar(CP_UTF8, 0, cmd_line, -1, NULL, 0);
    if (cmd_len <= 0) return ALGO_ERR_UNKNOWN;

    wchar_t *w_cmd = (wchar_t *)malloc((size_t)cmd_len * sizeof(wchar_t));
    if (!w_cmd) return ALGO_ERR_MEM;

    MultiByteToWideChar(CP_UTF8, 0, cmd_line, -1, w_cmd, cmd_len);

    STARTUPINFOW si = {0};
    si.cb           = sizeof(si);
    si.dwFlags      = STARTF_USESHOWWINDOW;
    si.wShowWindow  = SW_SHOWNORMAL;

    PROCESS_INFORMATION pi = {0};

    BOOL ok = CreateProcessW(
        NULL,        /* lpApplicationName — NULL 表示从 lpCommandLine 解析 */
        w_cmd,       /* lpCommandLine — 可执行文件 + 参数 */
        NULL, NULL,  /* 进程/线程安全属性：默认 */
        FALSE,       /* 不继承句柄 */
        0,           /* 创建标志：默认 */
        NULL, NULL,  /* 环境变量/工作目录：继承 */
        &si, &pi);

    free(w_cmd);

    if (!ok) {
        DWORD err = GetLastError();
        if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) {
            return ALGO_ERR_NOT_FOUND;
        }
        return ALGO_ERR_UNKNOWN;
    }

    /* 阻塞等待编辑器退出 */
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exit_code = 0;
    GetExitCodeProcess(pi.hProcess, &exit_code);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return (exit_code == 0) ? ALGO_SUCCESS : ALGO_ERR_UNKNOWN;
}


/* --- 公开接口实现 POSIX (Linux / macOS) ----------------------------------- */

#else

#include <sys/stat.h>
#include <unistd.h>

/* 平台兼容层初始化：POSIX 平台无操作 */
void utils_compat_init(int *argc, char ***argv) {
    (void)argc;
    (void)argv;
}

/* 释放兼容层资源：POSIX 平台无操作 */
void utils_compat_cleanup(void) {
    /* 无动态资源需释放 */
}

/* 跨平台文件打开：POSIX 平台直接透传 fopen */
FILE *utils_compat_fopen(const char *path, const char *mode) {
    return fopen(path, mode);
}

/* 检查物理路径是否存在：POSIX 平台调用 access(F_OK) */
bool utils_compat_path_exists(const char *path) {
    if (!path || path[0] == '\0') return false;

    return access(path, F_OK) == 0;
}

/* 创建目录：mkdir 返回 0 成功；EEXIST 视为幂等成功 */
bool utils_compat_make_dir(const char *path) {
    if (!path || path[0] == '\0') return false;

    return (mkdir(path, 0755) == 0 || errno == EEXIST);
}

/* 安全获取当前工作目录：POSIX 直接调用 getcwd */
char *utils_compat_getcwd(char *buffer, size_t size) {
    return getcwd(buffer, size);
}

/* 线程安全本地时间解析：封装 localtime_r */
int utils_compat_localtime_safe(const time_t *timer, struct tm *out_tm) {
    if (!timer || !out_tm) return -1;

    return localtime_r(timer, out_tm) ? 0 : -1;
}

/* 跨平台安全删除文件：POSIX 平台调用 remove */
AlgoStatus utils_compat_remove_file(const char *path) {
    if (!path || path[0] == '\0') return ALGO_ERR_ARG;

    return (remove(path) == 0) ? ALGO_SUCCESS : ALGO_ERR_FILE;
}

/* 跨平台编辑器启动：POSIX 平台通过 system() 调用 shell 启动编辑器 */
AlgoStatus utils_compat_open_editor(const char *editor, const char *path) {
    if (!editor || editor[0] == '\0' || !path || path[0] == '\0')
        return ALGO_ERR_ARG;

    char cmd[ALGO_MAX_PATH_LEN * 2 + 64];
    int n = snprintf(cmd, sizeof(cmd), "%s \"%s\"", editor, path);
    if (n < 0 || (size_t)n >= sizeof(cmd)) return ALGO_ERR_RANGE;

    int ret = system(cmd);
    if (ret == -1) return ALGO_ERR_UNKNOWN;
    if (ret != 0) return ALGO_ERR_UNKNOWN;
    return ALGO_SUCCESS;
}

#endif


/* --- 公开接口实现 广域跨平台函数 ------------------------------------------ */

/* 统一提取路径中最后一个目录分隔符指针（同时识别 / 和 \） */
char *utils_compat_get_last_slash(char *path) {
    if (!path) return NULL;

    char *slash1 = strrchr(path, '/');
    char *slash2 = strrchr(path, '\\');

    /* 两者均存在时取较靠后的一个 */
    if (slash1 && slash2) {
        return (slash1 > slash2) ? slash1 : slash2;
    }

    return slash1 ? slash1 : slash2;
}

/* 检测 stdout 是否为交互式终端（用于判断是否启用彩色输出） */
bool utils_compat_isatty_stdout(void) {
#ifdef _WIN32
    return _isatty(_fileno(stdout)) != 0;
#else
    return isatty(fileno(stdout)) != 0;
#endif
}

/* 获取当前系统用户的标准 Home 目录路径 */
bool utils_compat_get_user_home(char *buf, size_t size) {
    if (!buf || size == 0) return false;

    const char *home = getenv("HOME");

#ifdef _WIN32
    /* Windows 平台 HOME 不存在时回退到 USERPROFILE */
    if (!home || home[0] == '\0') {
        home = getenv("USERPROFILE");
    }
#endif

    if (!home || home[0] == '\0') {
        return false;
    }

    utils_strcpy_safe(buf, home, size);
    return true;
}
