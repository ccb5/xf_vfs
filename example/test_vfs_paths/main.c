/**
 * @file xf_main.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-10-23
 *
 * Copyright (c) 2023, CorAL. All rights reserved.
 *
 */

/* ==================== [Includes] ========================================== */

#include "xf_utils.h"
#include "xf_vfs.h"

/* ==================== [Defines] =========================================== */

#define TAG "main"

/* ==================== [Typedefs] ========================================== */

/* Dummy VFS implementation to check if VFS is called or not with expected path
 */
typedef struct {
    const char *match_path;
    bool called;
} dummy_vfs_t;

/* ==================== [Static Prototypes] ================================= */

static int dummy_open(void *ctx, const char *path, int flags, int mode);
static int dummy_close(void *ctx, int fd);
static xf_vfs_dir_t *dummy_opendir(void *ctx, const char *path);
static int dummy_closedir(void *ctx, xf_vfs_dir_t *pdir);
static void test_open(dummy_vfs_t *instance, const char *path,
                      bool should_be_called, bool should_be_opened, int line);
static void test_opendir(dummy_vfs_t *instance, const char *path,
                         bool should_be_called, bool should_be_opened, int line);

static void test_vfs_register(const char *prefix, bool expect_success, int line);

static void TEST_CASE_vfs_parses_paths_correctly(void);
static void TEST_CASE_vfs_unregisters_correct_nested_mount_point(void);
static void TEST_CASE_vfs_checks_mount_point_path(void);
static int test_main(void);

/* ==================== [Static Variables] ================================== */

/* ==================== [Macros] ============================================ */

#define TEST_XF_OK(x) \
    do { \
        if (x != XF_OK) { \
            xf_log_printf("Test failed at line %d\n", __LINE__); \
            while (1); \
        } \
    } while (0)

#define TEST_XF_ERR(err, rc) \
    do { \
        if (err!= rc) { \
            xf_log_printf("Test failed at line %d\n", __LINE__); \
            while (1); \
        } \
    } while (0)

#define TEST_ASSERT_NOT_NULL(pointer) \
    do { \
        if ((pointer) == NULL) { \
            xf_log_printf("Test failed at line %d\n", __LINE__); \
            while (1); \
        } \
    } while (0)

#define TEST_ASSERT_EQUAL(expected, actual) \
    do { \
        if ((expected)!=(actual)) { \
            xf_log_printf("Test failed at line %d\n", __LINE__); \
            while (1); \
        } \
    } while (0)

#define UNITY_TEST_ASSERT_EQUAL_INT(expected, actual, line, message) \
    do { \
        if ((expected) != (actual)) { \
            xf_log_printf("ASSERT_EQUAL_INT failed: %s:%d: %s expected %d but was %d\n", \
                   message, line, #expected, (expected), (actual)); \
            while (1); \
        } \
    } while (0)

#define UNITY_TEST_ASSERT(condition, line, message) \
    do { \
        if (!(condition)) { \
            xf_log_printf("ASSERT failed: %s:%d: %s\n", \
                   message, line, #condition); \
            while (1); \
        } \
    } while (0)

#define test_register_ok(prefix) test_vfs_register(prefix, true, __LINE__)
#define test_register_fail(prefix) test_vfs_register(prefix, false, __LINE__)

/* Initializer for this dummy VFS implementation
 */

#define DUMMY_VFS() { \
        .flags = XF_VFS_FLAG_CONTEXT_PTR, \
        .open_p = dummy_open, \
        .close_p = dummy_close, \
        .opendir_p = dummy_opendir, \
        .closedir_p = dummy_closedir \
    }

/* Helper macros which forward line number to assertion macros inside test_open
 * and test_opendir
 */

#define test_opened(instance, path) test_open(instance, path, true, true, __LINE__)
#define test_not_opened(instance, path) test_open(instance, path, true, false, __LINE__)
#define test_not_called(instance, path) test_open(instance, path, false, false, __LINE__)

#define test_dir_opened(instance, path) test_opendir(instance, path, true, true, __LINE__)
#define test_dir_not_opened(instance, path) test_opendir(instance, path, true, false, __LINE__)
#define test_dir_not_called(instance, path) test_opendir(instance, path, false, false, __LINE__)

/* ==================== [Global Functions] ================================== */

int main(void)
{
    return test_main();
}

/* ==================== [Static Functions] ================================== */

static int test_main(void)
{
    TEST_CASE_vfs_parses_paths_correctly();
    TEST_CASE_vfs_unregisters_correct_nested_mount_point();
    TEST_CASE_vfs_checks_mount_point_path();
    return 0;
}

static int dummy_open(void *ctx, const char *path, int flags, int mode)
{
    dummy_vfs_t *dummy = (dummy_vfs_t *) ctx;
    dummy->called = true;
    if (xf_strcmp(dummy->match_path, path) == 0) {
        return 1;
    }
    errno = ENOENT;
    return -1;
}

static int dummy_close(void *ctx, int fd)
{
    dummy_vfs_t *dummy = (dummy_vfs_t *) ctx;
    dummy->called = true;
    if (fd == 1) {
        return 0;
    }
    errno = EBADF;
    return -1;
}

static xf_vfs_dir_t *dummy_opendir(void *ctx, const char *path)
{
    dummy_vfs_t *dummy = (dummy_vfs_t *) ctx;
    dummy->called = true;
    if (xf_strcmp(dummy->match_path, path) == 0) {
        xf_vfs_dir_t *result = xf_malloc(sizeof(xf_vfs_dir_t));
        TEST_ASSERT_NOT_NULL(result);
        return result;
    }
    errno = ENOENT;
    return NULL;
}

static int dummy_closedir(void *ctx, xf_vfs_dir_t *pdir)
{
    dummy_vfs_t *dummy = (dummy_vfs_t *) ctx;
    dummy->called = true;
    xf_free(pdir);
    return 0;
}

/* Helper functions to test VFS behavior
 */

static void test_open(dummy_vfs_t *instance, const char *path,
                      bool should_be_called, bool should_be_opened, int line)
{
    const int flags = XF_VFS_O_CREAT | XF_VFS_O_TRUNC | XF_VFS_O_RDWR;
    instance->called = false;
    int fd = xf_vfs_open(path, flags, 0);
    UNITY_TEST_ASSERT_EQUAL_INT(should_be_called, instance->called, line,
                                "should_be_called check failed");
    if (should_be_called) {
        if (should_be_opened) {
            UNITY_TEST_ASSERT(fd >= 0, line, "should be opened");
        } else {
            UNITY_TEST_ASSERT(fd < 0, line, "should not be opened");
        }
    }
    xf_vfs_close(fd);
}

static void test_opendir(dummy_vfs_t *instance, const char *path,
                         bool should_be_called, bool should_be_opened, int line)
{
    instance->called = false;
    xf_vfs_dir_t *dir = xf_vfs_opendir(path);
    UNITY_TEST_ASSERT_EQUAL_INT(should_be_called, instance->called, line,
                                "should_be_called check failed");
    if (should_be_called) {
        if (should_be_opened) {
            UNITY_TEST_ASSERT(dir != NULL, line, "should be opened");
        } else {
            UNITY_TEST_ASSERT(dir == 0, line, "should not be opened");
        }
    }
    if (dir) {
        xf_vfs_closedir(dir);
    }
}

static void TEST_CASE_vfs_parses_paths_correctly(void)
{
    dummy_vfs_t inst_foo = {
        .match_path = "",
        .called = false
    };
    xf_vfs_t desc_foo = DUMMY_VFS();
    TEST_XF_OK(xf_vfs_register("/foo", &desc_foo, &inst_foo));

    dummy_vfs_t inst_foo1 = {
        .match_path = "",
        .called = false
    };
    xf_vfs_t desc_foo1 = DUMMY_VFS();
    TEST_XF_OK(xf_vfs_register("/foo1", &desc_foo1, &inst_foo1));

    inst_foo.match_path = "/file";
    test_opened(&inst_foo, "/foo/file");
    test_not_opened(&inst_foo, "/foo/file1");
    test_not_called(&inst_foo, "/foo1/file");
    test_not_called(&inst_foo, "/foo1");
    test_not_opened(&inst_foo, "/foo");
    inst_foo.match_path = "/junk";
    test_dir_opened(&inst_foo, "/foo/junk");
    inst_foo.match_path = "/";
    test_dir_opened(&inst_foo, "/foo/");
    test_dir_opened(&inst_foo, "/foo");
    test_dir_not_called(&inst_foo1, "/foo");
    test_dir_not_opened(&inst_foo, "/foo/1");
    test_dir_not_called(&inst_foo, "/foo1");

    inst_foo1.match_path = "/file1";
    test_not_called(&inst_foo1, "/foo/file1");
    test_opened(&inst_foo1, "/foo1/file1");
    test_not_opened(&inst_foo1, "/foo1/file");

    // Test nested VFS entries
    dummy_vfs_t inst_foobar = {
        .match_path = "",
        .called = false
    };
    xf_vfs_t desc_foobar = DUMMY_VFS();
    TEST_XF_OK(xf_vfs_register("/foo/bar", &desc_foobar, &inst_foobar));

    dummy_vfs_t inst_toplevel = {
        .match_path = "",
        .called = false
    };
    xf_vfs_t desc_toplevel = DUMMY_VFS();
    TEST_XF_OK(xf_vfs_register("", &desc_toplevel, &inst_toplevel));

    inst_foo.match_path = "/bar/file";
    inst_foobar.match_path = "/file";
    test_not_called(&inst_foo, "/foo/bar/file");
    test_opened(&inst_foobar, "/foo/bar/file");
    test_dir_not_called(&inst_foo, "/foo/bar/file");
    test_dir_opened(&inst_foobar, "/foo/bar/file");
    inst_toplevel.match_path = "/tmp/foo";
    test_opened(&inst_toplevel, "/tmp/foo");
    inst_toplevel.match_path = "foo";
    test_opened(&inst_toplevel, "foo");

    TEST_XF_OK(xf_vfs_unregister("/foo"));
    TEST_XF_OK(xf_vfs_unregister("/foo1"));
    TEST_XF_OK(xf_vfs_unregister("/foo/bar"));
    TEST_XF_OK(xf_vfs_unregister(""));

    XF_LOGI(TAG, "%s passed", __FUNCTION__);
}

static void TEST_CASE_vfs_unregisters_correct_nested_mount_point(void)
{
    dummy_vfs_t inst_foobar = {
        .match_path = "/file",
        .called = false
    };
    xf_vfs_t desc_foobar = DUMMY_VFS();
    TEST_XF_OK(xf_vfs_register("/foo/bar", &desc_foobar, &inst_foobar));

    dummy_vfs_t inst_foo = {
        .match_path = "/bar/file",
        .called = false
    };
    xf_vfs_t desc_foo = DUMMY_VFS();
    TEST_XF_OK(xf_vfs_register("/foo", &desc_foo, &inst_foo));

    /* basic operation */
    test_opened(&inst_foobar, "/foo/bar/file");
    test_not_called(&inst_foo, "/foo/bar/file");

    /* this should not match anything */
    TEST_XF_ERR(XF_ERR_INVALID_STATE, xf_vfs_unregister("/foo/b"));

    /* unregister "/foo" and check that we haven't unregistered "/foo/bar" */
    TEST_XF_OK(xf_vfs_unregister("/foo"));
    test_not_called(&inst_foo, "/foo/bar/file");
    test_opened(&inst_foobar, "/foo/bar/file");

    /* repeat the above, with the reverse order of registration */
    TEST_XF_OK(xf_vfs_unregister("/foo/bar"));
    TEST_XF_OK(xf_vfs_register("/foo", &desc_foo, &inst_foo));
    TEST_XF_OK(xf_vfs_register("/foo/bar", &desc_foobar, &inst_foobar));
    test_opened(&inst_foobar, "/foo/bar/file");
    test_not_called(&inst_foo, "/foo/bar/file");
    TEST_XF_OK(xf_vfs_unregister("/foo"));
    test_not_called(&inst_foo, "/foo/bar/file");
    test_opened(&inst_foobar, "/foo/bar/file");
    TEST_XF_OK(xf_vfs_unregister("/foo/bar"));

    XF_LOGI(TAG, "%s passed", __FUNCTION__);
}

static void test_vfs_register(const char *prefix, bool expect_success, int line)
{
    dummy_vfs_t inst;
    xf_vfs_t desc = DUMMY_VFS();
    xf_err_t err = xf_vfs_register(prefix, &desc, &inst);
    if (expect_success) {
        UNITY_TEST_ASSERT_EQUAL_INT(XF_OK, err, line, "xf_vfs_register should succeed");
    } else {
        UNITY_TEST_ASSERT_EQUAL_INT(XF_ERR_INVALID_ARG,
                                    err, line, "xf_vfs_register should fail");
    }
    if (err == XF_OK) {
        TEST_XF_OK(xf_vfs_unregister(prefix));
    }
}

static void TEST_CASE_vfs_checks_mount_point_path(void)
{
    test_register_ok("");
    test_register_fail("/");
    test_register_fail("a");
    test_register_fail("aa");
    test_register_fail("aaa");
    test_register_ok("/a");
    test_register_ok("/aa");
    test_register_ok("/aaa/bbb");
    test_register_fail("/aaa/");
    test_register_fail("/aaa/bbb/");
    test_register_ok("/23456789012345");
    test_register_fail("/234567890123456");

    XF_LOGI(TAG, "%s passed", __FUNCTION__);
}
