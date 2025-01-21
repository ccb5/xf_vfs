/**
 * @file xf_vfs_types.h
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

#ifndef __XF_VFS_TYPES_H__
#define __XF_VFS_TYPES_H__

/* ==================== [Includes] ========================================== */

#include "xf_vfs_config_internal.h"

#include "xf_vfs_sys__timeval.h"
#include "xf_vfs_sys_dirent.h"
#include "xf_vfs_sys_fcntl.h"
#include "xf_vfs_sys_select.h"
#include "xf_vfs_sys_stat.h"
#include "xf_vfs_sys_types.h"
#include "xf_vfs_sys_unistd.h"
#include "xf_vfs_sys_utime.h"

#if XF_VFS_SUPPORT_SELECT_IS_ENABLE
#include "xf_osal.h"
#endif // XF_VFS_SUPPORT_SELECT_IS_ENABLE

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond (XFAPI_USER || XFAPI_PORT || XFAPI_INTERNAL)
 * @addtogroup group_xf_vfs
 * @endcond
 * @{
 */

/* ==================== [Defines] =========================================== */

/* special length value for VFS which is never recognised by open() */
#define XF_VFS_PATH_PREFIX_LEN_IGNORED  (~(size_t)0)

/**
 * Default value of flags member in xf_vfs_t structure.
 */
#define XF_VFS_FLAG_DEFAULT             (1 << 0)

/**
 * Flag which indicates that FS needs extra context pointer in syscalls.
 */
#define XF_VFS_FLAG_CONTEXT_PTR         (1 << 1)

/**
 * Flag which indicates that FS is located on read-only partition.
 */
#define XF_VFS_FLAG_READONLY_FS         (1 << 2)

/**
 * Flag which indicates that VFS structure should be freed upon unregistering.
 * @note Free if false, do not free if true
 */
#define XF_VFS_FLAG_STATIC              (1 << 3)

/* ==================== [Typedefs] ========================================== */

#if XF_VFS_SUPPORT_SELECT_IS_ENABLE
/**
 * @brief VFS semaphore type for select()
 */
typedef struct {
    bool is_sem_local;      /*!< type of "sem" is SemaphoreHandle_t when true, defined by socket driver otherwise */
    void *sem;              /*!< semaphore instance */
} xf_vfs_select_sem_t;
#endif // XF_VFS_SUPPORT_SELECT_IS_ENABLE

/*
 * @brief VFS identificator used for xf_vfs_register_with_id()
 */
typedef int xf_vfs_id_t;

/* *INDENT-OFF* */

/**
 * @brief VFS definition structure
 *
 * This structure should be filled with pointers to corresponding
 * FS driver functions.
 *
 * VFS component will translate all FDs so that the filesystem implementation
 * sees them starting at zero. The caller sees a global FD which is prefixed
 * with an pre-filesystem-implementation.
 *
 * Some FS implementations expect some state (e.g. pointer to some structure)
 * to be passed in as a first argument. For these implementations,
 * populate the members of this structure which have _p suffix, set
 * flags member to XF_VFS_FLAG_CONTEXT_PTR and provide the context pointer
 * to xf_vfs_register function.
 * If the implementation doesn't use this extra argument, populate the
 * members without _p suffix and set flags member to XF_VFS_FLAG_DEFAULT.
 *
 * If the FS driver doesn't provide some of the functions, set corresponding
 * members to NULL.
 */
typedef struct
{
    int flags;      /*!< XF_VFS_FLAG_CONTEXT_PTR and/or XF_VFS_FLAG_READONLY_FS or XF_VFS_FLAG_DEFAULT */
    union {
        xf_vfs_ssize_t (*write_p)(void* p, int fd, const void * data, size_t size);                         /*!< Write with context pointer */
        xf_vfs_ssize_t (*write)(int fd, const void * data, size_t size);                                    /*!< Write without context pointer */
    };
    union {
        xf_vfs_off_t (*lseek_p)(void* p, int fd, xf_vfs_off_t size, int mode);                              /*!< Seek with context pointer */
        xf_vfs_off_t (*lseek)(int fd, xf_vfs_off_t size, int mode);                                         /*!< Seek without context pointer */
    };
    union {
        xf_vfs_ssize_t (*read_p)(void* ctx, int fd, void * dst, size_t size);                               /*!< Read with context pointer */
        xf_vfs_ssize_t (*read)(int fd, void * dst, size_t size);                                            /*!< Read without context pointer */
    };
    union {
        xf_vfs_ssize_t (*pread_p)(void *ctx, int fd, void * dst, size_t size, xf_vfs_off_t offset);         /*!< pread with context pointer */
        xf_vfs_ssize_t (*pread)(int fd, void * dst, size_t size, xf_vfs_off_t offset);                      /*!< pread without context pointer */
    };
    union {
        xf_vfs_ssize_t (*pwrite_p)(void *ctx, int fd, const void *src, size_t size, xf_vfs_off_t offset);   /*!< pwrite with context pointer */
        xf_vfs_ssize_t (*pwrite)(int fd, const void *src, size_t size, xf_vfs_off_t offset);                /*!< pwrite without context pointer */
    };
    union {
        int (*open_p)(void* ctx, const char * path, int flags, int mode);                           /*!< open with context pointer */
        int (*open)(const char * path, int flags, int mode);                                        /*!< open without context pointer */
    };
    union {
        int (*close_p)(void* ctx, int fd);                                                          /*!< close with context pointer */
        int (*close)(int fd);                                                                       /*!< close without context pointer */
    };
    union {
        int (*fstat_p)(void* ctx, int fd, xf_vfs_stat_t * st);                                      /*!< fstat with context pointer */
        int (*fstat)(int fd, xf_vfs_stat_t * st);                                                   /*!< fstat without context pointer */
    };
#if XF_VFS_SUPPORT_DIR_IS_ENABLE
    union {
        int (*stat_p)(void* ctx, const char * path, xf_vfs_stat_t * st);                            /*!< stat with context pointer */
        int (*stat)(const char * path, xf_vfs_stat_t * st);                                         /*!< stat without context pointer */
    };
    union {
        int (*link_p)(void* ctx, const char* n1, const char* n2);                                   /*!< link with context pointer */
        int (*link)(const char* n1, const char* n2);                                                /*!< link without context pointer */
    };
    union {
        int (*unlink_p)(void* ctx, const char *path);                                               /*!< unlink with context pointer */
        int (*unlink)(const char *path);                                                            /*!< unlink without context pointer */
    };
    union {
        int (*rename_p)(void* ctx, const char *src, const char *dst);                               /*!< rename with context pointer */
        int (*rename)(const char *src, const char *dst);                                            /*!< rename without context pointer */
    };
    union {
        xf_vfs_dir_t* (*opendir_p)(void* ctx, const char* name);                                    /*!< opendir with context pointer */
        xf_vfs_dir_t* (*opendir)(const char* name);                                                 /*!< opendir without context pointer */
    };
    union {
        xf_vfs_dirent_t* (*readdir_p)(void* ctx, xf_vfs_dir_t* pdir);                               /*!< readdir with context pointer */
        xf_vfs_dirent_t* (*readdir)(xf_vfs_dir_t* pdir);                                            /*!< readdir without context pointer */
    };
    union {
        int (*readdir_r_p)(void* ctx, xf_vfs_dir_t* pdir, xf_vfs_dirent_t* entry, xf_vfs_dirent_t** out_dirent); /*!< readdir_r with context pointer */
        int (*readdir_r)(xf_vfs_dir_t* pdir, xf_vfs_dirent_t* entry, xf_vfs_dirent_t** out_dirent);              /*!< readdir_r without context pointer */
    };
    union {
        long (*telldir_p)(void* ctx, xf_vfs_dir_t* pdir);                                           /*!< telldir with context pointer */
        long (*telldir)(xf_vfs_dir_t* pdir);                                                        /*!< telldir without context pointer */
    };
    union {
        void (*seekdir_p)(void* ctx, xf_vfs_dir_t* pdir, long offset);                              /*!< seekdir with context pointer */
        void (*seekdir)(xf_vfs_dir_t* pdir, long offset);                                           /*!< seekdir without context pointer */
    };
    union {
        int (*closedir_p)(void* ctx, xf_vfs_dir_t* pdir);                                           /*!< closedir with context pointer */
        int (*closedir)(xf_vfs_dir_t* pdir);                                                        /*!< closedir without context pointer */
    };
    union {
        int (*mkdir_p)(void* ctx, const char* name, xf_vfs_mode_t mode);                            /*!< mkdir with context pointer */
        int (*mkdir)(const char* name, xf_vfs_mode_t mode);                                         /*!< mkdir without context pointer */
    };
    union {
        int (*rmdir_p)(void* ctx, const char* name);                                                /*!< rmdir with context pointer */
        int (*rmdir)(const char* name);                                                             /*!< rmdir without context pointer */
    };
#endif // CONFIG_XF_VFS_SUPPORT_DIR
    union {
        int (*fcntl_p)(void* ctx, int fd, int cmd, int arg);                                        /*!< fcntl with context pointer */
        int (*fcntl)(int fd, int cmd, int arg);                                                     /*!< fcntl without context pointer */
    };
    union {
        int (*ioctl_p)(void* ctx, int fd, int cmd, va_list args);                                   /*!< ioctl with context pointer */
        int (*ioctl)(int fd, int cmd, va_list args);                                                /*!< ioctl without context pointer */
    };
    union {
        int (*fsync_p)(void* ctx, int fd);                                                          /*!< fsync with context pointer */
        int (*fsync)(int fd);                                                                       /*!< fsync without context pointer */
    };
#if XF_VFS_SUPPORT_DIR_IS_ENABLE
    union {
        int (*access_p)(void* ctx, const char *path, int amode);                                    /*!< access with context pointer */
        int (*access)(const char *path, int amode);                                                 /*!< access without context pointer */
    };
    union {
        int (*truncate_p)(void* ctx, const char *path, xf_vfs_off_t length);                        /*!< truncate with context pointer */
        int (*truncate)(const char *path, xf_vfs_off_t length);                                     /*!< truncate without context pointer */
    };
    union {
        int (*ftruncate_p)(void* ctx, int fd, xf_vfs_off_t length);                                 /*!< ftruncate with context pointer */
        int (*ftruncate)(int fd, xf_vfs_off_t length);                                              /*!< ftruncate without context pointer */
    };
    union {
        int (*utime_p)(void* ctx, const char *path, const xf_vfs_utimbuf_t *times);                   /*!< utime with context pointer */
        int (*utime)(const char *path, const xf_vfs_utimbuf_t *times);                                /*!< utime without context pointer */
    };
#endif // CONFIG_XF_VFS_SUPPORT_DIR
#if XF_VFS_SUPPORT_SELECT_IS_ENABLE || defined __DOXYGEN__
    /** start_select is called for setting up synchronous I/O multiplexing of the desired file descriptors in the given VFS */
    xf_err_t (*start_select)(int nfds, xf_fd_set *readfds, xf_fd_set *writefds, xf_fd_set *exceptfds, xf_vfs_select_sem_t sem, void **end_select_args);
    /** socket select function for socket FDs with the functionality of POSIX select(); this should be set only for the socket VFS */
    int (*socket_select)(int nfds, xf_fd_set *readfds, xf_fd_set *writefds, xf_fd_set *errorfds, xf_vfs_timeval_t *timeout);
    /** called by VFS to interrupt the socket_select call when select is activated from a non-socket VFS driver; set only for the socket driver */
    void (*stop_socket_select)(void *sem);
    /** stop_socket_select which can be called from ISR; set only for the socket driver */
    void (*stop_socket_select_isr)(void *sem, int *woken);
    /** end_select is called to stop the I/O multiplexing and deinitialize the environment created by start_select for the given VFS */
    void* (*get_socket_select_semaphore)(void);
    /** get_socket_select_semaphore returns semaphore allocated in the socket driver; set only for the socket driver */
    xf_err_t (*end_select)(void *end_select_args);
#endif // XF_VFS_SUPPORT_SELECT_IS_ENABLE || defined __DOXYGEN__
} xf_vfs_t;

/* *INDENT-ON* */

#if XF_VFS_SUPPORT_SELECT_IS_ENABLE
/* *INDENT-OFF* */

typedef  xf_err_t  (*xf_vfs_start_select_op_t)                (int nfds, xf_fd_set *readfds, xf_fd_set *writefds, xf_fd_set *exceptfds, xf_vfs_select_sem_t sem, void **end_select_args);
typedef       int  (*xf_vfs_socket_select_op_t)               (int nfds, xf_fd_set *readfds, xf_fd_set *writefds, xf_fd_set *errorfds, xf_vfs_timeval_t *timeout);
typedef      void  (*xf_vfs_stop_socket_select_op_t)          (void *sem);
typedef      void  (*xf_vfs_stop_socket_select_isr_op_t)      (void *sem, int *woken);
typedef      void* (*xf_vfs_get_socket_select_semaphore_op_t) (void);
typedef  xf_err_t  (*xf_vfs_end_select_op_t)                  (void *end_select_args);

/**
 * @brief Struct containing function pointers to select related functionality.
 *
 */
typedef struct {
    /** start_select is called for setting up synchronous I/O multiplexing of the desired file descriptors in the given VFS */
    const xf_vfs_start_select_op_t                start_select;

    /** socket select function for socket FDs with the functionality of POSIX select(); this should be set only for the socket VFS */
    const xf_vfs_socket_select_op_t               socket_select;

    /** called by VFS to interrupt the socket_select call when select is activated from a non-socket VFS driver; set only for the socket driver */
    const xf_vfs_stop_socket_select_op_t          stop_socket_select;

    /** stop_socket_select which can be called from ISR; set only for the socket driver */
    const xf_vfs_stop_socket_select_isr_op_t      stop_socket_select_isr;

    /** end_select is called to stop the I/O multiplexing and deinitialize the environment created by start_select for the given VFS */
    const xf_vfs_get_socket_select_semaphore_op_t get_socket_select_semaphore;

    /** get_socket_select_semaphore returns semaphore allocated in the socket driver; set only for the socket driver */
    const xf_vfs_end_select_op_t                  end_select;
} xf_vfs_select_ops_t;

/* *INDENT-ON* */
#endif // XF_VFS_SUPPORT_SELECT_IS_ENABLE

/* ==================== [Global Prototypes] ================================= */

/* ==================== [Macros] ============================================ */

/**
 * End of addtogroup group_xf_vfs
 * @}
 */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __XF_VFS_TYPES_H__ */
