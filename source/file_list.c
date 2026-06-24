#include "file_list.h"

#include <3ds.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ui.h"

bool ends_with_ci(const char *s, const char *suf) {
    size_t ls = strlen(s), lf = strlen(suf);
    if (lf > ls) return false;
    for (size_t i = 0; i < lf; i++) {
        char a = s[ls - lf + i], b = suf[i];
        if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
        if (a != b) return false;
    }
    return true;
}

static void sort_files(file_list_t *fl) {
    for (int i = 1; i < fl->count; i++) {
        char nm[MAX_NAME_LEN]; off_t sz;
        snprintf(nm, sizeof(nm), "%s", fl->names[i]); sz = fl->sizes[i];
        int j = i - 1;
        while (j >= 0 && strcmp(fl->names[j], nm) > 0) {
            snprintf(fl->names[j+1], MAX_NAME_LEN, "%s", fl->names[j]);
            fl->sizes[j+1] = fl->sizes[j];
            j--;
        }
        snprintf(fl->names[j+1], MAX_NAME_LEN, "%s", nm);
        fl->sizes[j+1] = sz;
    }
}

static void scan_dir(file_list_t *fl, const char *dir,
                     const char *ext1, const char *ext2,
                     off_t expected_size)
{
    DIR *d = opendir(dir);
    if (!d) return;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (fl->count >= MAX_FILES) break;
        if (ent->d_name[0] == '.') continue;
        bool match = false;
        if (ext1 && ends_with_ci(ent->d_name, ext1)) match = true;
        if (ext2 && ends_with_ci(ent->d_name, ext2)) match = true;
        if (!match) continue;
        char full[MAX_NAME_LEN * 2 + 8];
        snprintf(full, sizeof(full), "%s/%s", dir, ent->d_name);
        struct stat st;
        if (stat(full, &st) != 0 || !S_ISREG(st.st_mode)) continue;
        if (st.st_size == 0) continue;
        if (expected_size > 0 && st.st_size != expected_size) continue;
        snprintf(fl->names[fl->count], MAX_NAME_LEN, "%s", ent->d_name);
        fl->sizes[fl->count] = st.st_size;
        fl->count++;
    }
    closedir(d);
}

bool build_file_list(file_list_t *fl, const char *primary,
                     const char *const *fallbacks, int n_fallbacks,
                     const char *ext1, const char *ext2,
                     off_t expected_size)
{
    memset(fl, 0, sizeof(*fl));
    snprintf(fl->dir, sizeof(fl->dir), "%s", primary);
    scan_dir(fl, fl->dir, ext1, ext2, expected_size);
    for (int i = 0; i < n_fallbacks && fl->count == 0; i++) {
        snprintf(fl->dir, sizeof(fl->dir), "%s", fallbacks[i]);
        scan_dir(fl, fl->dir, ext1, ext2, expected_size);
    }
    sort_files(fl);
    return fl->count > 0;
}

void file_full_path(const file_list_t *fl, int idx, char *out, size_t cap) {
    snprintf(out, cap, "%s/%s", fl->dir, fl->names[idx]);
}

bool build_canonical_list(file_list_t *fl, const char *primary,
                          const char *const *fallbacks, int n_fallbacks,
                          const char *ext, off_t expected_size)
{
    if (!build_file_list(fl, primary, fallbacks, n_fallbacks, ext, NULL,
                          expected_size)) return false;
    /* In-place filter: keep only entries where classify_backup == 0. */
    int w = 0;
    for (int i = 0; i < fl->count; i++) {
        char c[MAX_NAME_LEN];
        if (classify_backup(fl->names[i], c, sizeof c) != 0) continue;
        if (w != i) {
            snprintf(fl->names[w], MAX_NAME_LEN, "%s", fl->names[i]);
            fl->sizes[w] = fl->sizes[i];
        }
        w++;
    }
    fl->count = w;
    return fl->count > 0;
}

int file_list_find(const file_list_t *fl, const char *full_path) {
    if (!full_path || !*full_path) return -1;
    char tmp[MAX_NAME_LEN * 2 + 8];
    for (int i = 0; i < fl->count; i++) {
        snprintf(tmp, sizeof tmp, "%s/%s", fl->dir, fl->names[i]);
        if (strcmp(tmp, full_path) == 0) return i;
    }
    return -1;
}

int run_file_picker(const char *title, file_list_t *fl) {
    if (fl->count <= 0) {
        while (aptMainLoop()) {
            hidScanInput();
            u32 d = hidKeysDown();
            if (d & (KEY_B | KEY_A | KEY_START)) return -1;
            frame_begin();
            draw_title(title);
            draw_text(MARGIN_X, 40, 0.5f, COL_ERR, "No files found.");
            draw_text(MARGIN_X, 64, 0.45f, COL_TEXT, "Dir checked:");
            draw_text(MARGIN_X + 10, 80, 0.45f, COL_DIM, fl->dir);
            draw_text(MARGIN_X, 100, 0.45f, COL_TEXT,
                      "Copy your file there and retry.");
            draw_footer("(B) back");
            frame_end();
        }
        return -1;
    }
    while (aptMainLoop()) {
        hidScanInput();
        u32 d = hidKeysDown();
        if (d & KEY_START) return -2;
        if (d & KEY_B)     return -1;
        if (d & KEY_DUP)   fl->cursor = (fl->cursor - 1 + fl->count) % fl->count;
        if (d & KEY_DDOWN) fl->cursor = (fl->cursor + 1) % fl->count;
        if (d & KEY_DLEFT)  fl->cursor = (fl->cursor > 5 ? fl->cursor - 5 : 0);
        if (d & KEY_DRIGHT) fl->cursor = (fl->cursor + 5 < fl->count
                                          ? fl->cursor + 5 : fl->count - 1);
        if (d & KEY_A)     return fl->cursor;

        if (fl->cursor < fl->scroll) fl->scroll = fl->cursor;
        if (fl->cursor >= fl->scroll + VIS_ROWS)
            fl->scroll = fl->cursor - VIS_ROWS + 1;

        frame_begin();
        draw_title(title);
        draw_textf(MARGIN_X, 28, 0.4f, COL_DIM, "Dir: %s", fl->dir);
        draw_textf(MARGIN_X, 42, 0.4f, COL_DIM, "%d file(s)", fl->count);
        int end = fl->scroll + VIS_ROWS;
        if (end > fl->count) end = fl->count;
        float y = 60;
        for (int i = fl->scroll; i < end; i++) {
            if (i == fl->cursor) {
                draw_frame(0, y - 2, TOP_W, LINE_H + 2, 2, COL_HILITE);
                draw_text(MARGIN_X + 4, y, 0.45f, COL_HIFG, fl->names[i]);
            } else {
                draw_text(MARGIN_X + 4, y, 0.45f, COL_TEXT, fl->names[i]);
            }
            y += LINE_H + 2;
        }
        draw_footer("(A) select  (B) back  (DPad) move");
        frame_end();
    }
    return -2;
}

int classify_backup(const char *name, char *out_canon, size_t cap) {
    size_t n = strlen(name);
    if (n > 4 && !strcmp(name + n - 4, ".bak")) {
        size_t copy_n = n - 4;
        if (copy_n + 1 > cap) return 0;
        memcpy(out_canon, name, copy_n);
        out_canon[copy_n] = 0;
        return 1;
    }
    const char *last_dot = strrchr(name, '.');
    if (!last_dot || last_dot == name) { snprintf(out_canon, cap, "%s", name); return 0; }
    if (last_dot - name < 16)            { snprintf(out_canon, cap, "%s", name); return 0; }
    const char *ts_dot = last_dot - 16;
    if (*ts_dot != '.') { snprintf(out_canon, cap, "%s", name); return 0; }
    for (int i = 1; i <= 15; i++) {
        char c = ts_dot[i];
        if (i == 9) { if (c != '-') { snprintf(out_canon, cap, "%s", name); return 0; } }
        else        { if (c < '0' || c > '9') { snprintf(out_canon, cap, "%s", name); return 0; } }
    }
    size_t prefix = (size_t)(ts_dot - name);
    size_t suffix = strlen(last_dot);
    if (prefix + suffix + 1 > cap) { snprintf(out_canon, cap, "%s", name); return 0; }
    memcpy(out_canon, name, prefix);
    memcpy(out_canon + prefix, last_dot, suffix);
    out_canon[prefix + suffix] = 0;
    return 2;
}

bool scan_all_files(file_list_t *fl, const char *dir, off_t expected_size) {
    memset(fl, 0, sizeof(*fl));
    snprintf(fl->dir, sizeof(fl->dir), "%s", dir);
    DIR *d = opendir(dir);
    if (!d) return false;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (fl->count >= MAX_FILES) break;
        if (ent->d_name[0] == '.') continue;
        size_t ln = strlen(ent->d_name);
        bool ok = (ln > 4 && (
                   ends_with_ci(ent->d_name, ".rom") ||
                   ends_with_ci(ent->d_name, ".sav") ||
                   ends_with_ci(ent->d_name, ".bak")));
        if (!ok) continue;
        char full[MAX_NAME_LEN * 2 + 8];
        snprintf(full, sizeof(full), "%s/%s", dir, ent->d_name);
        struct stat st;
        if (stat(full, &st) != 0 || !S_ISREG(st.st_mode)) continue;
        if (st.st_size == 0) continue;
        if (expected_size > 0 && st.st_size != expected_size) continue;
        snprintf(fl->names[fl->count], MAX_NAME_LEN, "%s", ent->d_name);
        fl->sizes[fl->count] = st.st_size;
        fl->count++;
    }
    closedir(d);
    sort_files(fl);
    return fl->count > 0;
}

int run_backup_picker(const char *title, file_list_t *fl) {
    if (fl->count <= 0) {
        while (aptMainLoop()) {
            hidScanInput();
            if (hidKeysDown() & (KEY_B | KEY_A | KEY_START)) return -1;
            frame_begin();
            draw_title(title);
            draw_textf(MARGIN_X, 60, 0.5f, COL_ERR, "No files in: %s", fl->dir);
            draw_footer("(B) back");
            frame_end();
        }
        return -1;
    }
    while (aptMainLoop()) {
        hidScanInput();
        u32 d = hidKeysDown();
        if (d & KEY_START) return -2;
        if (d & KEY_B)     return -1;
        if (d & KEY_DUP)   fl->cursor = (fl->cursor - 1 + fl->count) % fl->count;
        if (d & KEY_DDOWN) fl->cursor = (fl->cursor + 1) % fl->count;
        if (d & KEY_DLEFT)  fl->cursor = (fl->cursor > 5 ? fl->cursor - 5 : 0);
        if (d & KEY_DRIGHT) fl->cursor = (fl->cursor + 5 < fl->count
                                          ? fl->cursor + 5 : fl->count - 1);
        if (d & KEY_A)     return fl->cursor;

        if (fl->cursor < fl->scroll) fl->scroll = fl->cursor;
        if (fl->cursor >= fl->scroll + VIS_ROWS)
            fl->scroll = fl->cursor - VIS_ROWS + 1;

        frame_begin();
        draw_title(title);
        draw_textf(MARGIN_X, 28, 0.4f, COL_DIM, "Dir: %s", fl->dir);
        draw_textf(MARGIN_X, 42, 0.4f, COL_DIM, "%d file(s)", fl->count);
        int end = fl->scroll + VIS_ROWS;
        if (end > fl->count) end = fl->count;
        float y = 60;
        for (int i = fl->scroll; i < end; i++) {
            char canon[MAX_NAME_LEN];
            int cls = classify_backup(fl->names[i], canon, sizeof(canon));
            const char *tag = cls == 1 ? "[bak]" : cls == 2 ? "[ts] " : "[ok] ";
            u32 tag_col = cls == 1 ? COL_OK : cls == 2 ? COL_HILITE : COL_DIM;
            if (i == fl->cursor) {
                draw_frame(0, y - 2, TOP_W, LINE_H + 2, 2, COL_HILITE);
                draw_text(MARGIN_X + 4,  y, 0.42f, COL_HIFG, tag);
                draw_text(MARGIN_X + 50, y, 0.42f, COL_HIFG, fl->names[i]);
            } else {
                draw_text(MARGIN_X + 4,  y, 0.42f, tag_col, tag);
                draw_text(MARGIN_X + 50, y, 0.42f, COL_TEXT, fl->names[i]);
            }
            y += LINE_H + 2;
        }
        draw_footer("(A) actions  (B) back");
        frame_end();
    }
    return -2;
}
