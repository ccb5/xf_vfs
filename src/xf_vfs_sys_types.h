/**
 * @file xf_vfs_sys_types.h
 * @author catcatBlue (catcatblue@qq.com)
 * @brief 
 * @version 1.0
 * @date 2025-01-15
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef __XF_VFS_SYS_TYPES_H__
#define __XF_VFS_SYS_TYPES_H__

/* ==================== [Includes] ========================================== */

#include "xf_vfs_config_internal.h"

#include "xf_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== [Defines] =========================================== */

/* ==================== [Typedefs] ========================================== */

typedef signed int      xf_vfs_ssize_t;
typedef short           xf_vfs_dev_t;
typedef unsigned long   xf_vfs_ino_t;
typedef long            xf_vfs_off_t;
typedef unsigned long   xf_vfs_time_t;
typedef long            xf_vfs_suseconds_t;
typedef unsigned long   xf_vfs_useconds_t;
typedef unsigned long   xf_vfs_mode_t;

typedef long            xf_vfs_off_t;

typedef long            xf_vfs_blksize_t;
typedef long            xf_vfs_blkcnt_t;

/* ==================== [Global Prototypes] ================================= */

/* ==================== [Macros] ============================================ */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // __XF_VFS_SYS_TYPES_H__
