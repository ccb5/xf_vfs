/**
 * @file xf_vfs_ops.h
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

#ifndef __XF_VFS_OPS_H__
#define __XF_VFS_OPS_H__

/* ==================== [Includes] ========================================== */

#include "xf_vfs_config_internal.h"

#include "xf_vfs_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== [Defines] =========================================== */

/* ==================== [Typedefs] ========================================== */

/*
 * @brief VFS identificator used for xf_vfs_register_with_id()
 */
typedef int xf_vfs_id_t;

#if XF_VFS_SUPPORT_DIR_IS_ENABLE
/* *INDENT-OFF* */

typedef              int (*xf_vfs_stat_ctx_op_t)      (void *ctx, const char *path, xf_vfs_stat_t *st);                 /*!< stat with context pointer */
typedef              int (*xf_vfs_stat_op_t)          (           const char *path, xf_vfs_stat_t *st);                 /*!< stat without context pointer */
typedef              int (*xf_vfs_link_ctx_op_t)      (void *ctx, const char *n1, const char *n2);                      /*!< link with context pointer */
typedef              int (*xf_vfs_link_op_t)          (           const char *n1, const char *n2);                      /*!< link without context pointer */
typedef              int (*xf_vfs_unlink_ctx_op_t)    (void *ctx, const char *path);                                    /*!< unlink with context pointer */
typedef              int (*xf_vfs_unlink_op_t)        (           const char *path);                                    /*!< unlink without context pointer */
typedef              int (*xf_vfs_rename_ctx_op_t)    (void *ctx, const char *src, const char *dst);                    /*!< rename with context pointer */
typedef              int (*xf_vfs_rename_op_t)        (           const char *src, const char *dst);                    /*!< rename without context pointer */
typedef    xf_vfs_dir_t* (*xf_vfs_opendir_ctx_op_t)   (void *ctx, const char *name);                                    /*!< opendir with context pointer */
typedef    xf_vfs_dir_t* (*xf_vfs_opendir_op_t)       (           const char *name);                                    /*!< opendir without context pointer */
typedef xf_vfs_dirent_t* (*xf_vfs_readdir_ctx_op_t)   (void *ctx, xf_vfs_dir_t *pdir);                                  /*!< readdir with context pointer */
typedef xf_vfs_dirent_t* (*xf_vfs_readdir_op_t)       (           xf_vfs_dir_t *pdir);                                  /*!< readdir without context pointer */
typedef              int (*xf_vfs_readdir_r_ctx_op_t) (void *ctx, xf_vfs_dir_t *pdir, xf_vfs_dirent_t *entry, xf_vfs_dirent_t **out); /*!< readdir_r with context pointer */
typedef              int (*xf_vfs_readdir_r_op_t)     (           xf_vfs_dir_t *pdir, xf_vfs_dirent_t *entry, xf_vfs_dirent_t **out); /*!< readdir_r without context pointer */
typedef             long (*xf_vfs_telldir_ctx_op_t)   (void *ctx, xf_vfs_dir_t *pdir);                                  /*!< telldir with context pointer */
typedef             long (*xf_vfs_telldir_op_t)       (           xf_vfs_dir_t *pdir);                                  /*!< telldir without context pointer */
typedef             void (*xf_vfs_seekdir_ctx_op_t)   (void *ctx, xf_vfs_dir_t *pdir, long offset);                     /*!< seekdir with context pointer */
typedef             void (*xf_vfs_seekdir_op_t)       (           xf_vfs_dir_t *pdir, long offset);                     /*!< seekdir without context pointer */
typedef              int (*xf_vfs_closedir_ctx_op_t)  (void *ctx, xf_vfs_dir_t *pdir);                                  /*!< closedir with context pointer */
typedef              int (*xf_vfs_closedir_op_t)      (           xf_vfs_dir_t *pdir);                                  /*!< closedir without context pointer */
typedef              int (*xf_vfs_mkdir_ctx_op_t)     (void *ctx, const char *name, xf_vfs_mode_t mode);                /*!< mkdir with context pointer */
typedef              int (*xf_vfs_mkdir_op_t)         (           const char *name, xf_vfs_mode_t mode);                /*!< mkdir without context pointer */
typedef              int (*xf_vfs_rmdir_ctx_op_t)     (void *ctx, const char *name);                                    /*!< rmdir with context pointer */
typedef              int (*xf_vfs_rmdir_op_t)         (           const char *name);                                    /*!< rmdir without context pointer */
typedef              int (*xf_vfs_access_ctx_op_t)    (void *ctx, const char *path, int amode);                         /*!< access with context pointer */
typedef              int (*xf_vfs_access_op_t)        (           const char *path, int amode);                         /*!< access without context pointer */
typedef              int (*xf_vfs_truncate_ctx_op_t)  (void *ctx, const char *path, xf_vfs_off_t length);               /*!< truncate with context pointer */
typedef              int (*xf_vfs_truncate_op_t)      (           const char *path, xf_vfs_off_t length);               /*!< truncate without context pointer */
typedef              int (*xf_vfs_ftruncate_ctx_op_t) (void *ctx, int fd, xf_vfs_off_t length);                         /*!< ftruncate with context pointer */
typedef              int (*xf_vfs_ftruncate_op_t)     (           int fd, xf_vfs_off_t length);                         /*!< ftruncate without context pointer */
typedef              int (*xf_vfs_utime_ctx_op_t)     (void *ctx, const char *path, const xf_vfs_utimbuf_t *times);     /*!< utime with context pointer */
typedef              int (*xf_vfs_utime_op_t)         (           const char *path, const xf_vfs_utimbuf_t *times);     /*!< utime without context pointer */

/**
 * @brief Struct containing function pointers to directory related functionality.
 */
typedef struct {
    union {
        const xf_vfs_stat_ctx_op_t      stat_p;      /*!< stat with context pointer */
        const xf_vfs_stat_op_t          stat;        /*!< stat without context pointer */
    };
    union {
        const xf_vfs_link_ctx_op_t      link_p;      /*!< link with context pointer */
        const xf_vfs_link_op_t          link;        /*!< link without context pointer */
    };
    union {
        const xf_vfs_unlink_ctx_op_t    unlink_p;    /*!< unlink with context pointer */
        const xf_vfs_unlink_op_t        unlink;      /*!< unlink without context pointer */
    };
    union {
        const xf_vfs_rename_ctx_op_t    rename_p;    /*!< rename with context pointer */
        const xf_vfs_rename_op_t        rename;      /*!< rename without context pointer */
    };
    union {
        const xf_vfs_opendir_ctx_op_t   opendir_p;   /*!< opendir with context pointer */
        const xf_vfs_opendir_op_t       opendir;     /*!< opendir without context pointer */
    };
    union {
        const xf_vfs_readdir_ctx_op_t   readdir_p;   /*!< readdir with context pointer */
        const xf_vfs_readdir_op_t       readdir;     /*!< readdir without context pointer */
    };
    union {
        const xf_vfs_readdir_r_ctx_op_t readdir_r_p; /*!< readdir_r with context pointer */
        const xf_vfs_readdir_r_op_t     readdir_r;   /*!< readdir_r without context pointer */
    };
    union {
        const xf_vfs_telldir_ctx_op_t   telldir_p;   /*!< telldir with context pointer */
        const xf_vfs_telldir_op_t       telldir;     /*!< telldir without context pointer */
    };
    union {
        const xf_vfs_seekdir_ctx_op_t   seekdir_p;   /*!< seekdir with context pointer */
        const xf_vfs_seekdir_op_t       seekdir;     /*!< seekdir without context pointer */
    };
    union {
        const xf_vfs_closedir_ctx_op_t  closedir_p;  /*!< closedir with context pointer */
        const xf_vfs_closedir_op_t      closedir;    /*!< closedir without context pointer */
    };
    union {
        const xf_vfs_mkdir_ctx_op_t     mkdir_p;     /*!< mkdir with context pointer */
        const xf_vfs_mkdir_op_t         mkdir;       /*!< mkdir without context pointer */
    };
    union {
        const xf_vfs_rmdir_ctx_op_t     rmdir_p;     /*!< rmdir with context pointer */
        const xf_vfs_rmdir_op_t         rmdir;       /*!< rmdir without context pointer */
    };
    union {
        const xf_vfs_access_ctx_op_t    access_p;    /*!< access with context pointer */
        const xf_vfs_access_op_t        access;      /*!< access without context pointer */
    };
    union {
        const xf_vfs_truncate_ctx_op_t  truncate_p;  /*!< truncate with context pointer */
        const xf_vfs_truncate_op_t      truncate;    /*!< truncate without context pointer */
    };
    union {
        const xf_vfs_ftruncate_ctx_op_t ftruncate_p; /*!< ftruncate with context pointer */
        const xf_vfs_ftruncate_op_t     ftruncate;   /*!< ftruncate without context pointer */
    };
    union {
        const xf_vfs_utime_ctx_op_t     utime_p;     /*!< utime with context pointer */
        const xf_vfs_utime_op_t         utime;       /*!< utime without context pointer */
    };
} xf_vfs_dir_ops_t;

/* *INDENT-ON* */
#endif // CONFIG_XF_VFS_SUPPORT_DIR

/* *INDENT-OFF* */

typedef xf_vfs_ssize_t (*xf_vfs_write_ctx_op_t)  (void *ctx, int fd, const void *data, size_t size);                /*!< Write with context pointer */
typedef xf_vfs_ssize_t (*xf_vfs_write_op_t)      (           int fd, const void *data, size_t size);                /*!< Write without context pointer */
typedef   xf_vfs_off_t (*xf_vfs_lseek_ctx_op_t)  (void *ctx, int fd, xf_vfs_off_t size, int mode);                  /*!< Seek with context pointer */
typedef   xf_vfs_off_t (*xf_vfs_lseek_op_t)      (           int fd, xf_vfs_off_t size, int mode);                  /*!< Seek without context pointer */
typedef xf_vfs_ssize_t (*xf_vfs_read_ctx_op_t)   (void *ctx, int fd, void *dst, size_t size);                       /*!< Read with context pointer */
typedef xf_vfs_ssize_t (*xf_vfs_read_op_t)       (           int fd, void *dst, size_t size);                       /*!< Read without context pointer */
typedef xf_vfs_ssize_t (*xf_vfs_pread_ctx_op_t)  (void *ctx, int fd, void *dst, size_t size, xf_vfs_off_t offset);       /*!< pread with context pointer */
typedef xf_vfs_ssize_t (*xf_vfs_pread_op_t)      (           int fd, void *dst, size_t size, xf_vfs_off_t offset);       /*!< pread without context pointer */
typedef xf_vfs_ssize_t (*xf_vfs_pwrite_ctx_op_t) (void *ctx, int fd, const void *src, size_t size, xf_vfs_off_t offset); /*!< pwrite with context pointer */
typedef xf_vfs_ssize_t (*xf_vfs_pwrite_op_t)     (           int fd, const void *src, size_t size, xf_vfs_off_t offset); /*!< pwrite without context pointer */
typedef            int (*xf_vfs_open_ctx_op_t)   (void *ctx, const char *path, int flags, int mode);                /*!< open with context pointer */
typedef            int (*xf_vfs_open_op_t)       (           const char *path, int flags, int mode);                /*!< open without context pointer */
typedef            int (*xf_vfs_close_ctx_op_t)  (void *ctx, int fd);                                               /*!< close with context pointer */
typedef            int (*xf_vfs_close_op_t)      (           int fd);                                               /*!< close without context pointer */
typedef            int (*xf_vfs_fstat_ctx_op_t)  (void *ctx, int fd, xf_vfs_stat_t *st);                            /*!< fstat with context pointer */
typedef            int (*xf_vfs_fstat_op_t)      (           int fd, xf_vfs_stat_t *st);                            /*!< fstat without context pointer */
typedef            int (*xf_vfs_fcntl_ctx_op_t)  (void *ctx, int fd, int cmd, int arg);                             /*!< fcntl with context pointer */
typedef            int (*xf_vfs_fcntl_op_t)      (           int fd, int cmd, int arg);                             /*!< fcntl without context pointer */
typedef            int (*xf_vfs_ioctl_ctx_op_t)  (void *ctx, int fd, int cmd, va_list args);                        /*!< ioctl with context pointer */
typedef            int (*xf_vfs_ioctl_op_t)      (           int fd, int cmd, va_list args);                        /*!< ioctl without context pointer */
typedef            int (*xf_vfs_fsync_ctx_op_t)  (void *ctx, int fd);                                               /*!< fsync with context pointer */
typedef            int (*xf_vfs_fsync_op_t)      (           int fd);                                               /*!< fsync without context pointer */

/**
 * @brief Main struct of the minified vfs API, containing basic function pointers as well as pointers to the other subcomponents.
 */
typedef struct {
    union {
        const xf_vfs_write_ctx_op_t  write_p;  /*!< Write with context pointer */
        const xf_vfs_write_op_t      write;    /*!< Write without context pointer */
    };
    union {
        const xf_vfs_lseek_ctx_op_t  lseek_p;  /*!< Seek with context pointer */
        const xf_vfs_lseek_op_t      lseek;    /*!< Seek without context pointer */
    };
    union {
        const xf_vfs_read_ctx_op_t   read_p;   /*!< Read with context pointer */
        const xf_vfs_read_op_t       read;     /*!< Read without context pointer */
    };
    union {
        const xf_vfs_pread_ctx_op_t  pread_p;  /*!< pread with context pointer */
        const xf_vfs_pread_op_t      pread;    /*!< pread without context pointer */
    };
    union {
        const xf_vfs_pwrite_ctx_op_t pwrite_p; /*!< pwrite with context pointer */
        const xf_vfs_pwrite_op_t     pwrite;   /*!< pwrite without context pointer */
    };
    union {
        const xf_vfs_open_ctx_op_t   open_p;   /*!< open with context pointer */
        const xf_vfs_open_op_t       open;     /*!< open without context pointer */
    };
    union {
        const xf_vfs_close_ctx_op_t  close_p;  /*!< close with context pointer */
        const xf_vfs_close_op_t      close;    /*!< close without context pointer */
    };
    union {
        const xf_vfs_fstat_ctx_op_t  fstat_p;  /*!< fstat with context pointer */
        const xf_vfs_fstat_op_t      fstat;    /*!< fstat without context pointer */
    };
    union {
        const xf_vfs_fcntl_ctx_op_t  fcntl_p;  /*!< fcntl with context pointer */
        const xf_vfs_fcntl_op_t      fcntl;    /*!< fcntl without context pointer */
    };
    union {
        const xf_vfs_ioctl_ctx_op_t  ioctl_p;  /*!< ioctl with context pointer */
        const xf_vfs_ioctl_op_t      ioctl;    /*!< ioctl without context pointer */
    };
    union {
        const xf_vfs_fsync_ctx_op_t  fsync_p;  /*!< fsync with context pointer */
        const xf_vfs_fsync_op_t      fsync;    /*!< fsync without context pointer */
    };

#if XF_VFS_SUPPORT_DIR_IS_ENABLE
    const xf_vfs_dir_ops_t *const dir;         /*!< pointer to the dir subcomponent */
#endif

#if XF_VFS_SUPPORT_SELECT_IS_ENABLE || defined __DOXYGEN__
    const xf_vfs_select_ops_t *const select;   /*!< pointer to the select subcomponent */
#endif

} xf_vfs_fs_ops_t;

/* *INDENT-ON* */

/* ==================== [Global Prototypes] ================================= */

/**
 * Register a virtual filesystem for given path prefix.
 *
 * @param base_path  file path prefix associated with the filesystem.
 *                   Must be a zero-terminated C string, may be empty.
 *                   If not empty, must be up to XF_VFS_PATH_MAX
 *                   characters long, and at least 2 characters long.
 *                   Name must start with a "/" and must not end with "/".
 *                   For example, "/data" or "/dev/spi" are valid.
 *                   These VFSes would then be called to handle file paths such as
 *                   "/data/myfile.txt" or "/dev/spi/0".
 *                   In the special case of an empty base_path, a "fallback"
 *                   VFS is registered. Such VFS will handle paths which are not
 *                   matched by any other registered VFS.
 * @param vfs  Pointer to xf_vfs_fs_ops_t, a structure which maps syscalls to
 *             the filesystem driver functions. VFS component does not assume ownership of this struct, but see flags for more info
 *
 * @param flags Set of binary flags controlling how the registered FS should be treated
 *             - XF_VFS_FLAG_STATIC - if this flag is specified VFS assumes the provided xf_vfs_fs_ops_t and all its subcomponents are statically allocated,
 *                                     if it is not enabled a deep copy of the provided struct will be created, which will be managed by the VFS component
 *             - XF_VFS_FLAG_CONTEXT_PTR - If set, the VFS will use the context-aware versions of the filesystem operation functions (suffixed with `_p`) in `xf_vfs_fs_ops_t` and its subcomponents.
 *                                          The `ctx` parameter will be passed as the context argument when these functions are invoked.
 *
 * @param ctx  Context pointer for fs operation functions, see the XF_VFS_FLAG_CONTEXT_PTR.
 *             Should be `NULL` if not used.
 *
 * @return  XF_OK if successful, XF_ERR_NO_MEM if too many FSes are
 *          registered.
 */
xf_err_t xf_vfs_register_fs(const char *base_path, const xf_vfs_fs_ops_t *vfs, int flags, void *ctx);

/**
 * Analog of xf_vfs_register_with_id which accepts xf_vfs_fs_ops_t instead.
 *
 */
xf_err_t xf_vfs_register_fs_with_id(const xf_vfs_fs_ops_t *vfs, int flags, void *ctx, xf_vfs_id_t *id);

/**
 * Alias for xf_vfs_unregister for naming consistency
 */
xf_err_t xf_vfs_unregister_fs(const char *base_path);

/**
 * Alias for xf_vfs_unregister_with_id for naming consistency
 */
xf_err_t xf_vfs_unregister_fs_with_id(xf_vfs_id_t id);

/* ==================== [Macros] ============================================ */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __XF_VFS_OPS_H__ */
