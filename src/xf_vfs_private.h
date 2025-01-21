/**
 * @file xf_vfs_private.h
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

#ifndef __XF_VFS_PRIVATE_H__
#define __XF_VFS_PRIVATE_H__

/* ==================== [Includes] ========================================== */

#include "xf_vfs_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond (XFAPI_PORT || XFAPI_INTERNAL)
 * @addtogroup group_xf_vfs
 * @endcond
 * @{
 */

/* ==================== [Defines] =========================================== */

/* ==================== [Typedefs] ========================================== */

typedef struct _xf_vfs_entry_t {
    int flags;              /*!< XF_VFS_FLAG_CONTEXT_PTR and/or XF_VFS_FLAG_READONLY_FS or XF_VFS_FLAG_DEFAULT */
    const xf_vfs_fs_ops_t *vfs;          // contains pointers to VFS functions
    char path_prefix[XF_VFS_PATH_MAX]; // path prefix mapped to this VFS
    size_t path_prefix_len; // micro-optimization to avoid doing extra strlen
    void *ctx;              // optional pointer which can be passed to VFS
    int offset;             // index of this structure in s_vfs array
} xf_vfs_entry_t;

/**
 * Register a virtual filesystem.
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
 * @param len  Length of the base_path.
 * @param vfs  Pointer to xf_vfs_t, a structure which maps syscalls to
 *             the filesystem driver functions. VFS component doesn't
 *             assume ownership of this pointer.
 * @param ctx  If vfs->flags has XF_VFS_FLAG_CONTEXT_PTR set, a pointer
 *             which should be passed to VFS functions. Otherwise, NULL.
 * @param vfs_index Index for getting the vfs content.
 *
 * @return  XF_OK if successful.
 *          XF_ERR_NO_MEM if too many VFSes are registered.
 *          XF_ERR_INVALID_ARG if given an invalid parameter.
 */
xf_err_t xf_vfs_register_common(const char *base_path, size_t len, const xf_vfs_t *vfs, void *ctx, int *vfs_index);

/**
 * Get vfs fd with given path.
 *
 * @param path file path prefix associated with the filesystem.
 *
 * @return Pointer to the `xf_vfs_entry_t` corresponding to the given path, which cannot be NULL.
 */
const xf_vfs_entry_t *xf_vfs_get_vfs_for_path(const char *path);

/**
 * Get vfs fd with given vfs index.
 *
 * @param index VFS index.
 *
 * @return Pointer to the `xf_vfs_entry_t` corresponding to the given path, which cannot be NULL.
 */
const xf_vfs_entry_t *xf_vfs_get_vfs_for_index(int index);

/* ==================== [Global Prototypes] ================================= */

/* ==================== [Macros] ============================================ */

/**
 * End of addtogroup group_xf_vfs
 * @}
 */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __XF_VFS_PRIVATE_H__ */
