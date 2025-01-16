/**
 * @file xf_vfs_sys_dirent.h
 * @author catcatBlue (catcatblue@qq.com)
 * @brief 
 * @version 1.0
 * @date 2025-01-15
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef __XF_VFS_SYS_DIRENT_H__
#define __XF_VFS_SYS_DIRENT_H__

/* ==================== [Includes] ========================================== */

#include "xf_vfs_config_internal.h"

#include "xf_vfs_sys_types.h"

#include "xf_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== [Defines] =========================================== */

/* ==================== [Typedefs] ========================================== */

/**
 * @brief Opaque directory structure
 */
typedef struct {
    uint16_t dd_vfs_idx; /*!< VFS index, not to be used by applications */
    uint16_t dd_rsv;     /*!< field reserved for future extension */
    /* remaining fields are defined by VFS implementation */
} xf_vfs_dir_t;

/**
 * @brief Directory entry structure
 */
typedef struct {
    xf_vfs_ino_t        d_ino;
    xf_vfs_off_t        d_off;
    uint8_t             d_type;
#define XF_VFS_DT_UNKNOWN  0
#define XF_VFS_DT_REG      1
#define XF_VFS_DT_DIR      2
    uint8_t             d_namlen;
    uint16_t            d_reclen;
    char                d_name[XF_VFS_DIRENT_NAME_SIZE]; /*!< The null-terminated file name */
} xf_vfs_dirent_t;

/* ==================== [Global Prototypes] ================================= */

/* ==================== [Macros] ============================================ */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // __XF_VFS_SYS_DIRENT_H__
