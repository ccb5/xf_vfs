/**
 * @file xf_vfs_sys__timeval.h
 * @author catcatBlue (catcatblue@qq.com)
 * @brief 
 * @version 1.0
 * @date 2025-01-15
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef __XF_VFS_SYS__TIMEVAL_H__
#define __XF_VFS_SYS__TIMEVAL_H__

/* ==================== [Includes] ========================================== */

#include "xf_vfs_config_internal.h"

#include "xf_vfs_sys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== [Defines] =========================================== */

/* ==================== [Typedefs] ========================================== */

typedef struct xf_vfs_timeval {
    xf_vfs_time_t      tv_sec;     /* seconds */
    xf_vfs_suseconds_t tv_usec;    /* and microseconds */
} xf_vfs_timeval_t;

/* ==================== [Global Prototypes] ================================= */

/* ==================== [Macros] ============================================ */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // __XF_VFS_SYS__TIMEVAL_H__
