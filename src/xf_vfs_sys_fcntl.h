/**
 * @file xf_vfs_sys_fcntl.h
 * @author catcatBlue (catcatblue@qq.com)
 * @brief
 * @version 1.0
 * @date 2025-01-15
 *
 * @copyright Copyright (c) 2025
 *
 */

#ifndef __XF_VFS_SYS_FCNTL_H__
#define __XF_VFS_SYS_FCNTL_H__

/* ==================== [Includes] ========================================== */

#include "xf_vfs_config_internal.h"

#include "xf_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== [Defines] =========================================== */

/**
 * @name sys_fcntl
 * @see sys/fcntl.h
 * @{
 */
#define XF_VFS__FOPEN       (-1)    /*!< 来自 sys/file.h，仅内核使用 */
#define XF_VFS__FREAD       0x0001  /*!< 读使能 */
#define XF_VFS__FWRITE      0x0002  /*!< 写使能 */
#define XF_VFS__FAPPEND     0x0008  /*!< 附加（保证在末尾写入）*/
#define XF_VFS__FMARK       0x0010  /*!< 内部的;在 gc() 期间标记 */
#define XF_VFS__FDEFER      0x0020  /*!< 内部的;推迟下一次GC传递*/
#define XF_VFS__FASYNC      0x0040  /*!< 当数据准备好时发出信号 pgrp */
#define XF_VFS__FSHLOCK     0x0080  /*!< BSD fancy() 共享锁存在 */
#define XF_VFS__FEXLOCK     0x0100  /*!< BSD fancy() 独占锁存在 */
#define XF_VFS__FCREAT      0x0200  /*!< 使用文件创建打开 */
#define XF_VFS__FTRUNC      0x0400  /*!< 以截断方式打开 */
#define XF_VFS__FEXCL       0x0800  /*!< 如果文件存在则打开错误 */
#define XF_VFS__FNBIO       0x1000  /*!< 非阻塞 I/O (sys5 风格) */
#define XF_VFS__FSYNC       0x2000  /*!< 同步执行所有写入操作 */
#define XF_VFS__FNONBLOCK   0x4000  /*!< 非阻塞 I/O (POSIX 风格) */
#define XF_VFS__FNDELAY     XF_VFS__FNONBLOCK  /*!< 非阻塞 I/O（4.2 样式） */
#define XF_VFS__FNOCTTY     0x8000  /*!< 不要在此打开上分配 ctty */

#define XF_VFS__FNOINHERIT  0x40000
#define XF_VFS__FDIRECT     0x80000
#define XF_VFS__FNOFOLLOW   0x100000
#define XF_VFS__FDIRECTORY  0x200000
#define XF_VFS__FEXECSRCH   0x400000

#define XF_VFS_O_ACCMODE    (XF_VFS_O_RDONLY|XF_VFS_O_WRONLY|XF_VFS_O_RDWR)

#define XF_VFS_O_RDONLY     0       /*!< +1 == FREAD */
#define XF_VFS_O_WRONLY     1       /*!< +1 == FWRITE */
#define XF_VFS_O_RDWR       2       /*!< +1 == FREAD|FWRITE */
#define XF_VFS_O_APPEND     XF_VFS__FAPPEND
#define XF_VFS_O_CREAT      XF_VFS__FCREAT
#define XF_VFS_O_TRUNC      XF_VFS__FTRUNC
#define XF_VFS_O_EXCL       XF_VFS__FEXCL
#define XF_VFS_O_SYNC       XF_VFS__FSYNC
#define XF_VFS_O_NONBLOCK   XF_VFS__FNONBLOCK
#define XF_VFS_O_NOCTTY     XF_VFS__FNOCTTY

#define XF_VFS_O_CLOEXEC    XF_VFS__FNOINHERIT
#define XF_VFS_O_NOFOLLOW   XF_VFS__FNOFOLLOW
#define XF_VFS_O_DIRECTORY  XF_VFS__FDIRECTORY
#define XF_VFS_O_EXEC       XF_VFS__FEXECSRCH
#define XF_VFS_O_SEARCH     XF_VFS__FEXECSRCH

#define XF_VFS_O_DIRECT     XF_VFS__FDIRECT

#define XF_VFS_FNONBLOCK    XF_VFS__FNONBLOCK

#define XF_VFS_FD_CLOEXEC   1   /* posix */

/* fcntl(2) requests */
#define XF_VFS_F_DUPFD      0   /*!< 重复字段 */
#define XF_VFS_F_GETFD      1   /*!< 获取 fildes 标志（执行时关闭） */
#define XF_VFS_F_SETFD      2   /*!< 设置 fildes 标志（执行时关闭）*/
#define XF_VFS_F_GETFL      3   /*!< 获取文件标志 */
#define XF_VFS_F_SETFL      4   /*!< 设置文件标志 */
#define XF_VFS_F_GETOWN     5   /*!< 获取所有者 -对于异步 */
#define XF_VFS_F_SETOWN     6   /*!< 设置所有者 -异步 */
#define XF_VFS_F_GETLK      7   /*!< 获取记录锁定信息 */
#define XF_VFS_F_SETLK      8   /*!< 设置或清除记录锁（非阻塞） */
#define XF_VFS_F_SETLKW     9   /*!< 设置或清除记录锁（阻塞） */
#define XF_VFS_F_DUPFD_CLOEXEC  14  /*!< 与 F_DUPFD 相同，但设置 close-on-exec 标志 */

#define XF_VFS_F_RDLCK      1   /*!< read lock */
#define XF_VFS_F_WRLCK      2   /*!< write lock */
#define XF_VFS_F_UNLCK      3   /*!< remove lock(s) */

#define XF_VFS_AT_FDCWD     -2

#define XF_VFS_AT_EACCESS              1
#define XF_VFS_AT_SYMLINK_NOFOLLOW     2
#define XF_VFS_AT_SYMLINK_FOLLOW       4
#define XF_VFS_AT_REMOVEDIR            8
#define XF_VFS_AT_EMPTY_PATH          16

#define XF_VFS_LOCK_SH     0x01        /*!< 共享文件锁 */
#define XF_VFS_LOCK_EX     0x02        /*!< 独占文件锁 */
#define XF_VFS_LOCK_NB     0x04        /*!< 加锁时不阻塞 */
#define XF_VFS_LOCK_UN     0x08        /*!< 解锁文件 */
/**
 * End of sys_fcntl.
 * @}
 */

/* ==================== [Typedefs] ========================================== */

/* ==================== [Global Prototypes] ================================= */

/* ==================== [Macros] ============================================ */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // __XF_VFS_SYS_FCNTL_H__
