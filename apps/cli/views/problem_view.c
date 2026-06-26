/**
 * @file problem_view.c
 * @brief 题目视图实现：通过 console_ui 语义角色 API 渲染题目数据。
 *
 * Copyright (c) 2026 Lyriquee
 * SPDX-License-Identifier: MIT
 */

#include "problem_view.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorepo/console/console_ui.h>
#include <algorepo/utils/utils_compat.h>
#include <algorepo/utils/utils_str.h>


/* 列宽常量 */
enum {
    LIST_ID_WIDTH      = 5,
    LIST_TITLE_WIDTH   = 24,
    LIST_DIFF_WIDTH    = 6,
    LIST_MASTERY_WIDTH = 6,
    LIST_TAG_WIDTH     = 18
};


/* 私有函数声明------------------------------------------------------ */

/* UTF-8 解码与显示宽度 */
static bool utf8_next_codepoint(const char **s, uint32_t *cp, size_t *bytes);
static int  codepoint_display_width(uint32_t cp);

/* 字符串级宽度计算 */
static int  utf8_display_width(const char *s);

/* 格式化输出 */
static void truncate_display_str(const char *src, char *dst, size_t dst_size,
                                 int max_width);
static void print_padded_role(ConsoleColorRole role, const char *s, int width);
static void format_date_str(int64_t ts, char *buf, size_t size, bool show_time);


/* 公开接口实现------------------------------------------------------ */

void problem_view_print_difficulty(Difficulty diff) {
    const char        *name;
    ConsoleColorRole   role;
    switch (diff) {
        case ALGO_DIFF_EASY:    name = "容易"; role = COLOR_ROLE_EASY;    break;
        case ALGO_DIFF_MEDIUM:  name = "中等"; role = COLOR_ROLE_MEDIUM;  break;
        case ALGO_DIFF_HARD:    name = "困难"; role = COLOR_ROLE_HARD;    break;
        case ALGO_DIFF_EXTREME: name = "极难"; role = COLOR_ROLE_EXTREME; break;
        default:                name = "未知"; role = COLOR_ROLE_DATE;    break;
    }
    print_padded_role(role, name, LIST_DIFF_WIDTH);
}

void problem_view_print_mastery(Mastery m) {
    const char        *name;
    ConsoleColorRole   role;
    switch (m) {
        case ALGO_MASTERY_LEARNING: name = "了解";   role = COLOR_ROLE_MASTERY_LEARNING; break;
        case ALGO_MASTERY_FAMILIAR: name = "熟悉";   role = COLOR_ROLE_MASTERY_FAMILIAR; break;
        case ALGO_MASTERY_MASTERED: name = "掌握";   role = COLOR_ROLE_MASTERY_MASTERED; break;
        case ALGO_MASTERY_EXPERT:   name = "精通";   role = COLOR_ROLE_MASTERY_EXPERT;   break;
        default:                    name = "未设置"; role = COLOR_ROLE_MASTERY_NONE;     break;
    }
    print_padded_role(role, name, LIST_MASTERY_WIDTH);
}

void problem_view_format_tags(const Problem *p, char *buffer, size_t size) {
    if (!buffer || size == 0) return;
    buffer[0] = '\0';
    if (!p || p->tag_count == 0) {
        utils_strcpy_safe(buffer, "-", size);
        return;
    }
    for (size_t i = 0; i < p->tag_count; i++) {
        if (i > 0) strncat(buffer, ", ", size - strlen(buffer) - 1);
        strncat(buffer, p->tags[i], size - strlen(buffer) - 1);
    }
}

void problem_view_print_row(const Problem *p, bool show_time) {
    char tags_buf[ALGO_MAX_TAGS * (ALGO_MAX_TAG_LEN + 1)] = {0};
    problem_view_format_tags(p, tags_buf, sizeof(tags_buf));

    char title_disp[ALGO_MAX_TITLE_LEN + 4];
    char tags_disp[ALGO_MAX_TAGS * (ALGO_MAX_TAG_LEN + 1)];
    char date_buf[32];

    truncate_display_str(p->title[0] ? p->title : "(未命名)",
                         title_disp, sizeof(title_disp), LIST_TITLE_WIDTH);
    truncate_display_str(tags_buf, tags_disp, sizeof(tags_disp), LIST_TAG_WIDTH);
    format_date_str(p->created_at, date_buf, sizeof(date_buf), show_time);

    console_print_role(COLOR_ROLE_ID,   "%-*d",  LIST_ID_WIDTH, p->id);
    console_print_spaces(2);
    print_padded_role(COLOR_ROLE_TITLE, title_disp, LIST_TITLE_WIDTH);
    console_print_spaces(2);
    problem_view_print_difficulty(p->diff);
    console_print_spaces(2);
    problem_view_print_mastery(p->mastery);
    console_print_spaces(2);
    print_padded_role(COLOR_ROLE_TAG, tags_disp, LIST_TAG_WIDTH);
    console_print_spaces(2);
    console_print_role(COLOR_ROLE_DATE, "%s\n",   date_buf);
}

void problem_view_print_header(bool show_time) {
    console_print_divider();
    print_padded_role(COLOR_ROLE_PRIMARY, "ID",     LIST_ID_WIDTH);
    console_print_spaces(2);
    print_padded_role(COLOR_ROLE_PRIMARY, "标题",   LIST_TITLE_WIDTH);
    console_print_spaces(2);
    print_padded_role(COLOR_ROLE_PRIMARY, "难度",   LIST_DIFF_WIDTH);
    console_print_spaces(2);
    print_padded_role(COLOR_ROLE_PRIMARY, "掌握度", LIST_MASTERY_WIDTH);
    console_print_spaces(2);
    print_padded_role(COLOR_ROLE_PRIMARY, "标签",   LIST_TAG_WIDTH);
    console_print_spaces(2);
    console_print_role(COLOR_ROLE_PRIMARY, "%s\n",
                       show_time ? "日期时间" : "日期");
    console_print_divider();
}

void problem_view_print_list(const ProblemCollection *pcol,
                             const char *empty_msg, bool show_time) {
    if (!pcol || pcol->size == 0) {
        console_log_info("%s", empty_msg ? empty_msg : "没有匹配的题目");
        return;
    }
    problem_view_print_header(show_time);
    for (size_t i = 0; i < pcol->size; i++) {
        problem_view_print_row(&pcol->items[i], show_time);
    }
    console_print_divider();
}

void problem_view_print_detail(const Problem *p) {
    char date_buf[32] = {0};
    char tags[ALGO_MAX_TAGS * (ALGO_MAX_TAG_LEN + 1)] = {0};
    problem_view_format_tags(p, tags, sizeof(tags));
    format_date_str(p->created_at, date_buf, sizeof(date_buf), true);

    console_print_divider();
    console_print_title("                                  题目详情");
    console_print_divider();

    console_print_role(COLOR_ROLE_PRIMARY, "  %-9s",  "ID:");
    console_print_role(COLOR_ROLE_ID,      "  %d\n",  p->id);

    console_print_role(COLOR_ROLE_PRIMARY, "  %-10s   ", "标题:");
    console_print_role(COLOR_ROLE_TITLE,   "%s\n",       p->title[0] ? p->title : "-");

    console_print_role(COLOR_ROLE_PRIMARY, "  %-10s   ", "难度:");
    problem_view_print_difficulty(p->diff);
    console_print_newline();

    console_print_role(COLOR_ROLE_PRIMARY, "  %-10s    ", "掌握度:");
    problem_view_print_mastery(p->mastery);
    console_print_text("\n\n");

    console_print_role(COLOR_ROLE_PRIMARY, "  %-10s ", "标签:");
    console_print_role(COLOR_ROLE_TAG,     "  %s\n",  tags);

    console_print_role(COLOR_ROLE_PRIMARY, "  %-10s ", "更新时间:");
    console_print_role(COLOR_ROLE_DATE,    " %s\n",   date_buf);

    console_print_role(COLOR_ROLE_PRIMARY, "  %-10s ", "源码路径:");
    console_print_role(COLOR_ROLE_VALUE, " %s\n",
                       p->solution_path[0] ? p->solution_path : "-");

    console_print_divider();
}

void problem_view_print_summary(const Problem *p) {
    if (!p) return;

    char tags[ALGO_MAX_TAGS * (ALGO_MAX_TAG_LEN + 1)] = {0};
    problem_view_format_tags(p, tags, sizeof(tags));

    console_print_divider();
    console_print_role(COLOR_ROLE_ID, "[ID:%d] ", p->id);
    console_print_role(COLOR_ROLE_TITLE, "%s", p->title[0] ? p->title : "-");
    console_print_role(COLOR_ROLE_MUTED, " | ");
    console_print_role(COLOR_ROLE_PRIMARY, "难度: ");
    problem_view_print_difficulty(p->diff);
    console_print_role(COLOR_ROLE_MUTED, " | ");
    console_print_role(COLOR_ROLE_PRIMARY, "掌握度: ");
    problem_view_print_mastery(p->mastery);
    console_print_role(COLOR_ROLE_MUTED, " | ");
    console_print_role(COLOR_ROLE_PRIMARY, "标签: ");
    console_print_role(COLOR_ROLE_TAG, "%s\n", tags);
    console_print_divider();
}

void problem_view_print_detail_all(const Problem *p) {
    problem_view_print_detail(p);

    if (p->desc[0])  {
        console_print_primary("  描述:");
        console_print_role(COLOR_ROLE_VALUE, "        %s\n", p->desc);
    }
    if (p->notes[0]) {
        console_print_primary("  笔记:");
        console_print_role(COLOR_ROLE_VALUE, "        %s\n", p->notes);
    }
}

void problem_view_print_code(const Problem *p, int head_lines, int tail_lines) {
    if (!p || p->solution_path[0] == '\0') {
        console_log_warn("源文件路径为空");
        return;
    }
    FILE *fp = utils_compat_fopen(p->solution_path, "r");
    if (!fp) {
        console_log_warn("无法打开源文件: %s", p->solution_path);
        return;
    }

    console_print_divider();
    console_print_title("                                   源码内容");
    console_print_divider();

    if (head_lines > 0 && tail_lines > 0) {
        /* 读入全文，打印首尾区间 */
        char **lines = NULL;
        size_t cap = 0, cnt = 0;
        char   lb[4096];

        while (fgets(lb, sizeof(lb), fp)) {
            if (cnt >= cap) {
                size_t nc = cap ? cap * 2 : 256;
                char **t = realloc(lines, nc * sizeof(char *));
                if (!t) break;
                lines = t;
                cap = nc;
            }
            lines[cnt] = strdup(lb);
            if (lines[cnt]) cnt++;
        }

        if (cnt > 0) {
            int sh = head_lines < (int)cnt ? head_lines : (int)cnt;
            for (int i = 0; i < sh; i++) {
                console_print_role(COLOR_ROLE_MUTED, "%-4d ", i + 1);
                console_print_role(COLOR_ROLE_VALUE, "%s", lines[i]);
            }

            if (tail_lines > 0 && (int)cnt > head_lines + tail_lines)
                console_print_role(COLOR_ROLE_MUTED, "  ... (%llu lines)\n",
                                   (unsigned long long)(cnt - head_lines - tail_lines));

            int st = (int)cnt - tail_lines;
            if (st < head_lines) st = head_lines;
            for (int i = st; i < (int)cnt; i++) {
                console_print_role(COLOR_ROLE_MUTED, "%-4d ", i + 1);
                console_print_role(COLOR_ROLE_VALUE, "%s", lines[i]);
            }
        }

        for (size_t i = 0; i < cnt; i++) free(lines[i]);
        free(lines);
    } else if (head_lines > 0) {
        char lb[4096];
        int  n = 0;
        while (n < head_lines && fgets(lb, sizeof(lb), fp)) {
            console_print_role(COLOR_ROLE_MUTED, "%-4d ", ++n);
            console_print_role(COLOR_ROLE_VALUE, "%s", lb);
        }
    } else if (tail_lines > 0) {
        /* 环形缓冲区保留最后 N 行 */
        char **ring = calloc((size_t)tail_lines, sizeof(char *));
        int    pos = 0, total = 0;
        char   lb[4096];

        while (fgets(lb, sizeof(lb), fp)) {
            free(ring[pos]);
            ring[pos] = strdup(lb);
            pos = (pos + 1) % tail_lines;
            total++;
        }

        int start = total < tail_lines ? 0 : pos;
        int count = total < tail_lines ? total : tail_lines;
        for (int i = 0; i < count; i++) {
            int idx = (start + i) % tail_lines;
            if (ring[idx]) {
                console_print_role(COLOR_ROLE_MUTED, "%-4d ",
                                   total - count + i + 1);
                console_print_role(COLOR_ROLE_VALUE, "%s", ring[idx]);
            }
        }

        for (int i = 0; i < tail_lines; i++) free(ring[i]);
        free(ring);
    }

    console_print_divider();
    fclose(fp);
}


/* --- 私有函数实现---------------------------------------------------------- */

static bool utf8_next_codepoint(const char **s, uint32_t *cp, size_t *bytes) {
    const unsigned char *p = (const unsigned char *)*s;
    if (!p || *p == '\0') return false;

    // ASCII
    if (p[0] < 0x80) {
        *cp    = p[0];
        *bytes = 1;
    }
    // 2-byte: 110xxxxx 10xxxxxx
    else if ((p[0] & 0xE0) == 0xC0 && p[1] != '\0' &&
             (p[1] & 0xC0) == 0x80) {
        *cp    = ((uint32_t)(p[0] & 0x1F) << 6) |
                 (uint32_t)(p[1] & 0x3F);
        *bytes = 2;
    }
    // 3-byte: 1110xxxx 10xxxxxx 10xxxxxx
    else if ((p[0] & 0xF0) == 0xE0 &&
             p[1] != '\0' && p[2] != '\0' &&
             (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80) {
        *cp    = ((uint32_t)(p[0] & 0x0F) << 12) |
                 ((uint32_t)(p[1] & 0x3F) << 6)  |
                 (uint32_t)(p[2] & 0x3F);
        *bytes = 3;
    }
    // 4-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    else if ((p[0] & 0xF8) == 0xF0 &&
             p[1] != '\0' && p[2] != '\0' && p[3] != '\0' &&
             (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80 &&
             (p[3] & 0xC0) == 0x80) {
        *cp    = ((uint32_t)(p[0] & 0x07) << 18) |
                 ((uint32_t)(p[1] & 0x3F) << 12) |
                 ((uint32_t)(p[2] & 0x3F) << 6)  |
                 (uint32_t)(p[3] & 0x3F);
        *bytes = 4;
    }
    // Illegal byte: pass through as-is
    else {
        *cp    = p[0];
        *bytes = 1;
    }

    *s += *bytes;
    return true;
}

static int codepoint_display_width(uint32_t cp) {
    if (cp == 0) return 0;

    // C0 控制字符 (0x00-0x1F) 及 DEL + C1 (0x7F-0x9F)
    if (cp < 32 || (cp >= 0x7F && cp < 0xA0))
        return 0;

    // 组合变音标记 (Combining Diacritical Marks / Extensions) 
    if ((cp >= 0x0300  && cp <= 0x036F)  ||
        (cp >= 0x1AB0  && cp <= 0x1AFF)  ||
        (cp >= 0x1DC0  && cp <= 0x1DFF)  ||
        (cp >= 0x20D0  && cp <= 0x20FF)  ||
        (cp >= 0xFE20  && cp <= 0xFE2F))
        return 0;

    /* 宽字符：CJK / 韩文 / 全角 / 表情符号 / 表意文字扩展 */
    if ((cp >= 0x1100  && cp <= 0x115F)  ||
        (cp >= 0x2329  && cp <= 0x232A)  ||
        (cp >= 0x2E80  && cp <= 0xA4CF)  ||
        (cp >= 0xAC00  && cp <= 0xD7A3)  ||
        (cp >= 0xF900  && cp <= 0xFAFF)  ||
        (cp >= 0xFE10  && cp <= 0xFE19)  ||
        (cp >= 0xFE30  && cp <= 0xFE6F)  ||
        (cp >= 0xFF00  && cp <= 0xFF60)  ||
        (cp >= 0xFFE0  && cp <= 0xFFE6)  ||
        (cp >= 0x1F300 && cp <= 0x1FAFF) ||
        (cp >= 0x20000 && cp <= 0x3FFFD))
        return 2;

    return 1;
}

static int utf8_display_width(const char *s) {
    if (!s) return 0;

    int width = 0;
    while (*s) {
        uint32_t cp    = 0;
        size_t   bytes = 0;
        if (!utf8_next_codepoint(&s, &cp, &bytes)) break;
        width += codepoint_display_width(cp);
    }
    return width;
}

static void truncate_display_str(const char *src, char *dst, size_t dst_size,
                                 int max_width) {
    if (!src || !dst || dst_size == 0) return;
    dst[0] = '\0';
    if (max_width <= 0) return;

    /* 整体宽度不超限则直接拷贝 */
    if (utf8_display_width(src) <= max_width) {
        utils_strcpy_safe(dst, src, dst_size);
        return;
    }

    const int ellipsis_width = max_width > 3 ? 3 : 0;
    int       used           = 0;
    size_t    out            = 0;
    const char *cur          = src;

    while (*cur && out + 1 < dst_size) {
        const char *before = cur;
        uint32_t    cp     = 0;
        size_t      bytes  = 0;
        if (!utf8_next_codepoint(&cur, &cp, &bytes) || bytes == 0) break;

        int char_width = codepoint_display_width(cp);
        if (used + char_width > max_width - ellipsis_width) break;
        if (out + bytes >= dst_size) break;

        memcpy(dst + out, before, bytes);
        out  += bytes;
        used += char_width;
    }

    dst[out] = '\0';
    if (ellipsis_width > 0 && out + 4 <= dst_size)
        strcat(dst, "...");   /* 3 ASCII dots + null = 4 bytes */
}

static void print_padded_role(ConsoleColorRole role, const char *s, int width) {
    if (!s) s = "";
    console_print_role(role, "%s", s);
    int pad = width - utf8_display_width(s);
    while (pad-- > 0) putchar(' ');
}

static void format_date_str(int64_t ts, char *buf, size_t size, bool show_time) {
    if (ts <= 0) {
        utils_strcpy_safe(buf, "-", size);
        return;
    }
    time_t t = (time_t)ts;
    struct tm tm_info;
    if (utils_compat_localtime_safe(&t, &tm_info) != 0) {
        utils_strcpy_safe(buf, "-", size);
        return;
    }
    strftime(buf, size, show_time ? "%Y-%m-%d %H:%M" : "%Y-%m-%d", &tm_info);
}
