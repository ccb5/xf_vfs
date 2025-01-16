/**
 * @file xf_vfs.c
 * @author catcatBlue (catcatblue@qq.com)
 * @brief
 * @version 1.0
 * @date 2025-01-13
 */

/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Copyright (c) 2024, CorAL.
 * This file has been modified by CorAL under the terms of the Apache License, Version 2.0.
 *
 * Modifications:
 * - Modified by CorAL on 2025-01-10:
 *   1. modified the naming to prevent conflict with the original project.
 *   2. Remove posix docking, compatible with other platforms.
 *   3. removed esp-idf related dependencies.
 *   4. trimmed termios and other functions.
 */

/* ==================== [Includes] ========================================== */

#include "xf_vfs.h"
#include "xf_vfs_private.h"

/* ==================== [Defines] =========================================== */

#define FD_TABLE_ENTRY_UNUSED   (fd_table_t) { .permanent = false, .has_pending_close = false, .has_pending_select = false, .vfs_index = -1, .local_fd = -1 }

#if !defined(STATIC_ASSERT)
#   define STATIC_ASSERT(EXPR, ...)     extern char (*_do_assert(void)) [sizeof(char[1 - 2*!(EXPR)])]
#endif

#define _lock_acquire(lock)             xf_lock_lock(lock)
#define _lock_release(lock)             xf_lock_unlock(lock)

#if !defined(xf_strncpy)
#   define xf_strncpy(dst, src, len)    strncpy((dst), (src), (len))
#endif

/* ==================== [Typedefs] ========================================== */

#if ((1 << (1 /* byte */ * 8)) >= XF_VFS_FDS_MAX)
#   define _LOCAL_FD_T_     uint8_t
#else
#   define _LOCAL_FD_T_     uint16_t
#endif

typedef _LOCAL_FD_T_ local_fd_t;
STATIC_ASSERT((1 << (sizeof(local_fd_t) * 8)) >= XF_VFS_FDS_MAX, "file descriptor type too small");

typedef int8_t vfs_index_t;
STATIC_ASSERT((1 << (sizeof(vfs_index_t) * 8)) >= XF_VFS_MAX_COUNT, "VFS index type too small");
STATIC_ASSERT(((vfs_index_t) -1) < 0, "vfs_index_t must be a signed type");

typedef struct {
    bool permanent : 1;
    bool has_pending_close : 1;
    bool has_pending_select : 1;
    uint8_t _reserved : 5;
    vfs_index_t vfs_index;
    local_fd_t local_fd;
} fd_table_t;

typedef struct {
    bool isset; // none or at least one bit is set in the following 3 fd sets
    xf_fd_set readfds;
    xf_fd_set writefds;
    xf_fd_set errorfds;
} fds_triple_t;

typedef struct {
#if XF_VFS_SUPPORT_DIR_IS_ENABLE
    xf_vfs_dir_ops_t *dir;
#endif
#if XF_VFS_SUPPORT_SELECT_IS_ENABLE
    xf_vfs_select_ops_t *select;
#endif
} vfs_component_proxy_t;

/* ==================== [Static Prototypes] ================================= */

static xf_vfs_ssize_t xf_get_free_index(void);
static void xf_vfs_free_fs_ops(xf_vfs_fs_ops_t *vfs);
static void xf_vfs_free_entry(xf_vfs_entry_t *entry);
static xf_vfs_fs_ops_t *xf_minify_vfs(const xf_vfs_t *const vfs, vfs_component_proxy_t proxy);
static void free_proxy_members(vfs_component_proxy_t *proxy);
static xf_vfs_fs_ops_t *xf_vfs_duplicate_fs_ops(const xf_vfs_fs_ops_t *orig);
static xf_err_t xf_vfs_make_fs_ops(const xf_vfs_t *vfs, xf_vfs_fs_ops_t **min);
static xf_err_t xf_vfs_register_fs_common(
    const char *base_path, size_t len, const xf_vfs_fs_ops_t *vfs, int flags, void *ctx, int *vfs_index);
static inline bool fd_valid(int fd);
static const xf_vfs_entry_t *get_vfs_for_fd(int fd);
static inline int get_local_fd(const xf_vfs_entry_t *vfs, int fd);
static const char *translate_path(const xf_vfs_entry_t *vfs, const char *src_path);

/* ==================== [Static Variables] ================================== */

static const char *const TAG = "xf_vfs";

static xf_vfs_entry_t *s_vfs[XF_VFS_MAX_COUNT] = { 0 };
static size_t s_vfs_count = 0;

static fd_table_t s_fd_table[XF_VFS_FDS_MAX] = { [0 ... XF_VFS_FDS_MAX - 1] = FD_TABLE_ENTRY_UNUSED };
static xf_lock_t s_fd_table_lock;

/* ==================== [Macros] ============================================ */

/* ==================== [Global Functions] ================================== */

xf_err_t xf_vfs_register_fs(const char *base_path, const xf_vfs_fs_ops_t *vfs, int flags, void *ctx)
{
    if (s_fd_table_lock == NULL) {
        xf_lock_init(&s_fd_table_lock);
    }

    if (vfs == NULL) {
        XF_LOGE(TAG, "VFS is NULL");
        return XF_ERR_INVALID_ARG;
    }

    if ((flags & XF_VFS_FLAG_STATIC)) {
        return xf_vfs_register_fs_common(base_path, xf_strlen(base_path), vfs, flags, ctx, NULL);
    }

    xf_vfs_fs_ops_t *_vfs = xf_vfs_duplicate_fs_ops(vfs);
    if (_vfs == NULL) {
        return XF_ERR_NO_MEM;
    }

    xf_err_t ret = xf_vfs_register_fs_common(base_path, xf_strlen(base_path), _vfs, flags, ctx, NULL);
    if (ret != XF_OK) {
        xf_vfs_free_fs_ops(_vfs);
        return ret;
    }

    return XF_OK;
}

xf_err_t xf_vfs_register_common(const char *base_path, size_t len, const xf_vfs_t *vfs, void *ctx, int *vfs_index)
{
    if (s_fd_table_lock == NULL) {
        xf_lock_init(&s_fd_table_lock);
    }

    if (vfs == NULL) {
        XF_LOGE(TAG, "VFS is NULL");
        return XF_ERR_INVALID_ARG;
    }

    if (vfs->flags & XF_VFS_FLAG_STATIC) {
        XF_LOGE(TAG, "XF_VFS_FLAG_STATIC is not supported for xf_vfs_t, use xf_vfs_register_fs instead");
        return XF_ERR_INVALID_ARG;
    }

    xf_vfs_fs_ops_t *_vfs = NULL;
    xf_err_t ret = xf_vfs_make_fs_ops(vfs, &_vfs);
    if (ret != XF_OK) {
        return ret;
    }

    ret = xf_vfs_register_fs_common(base_path, len, _vfs, vfs->flags, ctx, vfs_index);
    if (ret != XF_OK) {
        xf_vfs_free_fs_ops(_vfs);
        return ret;
    }

    return XF_OK;
}

xf_err_t xf_vfs_register(const char *base_path, const xf_vfs_t *vfs, void *ctx)
{
    return xf_vfs_register_common(base_path, xf_strlen(base_path), vfs, ctx, NULL);
}

xf_err_t xf_vfs_register_fd_range(const xf_vfs_t *vfs, void *ctx, int min_fd, int max_fd)
{
    if (min_fd < 0 || max_fd < 0 || min_fd > XF_VFS_FDS_MAX || max_fd > XF_VFS_FDS_MAX || min_fd > max_fd) {
        XF_LOGD(TAG, "Invalid arguments: xf_vfs_register_fd_range(0x%x, 0x%x, %d, %d)", (int)(uintptr_t)vfs,
                (int)(uintptr_t)ctx, min_fd, max_fd);
        return XF_ERR_INVALID_ARG;
    }

    int index = 0;
    xf_err_t ret = xf_vfs_register_common("", XF_VFS_PATH_PREFIX_LEN_IGNORED, vfs, ctx, &index);

    if (ret == XF_OK) {
        _lock_acquire(s_fd_table_lock);
        for (int i = min_fd; i < max_fd; ++i) {
            if (s_fd_table[i].vfs_index != -1) {
                xf_free(s_vfs[index]);
                s_vfs[index] = NULL;
                for (int j = min_fd; j < i; ++j) {
                    if (s_fd_table[j].vfs_index == index) {
                        s_fd_table[j] = FD_TABLE_ENTRY_UNUSED;
                    }
                }
                _lock_release(s_fd_table_lock);
                XF_LOGD(TAG, "xf_vfs_register_fd_range cannot set fd %d (used by other VFS)", i);
                return XF_ERR_INVALID_ARG;
            }
            s_fd_table[i].permanent = true;
            s_fd_table[i].vfs_index = index;
            s_fd_table[i].local_fd = i;
        }
        _lock_release(s_fd_table_lock);

        XF_LOGW(TAG, "xf_vfs_register_fd_range is successful for range <%d; %d) and VFS ID %d", min_fd, max_fd, index);
    }

    return ret;
}

xf_err_t xf_vfs_register_fs_with_id(const xf_vfs_fs_ops_t *vfs, int flags, void *ctx, xf_vfs_id_t *vfs_id)
{
    if (vfs_id == NULL) {
        return XF_ERR_INVALID_ARG;
    }

    *vfs_id = -1;
    return xf_vfs_register_fs_common("", XF_VFS_PATH_PREFIX_LEN_IGNORED, vfs, flags, ctx, vfs_id);
}

xf_err_t xf_vfs_register_with_id(const xf_vfs_t *vfs, void *ctx, xf_vfs_id_t *vfs_id)
{
    if (vfs_id == NULL) {
        return XF_ERR_INVALID_ARG;
    }

    *vfs_id = -1;
    return xf_vfs_register_common("", XF_VFS_PATH_PREFIX_LEN_IGNORED, vfs, ctx, vfs_id);
}

xf_err_t xf_vfs_unregister_with_id(xf_vfs_id_t vfs_id)
{
    if (vfs_id < 0 || vfs_id >= XF_VFS_MAX_COUNT || s_vfs[vfs_id] == NULL) {
        return XF_ERR_INVALID_ARG;
    }
    xf_vfs_entry_t *vfs = s_vfs[vfs_id];
    xf_vfs_free_entry(vfs);
    s_vfs[vfs_id] = NULL;

    _lock_acquire(s_fd_table_lock);
    // Delete all references from the FD lookup-table
    for (int j = 0; j < XF_VFS_MAX_COUNT; ++j) {
        if (s_fd_table[j].vfs_index == vfs_id) {
            s_fd_table[j] = FD_TABLE_ENTRY_UNUSED;
        }
    }
    _lock_release(s_fd_table_lock);

    return XF_OK;

}

xf_err_t xf_vfs_unregister_fs_with_id(xf_vfs_id_t vfs_id)
{
    return xf_vfs_unregister_with_id(vfs_id);
}

xf_err_t xf_vfs_unregister(const char *base_path)
{
    const size_t base_path_len = xf_strlen(base_path);
    for (size_t i = 0; i < s_vfs_count; ++i) {
        xf_vfs_entry_t *vfs = s_vfs[i];
        if (vfs == NULL) {
            continue;
        }
        if (base_path_len == vfs->path_prefix_len &&
                xf_memcmp(base_path, vfs->path_prefix, vfs->path_prefix_len) == 0) {
            return xf_vfs_unregister_with_id(i);
        }
    }
    return XF_ERR_INVALID_STATE;
}

xf_err_t xf_vfs_unregister_fs(const char *base_path)
{
    return xf_vfs_unregister(base_path);
}

xf_err_t xf_vfs_register_fd(xf_vfs_id_t vfs_id, int *fd)
{
    return xf_vfs_register_fd_with_local_fd(vfs_id, -1, true, fd);
}

xf_err_t xf_vfs_register_fd_with_local_fd(xf_vfs_id_t vfs_id, int local_fd, bool permanent, int *fd)
{
    if (vfs_id < 0 || vfs_id >= s_vfs_count || fd == NULL) {
        XF_LOGD(TAG, "Invalid arguments for xf_vfs_register_fd_with_local_fd(%d, %d, %d, 0x%p)",
                vfs_id, local_fd, permanent, fd);
        return XF_ERR_INVALID_ARG;
    }

    xf_err_t ret = XF_ERR_NO_MEM;
    _lock_acquire(s_fd_table_lock);
    for (int i = 0; i < XF_VFS_FDS_MAX; ++i) {
        if (s_fd_table[i].vfs_index == -1) {
            s_fd_table[i].permanent = permanent;
            s_fd_table[i].vfs_index = vfs_id;
            if (local_fd >= 0) {
                s_fd_table[i].local_fd = local_fd;
            } else {
                s_fd_table[i].local_fd = i;
            }
            *fd = i;
            ret = XF_OK;
            break;
        }
    }
    _lock_release(s_fd_table_lock);

    XF_LOGD(TAG, "xf_vfs_register_fd_with_local_fd(%d, %d, %d, 0x%p) finished with %s",
            vfs_id, local_fd, permanent, fd, xf_err_to_name(ret));

    return ret;
}

xf_err_t xf_vfs_unregister_fd(xf_vfs_id_t vfs_id, int fd)
{
    xf_err_t ret = XF_ERR_INVALID_ARG;

    if (vfs_id < 0 || vfs_id >= s_vfs_count || fd < 0 || fd >= XF_VFS_FDS_MAX) {
        XF_LOGD(TAG, "Invalid arguments for xf_vfs_unregister_fd(%d, %d)", vfs_id, fd);
        return ret;
    }

    _lock_acquire(s_fd_table_lock);
    fd_table_t *item = s_fd_table + fd;
    if (item->permanent == true && item->vfs_index == vfs_id && item->local_fd == fd) {
        *item = FD_TABLE_ENTRY_UNUSED;
        ret = XF_OK;
    }
    _lock_release(s_fd_table_lock);

    XF_LOGD(TAG, "xf_vfs_unregister_fd(%d, %d) finished with %s", vfs_id, fd, xf_err_to_name(ret));

    return ret;
}

void xf_vfs_dump_fds(void)
{
    const xf_vfs_entry_t *vfs;
    xf_log_printf("------------------------------------------------------\n");
    xf_log_printf("<VFS Path Prefix>-<FD seen by App>-<FD seen by driver>\n");
    xf_log_printf("------------------------------------------------------\n");
    _lock_acquire(s_fd_table_lock);
    for (int index = 0; index < XF_VFS_FDS_MAX; index++) {
        if (s_fd_table[index].vfs_index != -1) {
            vfs = s_vfs[s_fd_table[index].vfs_index];
            if (xf_strcmp(vfs->path_prefix, "")) {
                xf_log_printf("(%s) - 0x%x - 0x%x\n", vfs->path_prefix, index, s_fd_table[index].local_fd);
            } else {
                xf_log_printf("(socket) - 0x%x - 0x%x\n", index, s_fd_table[index].local_fd);
            }
        }
    }
    _lock_release(s_fd_table_lock);
}

void xf_vfs_dump_registered_paths(void)
{
    xf_log_printf("------------------------------------------------------\n");
    xf_log_printf("<index>:<VFS Path Prefix> -> <VFS entry ptr>\n");
    xf_log_printf("------------------------------------------------------\n");
    for (int i = 0; i < XF_VFS_MAX_COUNT; ++i) {
        xf_log_printf(
            "%d:%s -> %p\n",
            (int)i,
            s_vfs[i] ? s_vfs[i]->path_prefix : "NULL",
            s_vfs[i] ? s_vfs[i]->vfs : NULL
        );
    }
}

/*
 * Set XF_VFS_FLAG_READONLY_FS read-only flag for a registered virtual filesystem
 * for given path prefix. Should be only called from the xf_vfs_*filesystem* register
 * or helper mount functions where vfs_t is not available to set the read-only
 * flag directly (e.g. xf_vfs_fat_spiflash_mount_rw_wl).
 */
xf_err_t xf_vfs_set_readonly_flag(const char *base_path)
{
    const size_t base_path_len = xf_strlen(base_path);
    for (size_t i = 0; i < s_vfs_count; ++i) {
        xf_vfs_entry_t *vfs = s_vfs[i];
        if (vfs == NULL) {
            continue;
        }
        if (base_path_len == vfs->path_prefix_len &&
                xf_memcmp(base_path, vfs->path_prefix, vfs->path_prefix_len) == 0) {
            vfs->flags |= XF_VFS_FLAG_READONLY_FS;
            return XF_OK;
        }
    }
    return XF_ERR_INVALID_STATE;
}

const xf_vfs_entry_t *xf_vfs_get_vfs_for_index(int index)
{
    if (index < 0 || index >= s_vfs_count) {
        return NULL;
    } else {
        return s_vfs[index];
    }
}

const xf_vfs_entry_t *xf_vfs_get_vfs_for_path(const char *path)
{
    const xf_vfs_entry_t *best_match = NULL;
    xf_vfs_ssize_t best_match_prefix_len = -1;
    size_t len = xf_strlen(path);
    for (size_t i = 0; i < s_vfs_count; ++i) {
        const xf_vfs_entry_t *vfs = s_vfs[i];
        if (vfs == NULL || vfs->path_prefix_len == XF_VFS_PATH_PREFIX_LEN_IGNORED) {
            continue;
        }
        // match path prefix
        if (len < vfs->path_prefix_len ||
                xf_memcmp(path, vfs->path_prefix, vfs->path_prefix_len) != 0) {
            continue;
        }
        // this is the default VFS and we don't have a better match yet.
        if (vfs->path_prefix_len == 0 && !best_match) {
            best_match = vfs;
            continue;
        }
        // if path is not equal to the prefix, expect to see a path separator
        // i.e. don't match "/data" prefix for "/data1/foo.txt" path
        if (len > vfs->path_prefix_len &&
                path[vfs->path_prefix_len] != '/') {
            continue;
        }
        // Out of all matching path prefixes, select the longest one;
        // i.e. if "/dev" and "/dev/uart" both match, for "/dev/uart/1" path,
        // choose "/dev/uart",
        // This causes all s_vfs_count VFS entries to be scanned when opening
        // a file by name. This can be optimized by introducing a table for
        // FS search order, sorted so that longer prefixes are checked first.
        if (best_match_prefix_len < (xf_vfs_ssize_t) vfs->path_prefix_len) {
            best_match_prefix_len = (xf_vfs_ssize_t) vfs->path_prefix_len;
            best_match = vfs;
        }
    }
    return best_match;
}

/*
 * Using huge multi-line macros is never nice, but in this case
 * the only alternative is to repeat this chunk of code (with different function names)
 * for each syscall being implemented. Given that this define is contained within a single
 * file, this looks like a good tradeoff.
 *
 * First we check if syscall is implemented by VFS (corresponding member is not NULL),
 * then call the right flavor of the method (e.g. open or open_p) depending on
 * XF_VFS_FLAG_CONTEXT_PTR flag. If XF_VFS_FLAG_CONTEXT_PTR is set, context is passed
 * in as first argument and _p variant is used for the call.
 * It is enough to check just one of them for NULL, as both variants are part of a union.
 */
#define CHECK_AND_CALL(ret, r, pvfs, func, ...) \
    if (pvfs->vfs->func == NULL) { \
        errno = ENOSYS; \
        return -1; \
    } \
    if (pvfs->flags & XF_VFS_FLAG_CONTEXT_PTR) { \
        ret = (*pvfs->vfs->func ## _p)(pvfs->ctx, __VA_ARGS__); \
    } else { \
        ret = (*pvfs->vfs->func)(__VA_ARGS__); \
    }

#define CHECK_AND_CALL_SUBCOMPONENT(ret, r, pvfs, component, func, ...) \
    if (pvfs->vfs->component == NULL || pvfs->vfs->component->func == NULL) { \
        errno = ENOSYS; \
        return -1; \
    } \
    if (pvfs->flags & XF_VFS_FLAG_CONTEXT_PTR) { \
        ret = (*pvfs->vfs->component->func ## _p)(pvfs->ctx, __VA_ARGS__); \
    } else { \
        ret = (*pvfs->vfs->component->func)(__VA_ARGS__); \
    }

#define CHECK_AND_CALLV(r, pvfs, func, ...) \
    if (pvfs->vfs->func == NULL) { \
        errno = ENOSYS; \
        return; \
    } \
    if (pvfs->flags & XF_VFS_FLAG_CONTEXT_PTR) { \
        (*pvfs->vfs->func ## _p)(pvfs->ctx, __VA_ARGS__); \
    } else { \
        (*pvfs->vfs->func)(__VA_ARGS__); \
    }

#define CHECK_AND_CALL_SUBCOMPONENTV(r, pvfs, component, func, ...) \
    if (pvfs->vfs->component == NULL || pvfs->vfs->component->func == NULL) { \
        errno = ENOSYS; \
        return; \
    } \
    if (pvfs->flags & XF_VFS_FLAG_CONTEXT_PTR) { \
        (*pvfs->vfs->component->func ## _p)(pvfs->ctx, __VA_ARGS__); \
    } else { \
        (*pvfs->vfs->component->func)(__VA_ARGS__); \
    }

#define CHECK_AND_CALLP(ret, r, pvfs, func, ...) \
    if (pvfs->vfs->func == NULL) { \
        errno = ENOSYS; \
        return NULL; \
    } \
    if (pvfs->flags & XF_VFS_FLAG_CONTEXT_PTR) { \
        ret = (*pvfs->vfs->func ## _p)(pvfs->ctx, __VA_ARGS__); \
    } else { \
        ret = (*pvfs->vfs->func)(__VA_ARGS__); \
    }

#define CHECK_AND_CALL_SUBCOMPONENTP(ret, r, pvfs, component, func, ...) \
    if (pvfs->vfs->component == NULL || pvfs->vfs->component->func == NULL) { \
        errno = ENOSYS; \
        return NULL; \
    } \
    if (pvfs->flags & XF_VFS_FLAG_CONTEXT_PTR) { \
        ret = (*pvfs->vfs->component->func ## _p)(pvfs->ctx, __VA_ARGS__); \
    } else { \
        ret = (*pvfs->vfs->component->func)(__VA_ARGS__); \
    }

#define CHECK_VFS_READONLY_FLAG(flags) \
    if (flags & XF_VFS_FLAG_READONLY_FS) { \
        errno = EROFS; \
        return -1; \
    }

int xf_vfs_open(const char *path, int flags, int mode)
{
    const xf_vfs_entry_t *vfs = xf_vfs_get_vfs_for_path(path);
    if (vfs == NULL) {
        errno = ENOENT;
        return -1;
    }

    int acc_mode = flags & XF_VFS_O_ACCMODE;
    int ro_filesystem = vfs->flags & XF_VFS_FLAG_READONLY_FS;
    if (acc_mode != XF_VFS_O_RDONLY && ro_filesystem) {
        errno = EROFS;
        return -1;
    }

    const char *path_within_vfs = translate_path(vfs, path);
    int fd_within_vfs;
    CHECK_AND_CALL(fd_within_vfs, r, vfs, open, path_within_vfs, flags, mode);
    if (fd_within_vfs >= 0) {
        _lock_acquire(s_fd_table_lock);
        for (int i = 0; i < XF_VFS_FDS_MAX; ++i) {
            if (s_fd_table[i].vfs_index == -1) {
                s_fd_table[i].permanent = false;
                s_fd_table[i].vfs_index = vfs->offset;
                s_fd_table[i].local_fd = fd_within_vfs;
                _lock_release(s_fd_table_lock);
                return i;
            }
        }
        _lock_release(s_fd_table_lock);
        int ret;
        CHECK_AND_CALL(ret, r, vfs, close, fd_within_vfs);
        (void) ret; // remove "set but not used" warning
        errno = ENOMEM;
        return -1;
    }
    return -1;
}

xf_vfs_ssize_t xf_vfs_write(int fd, const void *data, size_t size)
{
    const xf_vfs_entry_t *vfs = get_vfs_for_fd(fd);
    const int local_fd = get_local_fd(vfs, fd);
    if (vfs == NULL || local_fd < 0) {
        errno = EBADF;
        return -1;
    }
    xf_vfs_ssize_t ret;
    CHECK_AND_CALL(ret, r, vfs, write, local_fd, data, size);
    return ret;
}

xf_vfs_off_t xf_vfs_lseek(int fd, xf_vfs_off_t size, int mode)
{
    const xf_vfs_entry_t *vfs = get_vfs_for_fd(fd);
    const int local_fd = get_local_fd(vfs, fd);
    if (vfs == NULL || local_fd < 0) {
        errno = EBADF;
        return -1;
    }
    xf_vfs_off_t ret;
    CHECK_AND_CALL(ret, r, vfs, lseek, local_fd, size, mode);
    return ret;
}

xf_vfs_ssize_t xf_vfs_read(int fd, void *dst, size_t size)
{
    const xf_vfs_entry_t *vfs = get_vfs_for_fd(fd);
    const int local_fd = get_local_fd(vfs, fd);
    if (vfs == NULL || local_fd < 0) {
        errno = EBADF;
        return -1;
    }
    xf_vfs_ssize_t ret;
    CHECK_AND_CALL(ret, r, vfs, read, local_fd, dst, size);
    return ret;
}

xf_vfs_ssize_t xf_vfs_pread(int fd, void *dst, size_t size, xf_vfs_off_t offset)
{
    const xf_vfs_entry_t *vfs = get_vfs_for_fd(fd);
    const int local_fd = get_local_fd(vfs, fd);
    if (vfs == NULL || local_fd < 0) {
        errno = EBADF;
        return -1;
    }
    xf_vfs_ssize_t ret;
    CHECK_AND_CALL(ret, r, vfs, pread, local_fd, dst, size, offset);
    return ret;
}

xf_vfs_ssize_t xf_vfs_pwrite(int fd, const void *src, size_t size, xf_vfs_off_t offset)
{
    const xf_vfs_entry_t *vfs = get_vfs_for_fd(fd);
    const int local_fd = get_local_fd(vfs, fd);
    if (vfs == NULL || local_fd < 0) {
        errno = EBADF;
        return -1;
    }
    xf_vfs_ssize_t ret;
    CHECK_AND_CALL(ret, r, vfs, pwrite, local_fd, src, size, offset);
    return ret;
}

int xf_vfs_close(int fd)
{
    const xf_vfs_entry_t *vfs = get_vfs_for_fd(fd);
    const int local_fd = get_local_fd(vfs, fd);
    if (vfs == NULL || local_fd < 0) {
        errno = EBADF;
        return -1;
    }
    int ret;
    CHECK_AND_CALL(ret, r, vfs, close, local_fd);

    _lock_acquire(s_fd_table_lock);
    if (!s_fd_table[fd].permanent) {
        if (s_fd_table[fd].has_pending_select) {
            s_fd_table[fd].has_pending_close = true;
        } else {
            s_fd_table[fd] = FD_TABLE_ENTRY_UNUSED;
        }
    }
    _lock_release(s_fd_table_lock);
    return ret;
}

int xf_vfs_fstat(int fd, xf_vfs_stat_t *st)
{
    const xf_vfs_entry_t *vfs = get_vfs_for_fd(fd);
    const int local_fd = get_local_fd(vfs, fd);
    if (vfs == NULL || local_fd < 0) {
        errno = EBADF;
        return -1;
    }
    int ret;
    CHECK_AND_CALL(ret, r, vfs, fstat, local_fd, st);
    return ret;
}

int xf_vfs_fcntl_r(int fd, int cmd, int arg)
{
    const xf_vfs_entry_t *vfs = get_vfs_for_fd(fd);
    const int local_fd = get_local_fd(vfs, fd);
    if (vfs == NULL || local_fd < 0) {
        errno = EBADF;
        return -1;
    }
    int ret;
    CHECK_AND_CALL(ret, r, vfs, fcntl, local_fd, cmd, arg);
    return ret;
}

int xf_vfs_ioctl(int fd, int cmd, ...)
{
    const xf_vfs_entry_t *vfs = get_vfs_for_fd(fd);
    const int local_fd = get_local_fd(vfs, fd);
    if (vfs == NULL || local_fd < 0) {
        errno = EBADF;
        return -1;
    }
    int ret;
    va_list args;
    va_start(args, cmd);
    CHECK_AND_CALL(ret, r, vfs, ioctl, local_fd, cmd, args);
    va_end(args);
    return ret;
}

int xf_vfs_fsync(int fd)
{
    const xf_vfs_entry_t *vfs = get_vfs_for_fd(fd);
    const int local_fd = get_local_fd(vfs, fd);
    if (vfs == NULL || local_fd < 0) {
        errno = EBADF;
        return -1;
    }
    int ret;
    CHECK_AND_CALL(ret, r, vfs, fsync, local_fd);
    return ret;
}

#if XF_VFS_SUPPORT_DIR_IS_ENABLE

int xf_vfs_stat(const char *path, xf_vfs_stat_t *st)
{
    const xf_vfs_entry_t *vfs = xf_vfs_get_vfs_for_path(path);
    if (vfs == NULL) {
        errno = ENOENT;
        return -1;
    }
    const char *path_within_vfs = translate_path(vfs, path);
    int ret;
    CHECK_AND_CALL_SUBCOMPONENT(ret, r, vfs, dir, stat, path_within_vfs, st);
    return ret;
}

int xf_vfs_utime(const char *path, const xf_vfs_utimbuf_t *times)
{
    int ret;
    const xf_vfs_entry_t *vfs = xf_vfs_get_vfs_for_path(path);
    if (vfs == NULL) {
        errno = ENOENT;
        return -1;
    }
    const char *path_within_vfs = translate_path(vfs, path);
    CHECK_AND_CALL_SUBCOMPONENT(ret, r, vfs, dir, utime, path_within_vfs, times);
    return ret;
}

int xf_vfs_link(const char *n1, const char *n2)
{
    const xf_vfs_entry_t *vfs = xf_vfs_get_vfs_for_path(n1);
    if (vfs == NULL) {
        errno = ENOENT;
        return -1;
    }
    const xf_vfs_entry_t *vfs2 = xf_vfs_get_vfs_for_path(n2);
    if (vfs != vfs2) {
        errno = EXDEV;
        return -1;
    }

    CHECK_VFS_READONLY_FLAG(vfs2->flags);

    const char *path1_within_vfs = translate_path(vfs, n1);
    const char *path2_within_vfs = translate_path(vfs, n2);
    int ret;
    CHECK_AND_CALL_SUBCOMPONENT(ret, r, vfs, dir, link, path1_within_vfs, path2_within_vfs);
    return ret;
}

int xf_vfs_unlink(const char *path)
{
    const xf_vfs_entry_t *vfs = xf_vfs_get_vfs_for_path(path);
    if (vfs == NULL) {
        errno = ENOENT;
        return -1;
    }

    CHECK_VFS_READONLY_FLAG(vfs->flags);

    const char *path_within_vfs = translate_path(vfs, path);
    int ret;
    CHECK_AND_CALL_SUBCOMPONENT(ret, r, vfs, dir, unlink, path_within_vfs);
    return ret;
}

int xf_vfs_rename(const char *src, const char *dst)
{
    const xf_vfs_entry_t *vfs = xf_vfs_get_vfs_for_path(src);
    if (vfs == NULL) {
        errno = ENOENT;
        return -1;
    }

    CHECK_VFS_READONLY_FLAG(vfs->flags);

    const xf_vfs_entry_t *vfs_dst = xf_vfs_get_vfs_for_path(dst);
    if (vfs != vfs_dst) {
        errno = EXDEV;
        return -1;
    }

    CHECK_VFS_READONLY_FLAG(vfs_dst->flags);

    const char *src_within_vfs = translate_path(vfs, src);
    const char *dst_within_vfs = translate_path(vfs, dst);
    int ret;
    CHECK_AND_CALL_SUBCOMPONENT(ret, r, vfs, dir, rename, src_within_vfs, dst_within_vfs);
    return ret;
}

xf_vfs_dir_t *xf_vfs_opendir(const char *name)
{
    const xf_vfs_entry_t *vfs = xf_vfs_get_vfs_for_path(name);
    if (vfs == NULL) {
        errno = ENOENT;
        return NULL;
    }
    const char *path_within_vfs = translate_path(vfs, name);
    xf_vfs_dir_t *ret;
    CHECK_AND_CALL_SUBCOMPONENTP(ret, r, vfs, dir, opendir, path_within_vfs);
    if (ret != NULL) {
        ret->dd_vfs_idx = vfs->offset;
    }
    return ret;
}

xf_vfs_dirent_t *xf_vfs_readdir(xf_vfs_dir_t *pdir)
{
    const xf_vfs_entry_t *vfs = xf_vfs_get_vfs_for_index(pdir->dd_vfs_idx);
    if (vfs == NULL) {
        errno = EBADF;
        return NULL;
    }
    xf_vfs_dirent_t *ret;
    CHECK_AND_CALL_SUBCOMPONENTP(ret, r, vfs, dir, readdir, pdir);
    return ret;
}

int xf_vfs_readdir_r(xf_vfs_dir_t *pdir, xf_vfs_dirent_t *entry, xf_vfs_dirent_t **out_dirent)
{
    const xf_vfs_entry_t *vfs = xf_vfs_get_vfs_for_index(pdir->dd_vfs_idx);
    if (vfs == NULL) {
        errno = EBADF;
        return -1;
    }
    int ret;
    CHECK_AND_CALL_SUBCOMPONENT(ret, r, vfs, dir, readdir_r, pdir, entry, out_dirent);
    return ret;
}

long xf_vfs_telldir(xf_vfs_dir_t *pdir)
{
    const xf_vfs_entry_t *vfs = xf_vfs_get_vfs_for_index(pdir->dd_vfs_idx);
    if (vfs == NULL) {
        errno = EBADF;
        return -1;
    }
    long ret;
    CHECK_AND_CALL_SUBCOMPONENT(ret, r, vfs, dir, telldir, pdir);
    return ret;
}

void xf_vfs_seekdir(xf_vfs_dir_t *pdir, long loc)
{
    const xf_vfs_entry_t *vfs = xf_vfs_get_vfs_for_index(pdir->dd_vfs_idx);
    if (vfs == NULL) {
        errno = EBADF;
        return;
    }
    CHECK_AND_CALL_SUBCOMPONENTV(r, vfs, dir, seekdir, pdir, loc);
}

void xf_vfs_rewinddir(xf_vfs_dir_t *pdir)
{
    xf_vfs_seekdir(pdir, 0);
}

int xf_vfs_closedir(xf_vfs_dir_t *pdir)
{
    const xf_vfs_entry_t *vfs = xf_vfs_get_vfs_for_index(pdir->dd_vfs_idx);
    if (vfs == NULL) {
        errno = EBADF;
        return -1;
    }
    int ret;
    CHECK_AND_CALL_SUBCOMPONENT(ret, r, vfs, dir, closedir, pdir);
    return ret;
}

int xf_vfs_mkdir(const char *name, xf_vfs_mode_t mode)
{
    const xf_vfs_entry_t *vfs = xf_vfs_get_vfs_for_path(name);
    if (vfs == NULL) {
        errno = ENOENT;
        return -1;
    }

    CHECK_VFS_READONLY_FLAG(vfs->flags);

    const char *path_within_vfs = translate_path(vfs, name);
    int ret;
    CHECK_AND_CALL_SUBCOMPONENT(ret, r, vfs, dir, mkdir, path_within_vfs, mode);
    return ret;
}

int xf_vfs_rmdir(const char *name)
{
    const xf_vfs_entry_t *vfs = xf_vfs_get_vfs_for_path(name);
    if (vfs == NULL) {
        errno = ENOENT;
        return -1;
    }

    CHECK_VFS_READONLY_FLAG(vfs->flags);

    const char *path_within_vfs = translate_path(vfs, name);
    int ret;
    CHECK_AND_CALL_SUBCOMPONENT(ret, r, vfs, dir, rmdir, path_within_vfs);
    return ret;
}

int xf_vfs_access(const char *path, int amode)
{
    int ret;
    const xf_vfs_entry_t *vfs = xf_vfs_get_vfs_for_path(path);
    if (vfs == NULL) {
        errno = ENOENT;
        return -1;
    }
    const char *path_within_vfs = translate_path(vfs, path);
    CHECK_AND_CALL_SUBCOMPONENT(ret, r, vfs, dir, access, path_within_vfs, amode);
    return ret;
}

int xf_vfs_truncate(const char *path, xf_vfs_off_t length)
{
    int ret;
    const xf_vfs_entry_t *vfs = xf_vfs_get_vfs_for_path(path);
    if (vfs == NULL) {
        errno = ENOENT;
        return -1;
    }

    CHECK_VFS_READONLY_FLAG(vfs->flags);

    const char *path_within_vfs = translate_path(vfs, path);
    CHECK_AND_CALL_SUBCOMPONENT(ret, r, vfs, dir, truncate, path_within_vfs, length);
    return ret;
}

int xf_vfs_ftruncate(int fd, xf_vfs_off_t length)
{
    const xf_vfs_entry_t *vfs = get_vfs_for_fd(fd);
    int local_fd = get_local_fd(vfs, fd);
    if (vfs == NULL || local_fd < 0) {
        errno = EBADF;
        return -1;
    }

    CHECK_VFS_READONLY_FLAG(vfs->flags);

    int ret;
    CHECK_AND_CALL_SUBCOMPONENT(ret, r, vfs, dir, ftruncate, local_fd, length);
    return ret;
}

#endif // CONFIG_XF_VFS_SUPPORT_DIR

#if XF_VFS_SUPPORT_SELECT_IS_ENABLE

static void call_end_selects(int end_index, const fds_triple_t *vfs_fds_triple, void **driver_args)
{
    for (int i = 0; i < end_index; ++i) {
        const xf_vfs_entry_t *vfs = xf_vfs_get_vfs_for_index(i);
        const fds_triple_t *item = &vfs_fds_triple[i];
        if (vfs != NULL
                && vfs->vfs->select != NULL
                && vfs->vfs->select->end_select != NULL
                && item->isset
           ) {
            xf_err_t err = vfs->vfs->select->end_select(driver_args[i]);
            if (err != XF_OK) {
                XF_LOGD(TAG, "end_select failed: %s", xf_err_to_name(err));
            }
        }
    }
}

static inline bool xf_vfs_safe_fd_isset(int fd, const xf_fd_set *fds)
{
    return fds && XF_FD_ISSET(fd, fds);
}

static int set_global_fd_sets(const fds_triple_t *vfs_fds_triple, int size, xf_fd_set *readfds, xf_fd_set *writefds,
                              xf_fd_set *errorfds)
{
    int ret = 0;

    for (int i = 0; i < size; ++i) {
        const fds_triple_t *item = &vfs_fds_triple[i];
        if (item->isset) {
            for (int fd = 0; fd < XF_VFS_FDS_MAX; ++fd) {
                if (s_fd_table[fd].vfs_index == i) {
                    const int local_fd = s_fd_table[fd].local_fd; // single read -> no locking is required
                    if (readfds && xf_vfs_safe_fd_isset(local_fd, &item->readfds)) {
                        XF_LOGD(TAG, "FD %d in readfds was set from VFS ID %d", fd, i);
                        XF_FD_SET(fd, readfds);
                        ++ret;
                    }
                    if (writefds && xf_vfs_safe_fd_isset(local_fd, &item->writefds)) {
                        XF_LOGD(TAG, "FD %d in writefds was set from VFS ID %d", fd, i);
                        XF_FD_SET(fd, writefds);
                        ++ret;
                    }
                    if (errorfds && xf_vfs_safe_fd_isset(local_fd, &item->errorfds)) {
                        XF_LOGD(TAG, "FD %d in errorfds was set from VFS ID %d", fd, i);
                        XF_FD_SET(fd, errorfds);
                        ++ret;
                    }
                }
            }
        }
    }

    return ret;
}

static void xf_vfs_log_fd_set(const char *fds_name, const xf_fd_set *fds)
{
    if (fds_name && fds) {
        XF_LOGD(TAG, "FDs in %s =", fds_name);
        for (int i = 0; i < XF_VFS_FDS_MAX; ++i) {
            if (xf_vfs_safe_fd_isset(i, fds)) {
                XF_LOGD(TAG, "%d", i);
            }
        }
    }
}

int xf_vfs_select(int nfds, xf_fd_set *readfds, xf_fd_set *writefds, xf_fd_set *errorfds, xf_vfs_timeval_t *timeout)
{
    // NOTE: Please see the "Synchronous input/output multiplexing" section of the ESP-IDF Programming Guide
    // (API Reference -> Storage -> Virtual Filesystem) for a general overview of the implementation of VFS select().
    int ret = 0;

    XF_LOGD(TAG, "xf_vfs_select starts with nfds = %d", nfds);
    if (timeout) {
        XF_LOGD(TAG, "timeout is %lds + %ldus", (long)timeout->tv_sec, timeout->tv_usec);
    }
    xf_vfs_log_fd_set("readfds", readfds);
    xf_vfs_log_fd_set("writefds", writefds);
    xf_vfs_log_fd_set("errorfds", errorfds);

    if (nfds > XF_VFS_FDS_MAX || nfds < 0) {
        XF_LOGD(TAG, "incorrect nfds");
        errno = EINVAL;
        return -1;
    }

    // Capture s_vfs_count to a local variable in case a new driver is registered or removed during this actual select()
    // call. s_vfs_count cannot be protected with a mutex during a select() call (which can be one without a timeout)
    // because that could block the registration of new driver.
    const size_t vfs_count = s_vfs_count;
    fds_triple_t *vfs_fds_triple;
    vfs_fds_triple = xf_malloc(vfs_count * sizeof(fds_triple_t));
    if (vfs_fds_triple == NULL) {
        errno = ENOMEM;
        XF_LOGD(TAG, "calloc is unsuccessful");
        return -1;
    }
    xf_memset(vfs_fds_triple, 0, vfs_count * sizeof(fds_triple_t));

    xf_vfs_select_sem_t sel_sem = {
        .is_sem_local = false,
        .sem = NULL,
    };

    int (*socket_select)(int, xf_fd_set *, xf_fd_set *, xf_fd_set *, xf_vfs_timeval_t *) = NULL;
    for (int fd = 0; fd < nfds; ++fd) {
        _lock_acquire(s_fd_table_lock);
        const bool is_socket_fd = s_fd_table[fd].permanent;
        const int vfs_index = s_fd_table[fd].vfs_index;
        const int local_fd = s_fd_table[fd].local_fd;
        if (xf_vfs_safe_fd_isset(fd, errorfds)) {
            s_fd_table[fd].has_pending_select = true;
        }
        _lock_release(s_fd_table_lock);

        if (vfs_index < 0) {
            continue;
        }

        if (is_socket_fd) {
            if (!socket_select) {
                // no socket_select found yet so take a look
                if (xf_vfs_safe_fd_isset(fd, readfds) ||
                        xf_vfs_safe_fd_isset(fd, writefds) ||
                        xf_vfs_safe_fd_isset(fd, errorfds)) {
                    const xf_vfs_entry_t *vfs = s_vfs[vfs_index];
                    socket_select = vfs->vfs->select->socket_select;
                    sel_sem.sem = vfs->vfs->select->get_socket_select_semaphore();
                }
            }
            continue;
        }

        fds_triple_t *item = &vfs_fds_triple[vfs_index]; // FD sets for VFS which belongs to fd
        if (xf_vfs_safe_fd_isset(fd, readfds)) {
            item->isset = true;
            XF_FD_SET(local_fd, &item->readfds);
            XF_FD_CLR(fd, readfds);
            XF_LOGD(TAG, "removing %d from readfds and adding as local FD %d to xf_fd_set of VFS ID %d", fd, local_fd, vfs_index);
        }
        if (xf_vfs_safe_fd_isset(fd, writefds)) {
            item->isset = true;
            XF_FD_SET(local_fd, &item->writefds);
            XF_FD_CLR(fd, writefds);
            XF_LOGD(TAG, "removing %d from writefds and adding as local FD %d to xf_fd_set of VFS ID %d", fd, local_fd, vfs_index);
        }
        if (xf_vfs_safe_fd_isset(fd, errorfds)) {
            item->isset = true;
            XF_FD_SET(local_fd, &item->errorfds);
            XF_FD_CLR(fd, errorfds);
            XF_LOGD(TAG, "removing %d from errorfds and adding as local FD %d to xf_fd_set of VFS ID %d", fd, local_fd, vfs_index);
        }
    }

    // all non-socket VFSs have their FD sets in vfs_fds_triple
    // the global readfds, writefds and errorfds contain only socket FDs (if
    // there any)

    if (!socket_select) {
        // There is no socket VFS registered or select() wasn't called for
        // any socket. Therefore, we will use our own signalization.
        sel_sem.is_sem_local = true;
        xf_osal_semaphore_attr_t sem_attr = {
            .name = "sem",
        };
        sel_sem.sem = (void *)xf_osal_semaphore_create(1, 1, &sem_attr);
        if (sel_sem.sem == NULL) {
            xf_free(vfs_fds_triple);
            errno = ENOMEM;
            XF_LOGD(TAG, "cannot create select semaphore");
            return -1;
        }
    }

    void **driver_args = xf_malloc(vfs_count * sizeof(void *));

    if (driver_args == NULL) {
        xf_free(vfs_fds_triple);
        errno = ENOMEM;
        XF_LOGD(TAG, "calloc is unsuccessful for driver args");
        return -1;
    }
    xf_memset(driver_args, 0, vfs_count * sizeof(void *));

    for (size_t i = 0; i < vfs_count; ++i) {
        const xf_vfs_entry_t *vfs = xf_vfs_get_vfs_for_index(i);
        fds_triple_t *item = &vfs_fds_triple[i];

        if (vfs == NULL || vfs->vfs->select == NULL || vfs->vfs->select->start_select == NULL) {
            XF_LOGD(TAG, "start_select function callback for this vfs (s_vfs[%d]) is not defined", vfs->offset);
            continue;
        }

        if (!item->isset) {
            continue;
        }

        // call start_select for all non-socket VFSs with has at least one FD set in readfds, writefds, or errorfds
        // note: it can point to socket VFS but item->isset will be false for that
        XF_LOGD(TAG, "calling start_select for VFS ID %d with the following local FDs", i);
        xf_vfs_log_fd_set("readfds", &item->readfds);
        xf_vfs_log_fd_set("writefds", &item->writefds);
        xf_vfs_log_fd_set("errorfds", &item->errorfds);
        xf_err_t err = vfs->vfs->select->start_select(nfds, &item->readfds, &item->writefds, &item->errorfds, sel_sem,
                       driver_args + i);

        if (err != XF_OK) {
            if (err != XF_ERR_NOT_SUPPORTED) {
                call_end_selects(i, vfs_fds_triple, driver_args);
            }
            (void) set_global_fd_sets(vfs_fds_triple, vfs_count, readfds, writefds, errorfds);
            if (sel_sem.is_sem_local && sel_sem.sem) {
                xf_osal_semaphore_delete(sel_sem.sem);
                sel_sem.sem = NULL;
            }
            xf_free(vfs_fds_triple);
            xf_free(driver_args);
            errno = EINTR;
            XF_LOGD(TAG, "start_select failed: %s", xf_err_to_name(err));
            return -1;
        }
    }

    if (socket_select) {
        XF_LOGD(TAG, "calling socket_select with the following FDs");
        xf_vfs_log_fd_set("readfds", readfds);
        xf_vfs_log_fd_set("writefds", writefds);
        xf_vfs_log_fd_set("errorfds", errorfds);
        ret = socket_select(nfds, readfds, writefds, errorfds, timeout);
        XF_LOGD(TAG, "socket_select returned %d and the FDs are the following", ret);
        xf_vfs_log_fd_set("readfds", readfds);
        xf_vfs_log_fd_set("writefds", writefds);
        xf_vfs_log_fd_set("errorfds", errorfds);
    } else {
        if (readfds) {
            XF_FD_ZERO(readfds);
        }
        if (writefds) {
            XF_FD_ZERO(writefds);
        }
        if (errorfds) {
            XF_FD_ZERO(errorfds);
        }

        uint32_t ticks_to_wait = XF_OSAL_WAIT_FOREVER;
        if (timeout) {
            uint32_t timeout_ms = (timeout->tv_sec * 1000) + (timeout->tv_usec / 1000);
            /* Round up the number of ticks.
             * Not only we need to round up the number of ticks, but we also need to add 1.
             * Indeed, `select` function shall wait for AT LEAST timeout, but on FreeRTOS,
             * if we specify a timeout of 1 tick to `xSemaphoreTake`, it will take AT MOST
             * 1 tick before triggering a timeout. Thus, we need to pass 2 ticks as a timeout
             * to `xSemaphoreTake`. */
            // ticks_to_wait = ((timeout_ms + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS) + 1;
            ticks_to_wait = xf_osal_kernel_ms_to_ticks(timeout_ms) + 1;
            XF_LOGD(TAG, "timeout is %" PRIu32 "ms", timeout_ms);
        }
        XF_LOGD(TAG, "waiting without calling socket_select");
        xf_osal_semaphore_acquire(sel_sem.sem, ticks_to_wait);
    }

    call_end_selects(vfs_count, vfs_fds_triple, driver_args); // for VFSs for start_select was called before

    if (ret >= 0) {
        ret += set_global_fd_sets(vfs_fds_triple, vfs_count, readfds, writefds, errorfds);
    }
    if (sel_sem.sem) { // Cleanup the select semaphore
        if (sel_sem.is_sem_local) {
            xf_osal_semaphore_delete(sel_sem.sem);
        } else if (socket_select) {
            // SemaphoreHandle_t *s = sel_sem.sem;
            // /* Select might have been triggered from both lwip and vfs fds at the same time, and
            //  * we have to make sure that the lwip semaphore is cleared when we exit select().
            //  * It is safe, as the semaphore belongs to the calling thread. */
            // xSemaphoreTake(*s, 0);
            xf_osal_semaphore_acquire(sel_sem.sem, 0);
        }
        sel_sem.sem = NULL;
    }
    _lock_acquire(s_fd_table_lock);
    for (int fd = 0; fd < nfds; ++fd) {
        if (s_fd_table[fd].has_pending_close) {
            s_fd_table[fd] = FD_TABLE_ENTRY_UNUSED;
        }
    }
    _lock_release(s_fd_table_lock);
    xf_free(vfs_fds_triple);
    xf_free(driver_args);

    XF_LOGD(TAG, "xf_vfs_select returns %d", ret);
    xf_vfs_log_fd_set("readfds", readfds);
    xf_vfs_log_fd_set("writefds", writefds);
    xf_vfs_log_fd_set("errorfds", errorfds);
    return ret;
}

void xf_vfs_select_triggered(xf_vfs_select_sem_t sem)
{
    if (sem.is_sem_local) {
        xf_osal_semaphore_release(sem.sem);
    } else {
        // Another way would be to go through s_fd_table and find the VFS
        // which has a permanent FD. But in order to avoid to lock
        // s_fd_table_lock we go through the VFS table.
        for (int i = 0; i < s_vfs_count; ++i) {
            // Note: s_vfs_count could have changed since the start of vfs_select() call. However, that change doesn't
            // matter here stop_socket_select() will be called for only valid VFS drivers.
            const xf_vfs_entry_t *vfs = s_vfs[i];
            if (vfs != NULL
                    && vfs->vfs->select != NULL
                    && vfs->vfs->select->stop_socket_select != NULL
               ) {
                vfs->vfs->select->stop_socket_select(sem.sem);
                break;
            }
        }
    }
}

void xf_vfs_select_triggered_isr(xf_vfs_select_sem_t sem, int *woken)
{
    if (sem.is_sem_local) {
        // xSemaphoreGiveFromISR(sem.sem, woken);
        /* TODO 未处理 woken */
        xf_osal_semaphore_release(sem.sem);
    } else {
        // Another way would be to go through s_fd_table and find the VFS
        // which has a permanent FD. But in order to avoid to lock
        // s_fd_table_lock we go through the VFS table.
        for (int i = 0; i < s_vfs_count; ++i) {
            // Note: s_vfs_count could have changed since the start of vfs_select() call. However, that change doesn't
            // matter here stop_socket_select() will be called for only valid VFS drivers.
            const xf_vfs_entry_t *vfs = s_vfs[i];
            if (vfs != NULL
                    && vfs->vfs->select != NULL
                    && vfs->vfs->select->stop_socket_select_isr != NULL
               ) {
                // Note: If the UART ISR resides in IRAM, the function referenced by stop_socket_select_isr should also be placed in IRAM.
                vfs->vfs->select->stop_socket_select_isr(sem.sem, woken);
                break;
            }
        }
    }
}

#endif // XF_VFS_SUPPORT_SELECT_IS_ENABLE

/* ==================== [Static Functions] ================================== */

static xf_vfs_ssize_t xf_get_free_index(void)
{
    for (xf_vfs_ssize_t i = 0; i < XF_VFS_MAX_COUNT; i++) {
        if (s_vfs[i] == NULL) {
            return i;
        }
    }
    return -1;
}

static void xf_vfs_free_fs_ops(xf_vfs_fs_ops_t *vfs)
{
// We can afford to cast away the const qualifier here, because we know that we allocated the struct and therefore its safe

#if XF_VFS_SUPPORT_DIR_IS_ENABLE
    xf_free((void *)vfs->dir);
#endif

#if XF_VFS_SUPPORT_SELECT_IS_ENABLE
    xf_free((void *)vfs->select);
#endif

    xf_free(vfs);
}

static void xf_vfs_free_entry(xf_vfs_entry_t *entry)
{
    if (entry == NULL) { // Necessary because of the following flags check
        return;
    }

    if (!(entry->flags & XF_VFS_FLAG_STATIC)) {
        xf_vfs_free_fs_ops((xf_vfs_fs_ops_t *)entry->vfs); // const cast, but we know it's not static from the flag
    }

    xf_free(entry);
}

static xf_vfs_fs_ops_t *xf_minify_vfs(const xf_vfs_t *const vfs, vfs_component_proxy_t proxy)
{
    XF_CHECK(vfs == NULL, NULL, TAG, "vfs is NULL");
#if XF_VFS_SUPPORT_DIR_IS_ENABLE
    // If the dir functions are not implemented, we don't need to convert them
    if (proxy.dir != NULL) {
        xf_vfs_dir_ops_t tmp = {
            .stat = vfs->stat,
            .link = vfs->link,
            .unlink = vfs->unlink,
            .rename = vfs->rename,
            .opendir = vfs->opendir,
            .readdir = vfs->readdir,
            .readdir_r = vfs->readdir_r,
            .telldir = vfs->telldir,
            .seekdir = vfs->seekdir,
            .closedir = vfs->closedir,
            .mkdir = vfs->mkdir,
            .rmdir = vfs->rmdir,
            .access = vfs->access,
            .truncate = vfs->truncate,
            .ftruncate = vfs->ftruncate,
            .utime = vfs->utime,
        };

        xf_memcpy(proxy.dir, &tmp, sizeof(xf_vfs_dir_ops_t));
    }
#endif // CONFIG_XF_VFS_SUPPORT_DIR

#if XF_VFS_SUPPORT_SELECT_IS_ENABLE
    // If the select functions are not implemented, we don't need to convert them
    if (proxy.select != NULL) {
        xf_vfs_select_ops_t tmp = {
            .start_select = vfs->start_select,
            .socket_select = vfs->socket_select,
            .stop_socket_select = vfs->stop_socket_select,
            .stop_socket_select_isr = vfs->stop_socket_select_isr,
            .get_socket_select_semaphore = vfs->get_socket_select_semaphore,
            .end_select = vfs->end_select,
        };

        xf_memcpy(proxy.select, &tmp, sizeof(xf_vfs_select_ops_t));
    }
#endif // XF_VFS_SUPPORT_SELECT_IS_ENABLE

    xf_vfs_fs_ops_t tmp = {
        .write = vfs->write,
        .lseek = vfs->lseek,
        .read = vfs->read,
        .pread = vfs->pread,
        .pwrite = vfs->pwrite,
        .open = vfs->open,
        .close = vfs->close,
        .fstat = vfs->fstat,
        .fcntl = vfs->fcntl,
        .ioctl = vfs->ioctl,
        .fsync = vfs->fsync,
#if XF_VFS_SUPPORT_DIR_IS_ENABLE
        .dir = proxy.dir,
#endif
#if XF_VFS_SUPPORT_SELECT_IS_ENABLE
        .select = proxy.select,
#endif
    };

    xf_vfs_fs_ops_t *out = xf_malloc(sizeof(xf_vfs_fs_ops_t));
    if (out == NULL) {
        return NULL;
    }

    // Doing this is the only way to correctly initialize const members of a struct according to C standard
    xf_memcpy(out, &tmp, sizeof(xf_vfs_fs_ops_t));

    return out;
}

static void free_proxy_members(vfs_component_proxy_t *proxy)
{
#if XF_VFS_SUPPORT_DIR_IS_ENABLE
    xf_free(proxy->dir);
#endif
#if XF_VFS_SUPPORT_SELECT_IS_ENABLE
    xf_free(proxy->select);
#endif
}

static xf_vfs_fs_ops_t *xf_vfs_duplicate_fs_ops(const xf_vfs_fs_ops_t *orig)
{
    vfs_component_proxy_t proxy = {};

#if XF_VFS_SUPPORT_DIR_IS_ENABLE
    if (orig->dir != NULL) {
        proxy.dir = (xf_vfs_dir_ops_t *) xf_malloc(sizeof(xf_vfs_dir_ops_t));
        if (proxy.dir == NULL) {
            goto fail;
        }
        xf_memcpy(proxy.dir, orig->dir, sizeof(xf_vfs_dir_ops_t));
    }
#endif

#if XF_VFS_SUPPORT_SELECT_IS_ENABLE
    if (orig->select != NULL) {
        proxy.select = (xf_vfs_select_ops_t *) xf_malloc(sizeof(xf_vfs_select_ops_t));
        if (proxy.select == NULL) {
            goto fail;
        }
        xf_memcpy(proxy.select, orig->select, sizeof(xf_vfs_select_ops_t));
    }
#endif

    // This tediousness is required because of const members
    xf_vfs_fs_ops_t tmp = {
        .write = orig->write,
        .lseek = orig->lseek,
        .read = orig->read,
        .pread = orig->pread,
        .pwrite = orig->pwrite,
        .open = orig->open,
        .close = orig->close,
        .fstat = orig->fstat,
        .fcntl = orig->fcntl,
        .ioctl = orig->ioctl,
        .fsync = orig->fsync,
#if XF_VFS_SUPPORT_DIR_IS_ENABLE
        .dir = proxy.dir,
#endif
#if XF_VFS_SUPPORT_SELECT_IS_ENABLE
        .select = proxy.select,
#endif
    };

    xf_vfs_fs_ops_t *out = xf_malloc(sizeof(xf_vfs_fs_ops_t));
    if (out == NULL) {
        goto fail;
    }

    xf_memcpy(out, &tmp, sizeof(xf_vfs_fs_ops_t));

    return out;

fail:
    free_proxy_members(&proxy);
    return NULL;
}

static xf_err_t xf_vfs_make_fs_ops(const xf_vfs_t *vfs, xf_vfs_fs_ops_t **min)
{
    if (vfs == NULL) {
        XF_LOGE(TAG, "Cannot minify NULL VFS");
        return XF_ERR_INVALID_ARG;
    }

    if (min == NULL) {
        XF_LOGE(TAG, "Cannot minify VFS to NULL");
        return XF_ERR_INVALID_ARG;
    }

    vfs_component_proxy_t proxy = {};

#if XF_VFS_SUPPORT_DIR_IS_ENABLE
    const bool skip_dir =
        vfs->stat == NULL &&
        vfs->link == NULL &&
        vfs->unlink == NULL &&
        vfs->rename == NULL &&
        vfs->opendir == NULL &&
        vfs->readdir == NULL &&
        vfs->readdir_r == NULL &&
        vfs->telldir == NULL &&
        vfs->seekdir == NULL &&
        vfs->closedir == NULL &&
        vfs->mkdir == NULL &&
        vfs->rmdir == NULL &&
        vfs->access == NULL &&
        vfs->truncate == NULL &&
        vfs->ftruncate == NULL &&
        vfs->utime == NULL;

    if (!skip_dir) {
        proxy.dir = (xf_vfs_dir_ops_t *) xf_malloc(sizeof(xf_vfs_dir_ops_t));
        if (proxy.dir == NULL) {
            goto fail;
        }
    }
#endif

#if XF_VFS_SUPPORT_SELECT_IS_ENABLE
    const bool skip_select =
        vfs->start_select == NULL &&
        vfs->socket_select == NULL &&
        vfs->stop_socket_select == NULL &&
        vfs->stop_socket_select_isr == NULL &&
        vfs->get_socket_select_semaphore == NULL &&
        vfs->end_select == NULL;

    if (!skip_select) {
        proxy.select = (xf_vfs_select_ops_t *) xf_malloc(sizeof(xf_vfs_select_ops_t));
        if (proxy.select == NULL) {
            goto fail;
        }
    }
#endif

    xf_vfs_fs_ops_t *main = xf_minify_vfs(vfs, proxy);
    if (main == NULL) {
        goto fail;
    }

    *min = main;
    return XF_OK;

fail:

    free_proxy_members(&proxy);
    return XF_ERR_NO_MEM;
}

static xf_err_t xf_vfs_register_fs_common(const char *base_path, size_t len, const xf_vfs_fs_ops_t *vfs, int flags,
        void *ctx, int *vfs_index)
{
    if (vfs == NULL) {
        XF_LOGE(TAG, "VFS is NULL");
        return XF_ERR_INVALID_ARG;
    }

    if (len != XF_VFS_PATH_PREFIX_LEN_IGNORED) {
        /* empty prefix is allowed, "/" is not allowed */
        if ((len == 1) || (len > XF_VFS_PATH_MAX)) {
            return XF_ERR_INVALID_ARG;
        }
        /* prefix has to start with "/" and not end with "/" */
        if (len >= 2 && ((base_path[0] != '/') || (base_path[len - 1] == '/'))) {
            return XF_ERR_INVALID_ARG;
        }
    }

    xf_vfs_ssize_t index = xf_get_free_index();
    if (index < 0) {
        return XF_ERR_NO_MEM;
    }

    if (s_vfs[index] != NULL) {
        return XF_ERR_INVALID_STATE;
    }

    if (index == s_vfs_count) {
        s_vfs_count++;
    }

    xf_vfs_entry_t *entry = (xf_vfs_entry_t *) xf_malloc(sizeof(xf_vfs_entry_t));
    if (entry == NULL) {
        return XF_ERR_NO_MEM;
    }

    s_vfs[index] = entry;
    if (len != XF_VFS_PATH_PREFIX_LEN_IGNORED) {
        xf_strncpy(entry->path_prefix, base_path, sizeof(entry->path_prefix)); // we have already verified argument length
    } else {
        xf_memset(entry->path_prefix, 0, sizeof(entry->path_prefix));
    }
    entry->path_prefix_len = len;
    entry->vfs = vfs;
    entry->ctx = ctx;
    entry->offset = index;
    entry->flags = flags;

    if (vfs_index) {
        *vfs_index = index;
    }

    return XF_OK;
}

static inline bool fd_valid(int fd)
{
    return (fd < XF_VFS_FDS_MAX) && (fd >= 0);
}

static const xf_vfs_entry_t *get_vfs_for_fd(int fd)
{
    const xf_vfs_entry_t *vfs = NULL;
    if (fd_valid(fd)) {
        const int index = s_fd_table[fd].vfs_index; // single read -> no locking is required
        vfs = xf_vfs_get_vfs_for_index(index);
    }
    return vfs;
}

static inline int get_local_fd(const xf_vfs_entry_t *vfs, int fd)
{
    int local_fd = -1;

    if (vfs && fd_valid(fd)) {
        local_fd = s_fd_table[fd].local_fd; // single read -> no locking is required
    }

    return local_fd;
}

static const char *translate_path(const xf_vfs_entry_t *vfs, const char *src_path)
{
    int cmp_res = xf_strncmp(src_path, vfs->path_prefix, vfs->path_prefix_len);
    XF_CHECK(cmp_res != 0, NULL, TAG, "path prefix does not match");
    if (xf_strlen(src_path) == vfs->path_prefix_len) {
        // special case when src_path matches the path prefix exactly
        return "/";
    }
    return src_path + vfs->path_prefix_len;
}
