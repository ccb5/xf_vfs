/**
 * @file xf_vfs_config_internal.h
 * @author catcatBlue (catcatblue@qq.com)
 * @brief xf_vfs 模块内部配置总头文件。
 *        确保 xf_vfs_config.h 的所有定义都有默认值。
 * @version 1.0
 * @date 2025-01-10
 *
 * @copyright Copyright (c) 2025
 *
 */

#ifndef __XF_VFS_CONFIG_INTERNAL_H__
#define __XF_VFS_CONFIG_INTERNAL_H__

/* ==================== [Includes] ========================================== */

#include "xf_vfs_config.h"
#include "xf_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== [Defines] =========================================== */

#if (!defined(XF_VFS_SUPPORT_IO_ENABLE)) || (XF_VFS_SUPPORT_IO_ENABLE) || defined(__DOXYGEN__)
#   define XF_VFS_SUPPORT_IO_IS_ENABLE      (1)
#else
#   define XF_VFS_SUPPORT_IO_IS_ENABLE      (0)
#endif

#if (!defined(XF_VFS_SUPPORT_DIR_ENABLE)) || (XF_VFS_SUPPORT_DIR_ENABLE) || defined(__DOXYGEN__)
#   define XF_VFS_SUPPORT_DIR_IS_ENABLE     (1)
#else
#   define XF_VFS_SUPPORT_DIR_IS_ENABLE     (0)
#endif

#if (!defined(XF_VFS_SUPPORT_SELECT_ENABLE)) || (XF_VFS_SUPPORT_SELECT_ENABLE) || defined(__DOXYGEN__)
#   define XF_VFS_SUPPORT_SELECT_IS_ENABLE     (1)
#else
#   define XF_VFS_SUPPORT_SELECT_IS_ENABLE     (0)
#endif

#if !defined(XF_VFS_MAX_COUNT) || defined(__DOXYGEN__)
#   define XF_VFS_MAX_COUNT                 (8)
#endif

#if (!defined(XF_VFS_CUSTOM_FD_SETSIZE_ENABLE)) || (XF_VFS_CUSTOM_FD_SETSIZE_ENABLE) || defined(__DOXYGEN__)
#   define XF_VFS_CUSTOM_FD_SETSIZE_IS_ENABLE   (1)
#else
#   define XF_VFS_CUSTOM_FD_SETSIZE_IS_ENABLE   (0)
#endif

/**
 * Maximum number of (global) file descriptors.
 * for compatibility with fd_set and select()
 */
#if XF_VFS_CUSTOM_FD_SETSIZE_IS_ENABLE || defined(__DOXYGEN__)
#   define XF_VFS_FDS_MAX               XF_VFS_CUSTOM_FD_SETSIZE
#else
#   define XF_VFS_FDS_MAX               FD_SETSIZE
#endif

#if !defined(XF_VFS_CUSTOM_FD_SETSIZE) || defined(__DOXYGEN__)
#   define XF_VFS_CUSTOM_FD_SETSIZE         (64)
#endif

#if !defined(XF_VFS_PATH_MAX) || defined(__DOXYGEN__)
#   define XF_VFS_PATH_MAX                  (15)
#endif

#if !defined(XF_VFS_DIRENT_NAME_SIZE) || defined(__DOXYGEN__)
#   define XF_VFS_DIRENT_NAME_SIZE          (256)
#endif

/* ==================== [Typedefs] ========================================== */

/* ==================== [Global Prototypes] ================================= */

/* ==================== [Macros] ============================================ */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // __XF_VFS_CONFIG_INTERNAL_H__
