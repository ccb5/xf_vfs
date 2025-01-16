/**
 * @file xf_vfs_sys_utime.h
 * @author catcatBlue (catcatblue@qq.com)
 * @brief 
 * @version 1.0
 * @date 2025-01-15
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef __XF_VFS_SYS_UTIME_H__
#define __XF_VFS_SYS_UTIME_H__

/* ==================== [Includes] ========================================== */

#include "xf_vfs_config_internal.h"

#include "xf_vfs_sys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== [Defines] =========================================== */

/* ==================== [Typedefs] ========================================== */

/**
 * @brief aos_utimbuf 结构描述了文件系统 inode 的
 * 最后访问时间和最后修改时间。
 */
typedef struct xf_vfs_utimbuf {
    xf_vfs_time_t actime;  /*!< time of last access */
    xf_vfs_time_t modtime; /*!< time of last modification */
} xf_vfs_utimbuf_t;

/* ==================== [Global Prototypes] ================================= */

/* ==================== [Macros] ============================================ */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // __XF_VFS_SYS_UTIME_H__
