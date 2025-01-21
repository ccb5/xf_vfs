
/**
 * @file xf_vfs.h
 * @author catcatBlue (catcatblue@qq.com)
 * @brief
 * @version 1.0
 * @date 2025-01-10
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

/**
 * @cond (XFAPI_USER || XFAPI_PORT || XFAPI_INTERNAL)
 * @defgroup group_xf_vfs xf_vfs
 * @brief xf_vfs 虚拟文件系统 (Virtual File System).
 * @endcond
 */

#ifndef __XF_VFS_H__
#define __XF_VFS_H__

/* ==================== [Includes] ========================================== */

#include <stdarg.h>
#include <errno.h>

#include "xf_vfs_ops.h"
#include "xf_vfs_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== [Defines] =========================================== */

/* ==================== [Typedefs] ========================================== */

/* ==================== [Global Prototypes] ================================= */

/**
 * @cond (XFAPI_USER || XFAPI_PORT || XFAPI_INTERNAL)
 * @addtogroup group_xf_vfs
 * @endcond
 * @{
 */

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
 * @param vfs  Pointer to xf_vfs_t, a structure which maps syscalls to
 *             the filesystem driver functions. VFS component doesn't
 *             assume ownership of this pointer.
 * @param ctx  If vfs->flags has XF_VFS_FLAG_CONTEXT_PTR set, a pointer
 *             which should be passed to VFS functions. Otherwise, NULL.
 *
 * @return  XF_OK if successful, XF_ERR_NO_MEM if too many VFSes are
 *          registered.
 */
xf_err_t xf_vfs_register(const char *base_path, const xf_vfs_t *vfs, void *ctx);

/**
 * Special case function for registering a VFS that uses a method other than
 * open() to open new file descriptors from the interval <min_fd; max_fd).
 *
 * This is a special-purpose function intended for registering LWIP sockets to VFS.
 *
 * @param vfs Pointer to xf_vfs_t. Meaning is the same as for xf_vfs_register().
 * @param ctx Pointer to context structure. Meaning is the same as for xf_vfs_register().
 * @param min_fd The smallest file descriptor this VFS will use.
 * @param max_fd Upper boundary for file descriptors this VFS will use (the biggest file descriptor plus one).
 *
 * @return  XF_OK if successful, XF_ERR_NO_MEM if too many VFSes are
 *          registered, XF_ERR_INVALID_ARG if the file descriptor boundaries
 *          are incorrect.
 */
xf_err_t xf_vfs_register_fd_range(const xf_vfs_t *vfs, void *ctx, int min_fd, int max_fd);

/**
 * Special case function for registering a VFS that uses a method other than
 * open() to open new file descriptors. In comparison with
 * xf_vfs_register_fd_range, this function doesn't pre-registers an interval
 * of file descriptors. File descriptors can be registered later, by using
 * xf_vfs_register_fd.
 *
 * @param vfs Pointer to xf_vfs_t. Meaning is the same as for xf_vfs_register().
 * @param ctx Pointer to context structure. Meaning is the same as for xf_vfs_register().
 * @param vfs_id Here will be written the VFS ID which can be passed to
 *               xf_vfs_register_fd for registering file descriptors.
 *
 * @return  XF_OK if successful, XF_ERR_NO_MEM if too many VFSes are
 *          registered, XF_ERR_INVALID_ARG if the file descriptor boundaries
 *          are incorrect.
 */
xf_err_t xf_vfs_register_with_id(const xf_vfs_t *vfs, void *ctx, xf_vfs_id_t *vfs_id);

/**
 * Unregister a virtual filesystem for given path prefix
 *
 * @param base_path  file prefix previously used in xf_vfs_register call
 * @return XF_OK if successful, XF_ERR_INVALID_STATE if VFS for given prefix
 *         hasn't been registered
 */
xf_err_t xf_vfs_unregister(const char *base_path);

/**
 * Unregister a virtual filesystem with the given index
 *
 * @param vfs_id  The VFS ID returned by xf_vfs_register_with_id
 * @return XF_OK if successful, XF_ERR_INVALID_STATE if VFS for the given index
 *         hasn't been registered
 */
xf_err_t xf_vfs_unregister_with_id(xf_vfs_id_t vfs_id);

/**
 * Special function for registering another file descriptor for a VFS registered
 * by xf_vfs_register_with_id. This function should only be used to register
 * permanent file descriptors (socket fd) that are not removed after being closed.
 *
 * @param vfs_id VFS identificator returned by xf_vfs_register_with_id.
 * @param fd The registered file descriptor will be written to this address.
 *
 * @return  XF_OK if the registration is successful,
 *          XF_ERR_NO_MEM if too many file descriptors are registered,
 *          XF_ERR_INVALID_ARG if the arguments are incorrect.
 */
xf_err_t xf_vfs_register_fd(xf_vfs_id_t vfs_id, int *fd);

/**
 * Special function for registering another file descriptor with given local_fd
 * for a VFS registered by xf_vfs_register_with_id.
 *
 * @param vfs_id VFS identificator returned by xf_vfs_register_with_id.
 * @param local_fd The fd in the local vfs. Passing -1 will set the local fd as the (*fd) value.
 * @param permanent Whether the fd should be treated as permannet (not removed after close())
 * @param fd The registered file descriptor will be written to this address.
 *
 * @return  XF_OK if the registration is successful,
 *          XF_ERR_NO_MEM if too many file descriptors are registered,
 *          XF_ERR_INVALID_ARG if the arguments are incorrect.
 */
xf_err_t xf_vfs_register_fd_with_local_fd(xf_vfs_id_t vfs_id, int local_fd, bool permanent, int *fd);

/**
 * Special function for unregistering a file descriptor belonging to a VFS
 * registered by xf_vfs_register_with_id.
 *
 * @param vfs_id VFS identificator returned by xf_vfs_register_with_id.
 * @param fd File descriptor which should be unregistered.
 *
 * @return  XF_OK if the registration is successful,
 *          XF_ERR_INVALID_ARG if the arguments are incorrect.
 */
xf_err_t xf_vfs_unregister_fd(xf_vfs_id_t vfs_id, int fd);

/* 
    这些函数原本是系统调用，为了最大跨平台兼容性，现在直接调用。
 */
/**@{*/
xf_vfs_ssize_t xf_vfs_write(int fd, const void *data, size_t size);
xf_vfs_off_t xf_vfs_lseek(int fd, xf_vfs_off_t size, int mode);
xf_vfs_ssize_t xf_vfs_read(int fd, void *dst, size_t size);
int xf_vfs_open(const char *path, int flags, int mode);
int xf_vfs_close(int fd);
int xf_vfs_fstat(int fd, xf_vfs_stat_t *st);
#define xf_vfs_fcntl(fd, cmd, arg) xf_vfs_fcntl_r((fd), (cmd), (arg))
int xf_vfs_fcntl_r(int fd, int cmd, int arg);
int xf_vfs_ioctl(int fd, int cmd, ...);
int xf_vfs_fsync(int fd);

#if XF_VFS_SUPPORT_DIR_IS_ENABLE
int xf_vfs_stat(const char *path, xf_vfs_stat_t *st);
int xf_vfs_link(const char *n1, const char *n2);
int xf_vfs_unlink(const char *path);
int xf_vfs_rename(const char *src, const char *dst);
int xf_vfs_utime(const char *path, const xf_vfs_utimbuf_t *times);
xf_vfs_dir_t *xf_vfs_opendir(const char *name);
xf_vfs_dirent_t *xf_vfs_readdir(xf_vfs_dir_t *pdir);
int xf_vfs_readdir_r(xf_vfs_dir_t *pdir, xf_vfs_dirent_t *entry, xf_vfs_dirent_t **out_dirent);
long xf_vfs_telldir(xf_vfs_dir_t *pdir);
void xf_vfs_seekdir(xf_vfs_dir_t *pdir, long loc);
void xf_vfs_rewinddir(xf_vfs_dir_t *pdir);
int xf_vfs_closedir(xf_vfs_dir_t *pdir);
int xf_vfs_mkdir(const char *name, xf_vfs_mode_t mode);
int xf_vfs_rmdir(const char *name);
int xf_vfs_access(const char *path, int amode);
int xf_vfs_truncate(const char *path, xf_vfs_off_t length);
int xf_vfs_ftruncate(int fd, xf_vfs_off_t length);
#endif
/**@}*/

/**
 *
 * @brief Implements the VFS layer of POSIX pread()
 *
 * @param fd         File descriptor used for read
 * @param dst        Pointer to the buffer where the output will be written
 * @param size       Number of bytes to be read
 * @param offset     Starting offset of the read
 *
 * @return           A positive return value indicates the number of bytes read. -1 is return on failure and errno is
 *                   set accordingly.
 */
xf_vfs_ssize_t xf_vfs_pread(int fd, void *dst, size_t size, xf_vfs_off_t offset);

/**
 *
 * @brief Implements the VFS layer of POSIX pwrite()
 *
 * @param fd         File descriptor used for write
 * @param src        Pointer to the buffer from where the output will be read
 * @param size       Number of bytes to write
 * @param offset     Starting offset of the write
 *
 * @return           A positive return value indicates the number of bytes written. -1 is return on failure and errno is
 *                   set accordingly.
 */
xf_vfs_ssize_t xf_vfs_pwrite(int fd, const void *src, size_t size, xf_vfs_off_t offset);

/**
 *
 * @brief Dump the existing VFS FDs data to FILE* fp
 *
 * Dump the FDs in the format:
 @verbatim
         <VFS Path Prefix>-<FD seen by App>-<FD seen by driver>

    where:
     VFS Path Prefix   : file prefix used in the xf_vfs_register call
     FD seen by App    : file descriptor returned by the vfs to the application for the path prefix
     FD seen by driver : file descriptor used by the driver for the same file prefix.

 @endverbatim
 */
void xf_vfs_dump_fds(void);

/**
 * @brief Dump all registered FSs to the provided FILE*
 *
 * Dump the FSs in the format:
 @verbatim
        <index>:<VFS Path Prefix> -> <VFS entry ptr>

    where:
        index           : internal index in the table of registered FSs (the same as returned when registering fd with id)
        VFS Path Prefix : file prefix used in the xf_vfs_register call or "NULL"
        VFS entry ptr   : pointer to the xf_vfs_fs_ops_t struct used internally when resolving the calls
 @endverbatim
 */
void xf_vfs_dump_registered_paths(void);

#if XF_VFS_SUPPORT_SELECT_IS_ENABLE

/**
 * @brief Synchronous I/O multiplexing which implements the functionality of POSIX select() for VFS
 * @param nfds      Specifies the range of descriptors which should be checked.
 *                  The first nfds descriptors will be checked in each set.
 * @param readfds   If not NULL, then points to a descriptor set that on input
 *                  specifies which descriptors should be checked for being
 *                  ready to read, and on output indicates which descriptors
 *                  are ready to read.
 * @param writefds  If not NULL, then points to a descriptor set that on input
 *                  specifies which descriptors should be checked for being
 *                  ready to write, and on output indicates which descriptors
 *                  are ready to write.
 * @param errorfds  If not NULL, then points to a descriptor set that on input
 *                  specifies which descriptors should be checked for error
 *                  conditions, and on output indicates which descriptors
 *                  have error conditions.
 * @param timeout   If not NULL, then points to timeval structure which
 *                  specifies the time period after which the functions should
 *                  time-out and return. If it is NULL, then the function will
 *                  not time-out. Note that the timeout period is rounded up to
 *                  the system tick and incremented by one.
 *
 * @return      The number of descriptors set in the descriptor sets, or -1
 *              when an error (specified by errno) have occurred.
 */
int xf_vfs_select(int nfds, xf_fd_set *readfds, xf_fd_set *writefds, xf_fd_set *errorfds, xf_vfs_timeval_t *timeout);

/**
 * @brief Notification from a VFS driver about a read/write/error condition
 *
 * This function is called when the VFS driver detects a read/write/error
 * condition as it was requested by the previous call to start_select.
 *
 * @param sem semaphore structure which was passed to the driver by the start_select call
 */
void xf_vfs_select_triggered(xf_vfs_select_sem_t sem);

/**
 * @brief Notification from a VFS driver about a read/write/error condition (ISR version)
 *
 * This function is called when the VFS driver detects a read/write/error
 * condition as it was requested by the previous call to start_select.
 *
 * @param sem semaphore structure which was passed to the driver by the start_select call
 * @param woken is set to pdTRUE if the function wakes up a task with higher priority
 */
void xf_vfs_select_triggered_isr(xf_vfs_select_sem_t sem, int *woken);

#endif /* XF_VFS_SUPPORT_SELECT_IS_ENABLE */

/* ==================== [Macros] ============================================ */

/**
 * End of addtogroup group_xf_vfs
 * @}
 */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __XF_VFS_H__ */
