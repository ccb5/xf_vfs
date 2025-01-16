/**
 * @file xf_vfs_sys_select.h
 * @author catcatBlue (catcatblue@qq.com)
 * @brief 
 * @version 1.0
 * @date 2025-01-15
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef __XF_VFS_SYS_SELECT_H__
#define __XF_VFS_SYS_SELECT_H__

/* ==================== [Includes] ========================================== */

#include "xf_vfs_config_internal.h"

#include "xf_vfs_sys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name sys_select
 * @see sys/select.h
 * @{
 */

/* ==================== [Defines] =========================================== */

/* ==================== [Typedefs] ========================================== */

#if XF_VFS_CUSTOM_FD_SETSIZE_IS_ENABLE
#   define XF_FD_SETSIZE                XF_VFS_CUSTOM_FD_SETSIZE
#else
#   define XF_FD_SETSIZE                FD_SETSIZE
#endif

typedef unsigned long xf__fd_mask;
typedef xf__fd_mask xf_fd_mask;

#define XF__NFDBITS ((int)sizeof(xf__fd_mask) * 8) /* bits per mask */
#define XF_NFDBITS      XF__NFDBITS

#ifndef xf__howmany
#define xf__howmany(x,y)    (((x) + ((y) - 1)) / (y))
#endif

typedef struct xf_fd_set {
    xf__fd_mask __fds_bits[xf__howmany(XF_FD_SETSIZE, XF__NFDBITS)];
} xf_fd_set;
#define xf_fds_bits    __fds_bits

#define xf___fdset_mask(n) ((xf__fd_mask)1 << ((n) % XF__NFDBITS))
#define XF_FD_CLR(n, p)    ((p)->__fds_bits[(n)/XF__NFDBITS] &= ~xf___fdset_mask(n))
#define XF_FD_COPY(f, t)   (void)(*(t) = *(f))
#define XF_FD_ISSET(n, p)  (((p)->__fds_bits[(n)/XF__NFDBITS] & xf___fdset_mask(n)) != 0)
#define XF_FD_SET(n, p) ((p)->__fds_bits[(n)/XF__NFDBITS] |= xf___fdset_mask(n))
#define XF_FD_ZERO(p) do {                                  \
        xf_fd_set *_p;                                      \
        size_t _n;                                          \
                                                            \
        _p = (p);                                           \
        _n = xf__howmany(XF_FD_SETSIZE, XF__NFDBITS);       \
        while (_n > 0)                                      \
                _p->__fds_bits[--_n] = 0;                   \
} while (0)

/* ==================== [Global Prototypes] ================================= */

/* ==================== [Macros] ============================================ */

/**
 * End of sys_select.
 * @}
 */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // __XF_VFS_SYS_SELECT_H__
