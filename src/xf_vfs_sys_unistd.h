/**
 * @file xf_vfs_sys_unistd.h
 * @author catcatBlue (catcatblue@qq.com)
 * @brief
 * @version 1.0
 * @date 2025-01-15
 *
 * @copyright Copyright (c) 2025
 *
 */

#ifndef __XF_VFS_SYS_UNISTD_H__
#define __XF_VFS_SYS_UNISTD_H__

/* ==================== [Includes] ========================================== */

#include "xf_vfs_config_internal.h"

#include "xf_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== [Defines] =========================================== */

/**
 * @name amode_flags
 * @see sys/unistd.h
 * @{
 */
#define XF_VFS_F_OK     0
#define XF_VFS_R_OK     4
#define XF_VFS_W_OK     2
#define XF_VFS_X_OK     1
/**
 * End of amode_flags.
 * @}
 */

/**
 * @name seek_flags
 * @see sys/unistd.h
 * @{
 */
#define XF_VFS_SEEK_SET 0    /*!< 偏移量设置为偏移字节 */
#define XF_VFS_SEEK_CUR 1    /*!< 偏移量设置为其当前位置加上偏移字节 */
#define XF_VFS_SEEK_END 2    /*!< 偏移量设置为文件大小加上偏移字节 */
/**
 * End of seek_flags.
 * @}
 */

/* ==================== [Typedefs] ========================================== */

/* ==================== [Global Prototypes] ================================= */

/* ==================== [Macros] ============================================ */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // __XF_VFS_SYS_UNISTD_H__
