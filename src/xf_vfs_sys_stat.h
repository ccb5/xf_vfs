/**
 * @file xf_vfs_sys_stat.h
 * @author catcatBlue (catcatblue@qq.com)
 * @brief
 * @version 1.0
 * @date 2025-01-15
 *
 * @copyright Copyright (c) 2025
 *
 */

#ifndef __XF_VFS_SYS_STAT_H__
#define __XF_VFS_SYS_STAT_H__

/* ==================== [Includes] ========================================== */

#include "xf_vfs_config_internal.h"
#include "xf_vfs_sys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== [Defines] =========================================== */

#define XF_VFS__IFMT        0170000 /*!< 文件类型 */
#define     XF_VFS__IFDIR   0040000 /*!< 目录*/
#define     XF_VFS__IFCHR   0020000 /*!< 特殊字符 */
#define     XF_VFS__IFBLK   0060000 /*!< 特殊块 */
#define     XF_VFS__IFREG   0100000 /*!< 常规*/
#define     XF_VFS__IFLNK   0120000 /*!< 符号链接 */
#define     XF_VFS__IFSOCK  0140000 /*!< 套接字 */
#define     XF_VFS__IFIFO   0010000 /*!< 先进先出 */

#define     XF_VFS_S_BLKSIZE  1024  /*!< 块的大小 */

#define XF_VFS_S_ISUID      0004000 /*!< 在执行时设置用户 ID */
#define XF_VFS_S_ISGID      0002000 /*!< 在执行时设置组 ID */
#define XF_VFS_S_ISVTX      0001000 /*!< 即使在使用后也保存交换的文本 */

#define XF_VFS_S_IFMT       XF_VFS__IFMT
#define XF_VFS_S_IFDIR      XF_VFS__IFDIR
#define XF_VFS_S_IFCHR      XF_VFS__IFCHR
#define XF_VFS_S_IFBLK      XF_VFS__IFBLK
#define XF_VFS_S_IFREG      XF_VFS__IFREG
#define XF_VFS_S_IFLNK      XF_VFS__IFLNK
#define XF_VFS_S_IFSOCK XF_VFS__IFSOCK
#define XF_VFS_S_IFIFO      XF_VFS__IFIFO

#define XF_VFS_S_IRWXU     (XF_VFS_S_IRUSR | XF_VFS_S_IWUSR | XF_VFS_S_IXUSR)
#define     XF_VFS_S_IRUSR 0000400  /*!< 读取权限，所有者 */
#define     XF_VFS_S_IWUSR 0000200  /*!< 写权限，所有者 */
#define     XF_VFS_S_IXUSR 0000100  /*!< 执行/搜索权限，所有者 */
#define XF_VFS_S_IRWXG     (XF_VFS_S_IRGRP | XF_VFS_S_IWGRP | XF_VFS_S_IXGRP)
#define     XF_VFS_S_IRGRP 0000040  /*!< 读取权限，组 */
#define     XF_VFS_S_IWGRP 0000020  /*!< 写权限，组 */
#define     XF_VFS_S_IXGRP 0000010  /*!< 执行/搜索权限，组 */
#define XF_VFS_S_IRWXO     (XF_VFS_S_IROTH | XF_VFS_S_IWOTH | XF_VFS_S_IXOTH)
#define     XF_VFS_S_IROTH 0000004  /*!< 读取权限，其他 */
#define     XF_VFS_S_IWOTH 0000002  /*!< 写权限，其他 */
#define     XF_VFS_S_IXOTH 0000001  /*!< 执行/搜索权限，其他 */

/* ==================== [Typedefs] ========================================== */

typedef struct xf_vfs_stat {
    xf_vfs_dev_t        st_dev;        /*!< Device.  */
    xf_vfs_mode_t       st_mode;       /*!< 文件模式 */
    xf_vfs_off_t        st_size;       /*!< 文件字节数 */
    /*
        以下几个时间修改了命名，防止与标准库头文件同时存在时被错误覆盖
        #define st_atime st_actime
        #define st_mtime st_modtime
        #define st_ctime st_chtime
    */
    xf_vfs_time_t       st_actime;     /*!< 上次访问时间 */
    xf_vfs_time_t       st_modtime;    /*!< 最后修改时间 */
    xf_vfs_time_t       st_chtime;     /*!< 最后一次状态改变的时间 */
    xf_vfs_blksize_t    st_blksize;    /*!< I/O 的最佳块大小。 */
    xf_vfs_blkcnt_t     st_blocks;     /*!< 文件块数 */
} xf_vfs_stat_t;

/* ==================== [Global Prototypes] ================================= */

/* ==================== [Macros] ============================================ */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // __XF_VFS_SYS_STAT_H__
