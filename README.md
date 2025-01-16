# 虚拟文件系统 xf_vfs

本仓库依据 Apache-2.0 许可证修改 ESP-IDF v5.4 的 VFS 组件，来源可在同级目录下 `NOTICE` 找到。

虚拟文件系统组件详情参见：[虚拟文件系统组件 - ESP32 - — ESP-IDF 编程指南 v5.4 文档](https://docs.espressif.com/projects/esp-idf/zh_CN/v5.4/esp32/api-reference/storage/vfs.html)

特点：

1.  通过 xf_vfs 可以像操作文件对象一样操作驱动程序。
1.  仅依赖 `xf_utils` 和 `xf_osal` 组件。如果不需要启用 `select` 可以不需要 `xf_osal` 组件。
1.  高可移植性。

    1.  移除了大部分标准库依赖，并在 xf_vfs 内定义了代替相关标准库头文件的头文件，使得在 ARMCC 编译器上使用 VFS 成为可能。

        ```
        📦src
        ┣ 📜xf_vfs.c
        ┣ 📜xf_vfs.h
        ┣ 📜xf_vfs_config_internal.h
        ┣ 📜xf_vfs_ops.h
        ┣ 📜xf_vfs_private.h
        ┣ 📜xf_vfs_sys__timeval.h       # 代替标准库
        ┣ 📜xf_vfs_sys_dirent.h         # 代替标准库
        ┣ 📜xf_vfs_sys_fcntl.h          # 代替标准库
        ┣ 📜xf_vfs_sys_select.h         # 代替标准库
        ┣ 📜xf_vfs_sys_stat.h           # 代替标准库
        ┣ 📜xf_vfs_sys_types.h          # 代替标准库
        ┣ 📜xf_vfs_sys_unistd.h         # 代替标准库
        ┣ 📜xf_vfs_sys_utime.h          # 代替标准库
        ┗ 📜xf_vfs_types.h
        ```

    1.  所需的标准库头文件：除了 xf_utils 中所包含的标准库头文件外，还需 stdarg.h 和 errno.h.

1.  不直接兼容 posix 相应接口，可以与 posix 相应接口共存，或者对接到 posix 接口。
1.  精简：仅保留了 IO 操作、目录操作（可选）、select（可选）。

## 运行例程

可以选择 [直接使用 xmake](#直接使用-xmake) 的方式或者在 xfusion 中运行。

### 直接使用 xmake

1.  安装[xmake](https://xmake.io/#/zh-cn/getting_started)

1.  克隆 `xf_utils` 仓库

    ```shell
    git clone https://github.com/x-eks-fusion/xf_utils.git
    ```

1.  编译工程

    ```shell
    xmake b test_vfs_paths
    ```

1.  运行工程

    ```shell
    xmake r test_vfs_paths
    ```

## xf_vfs 提供的示例

1.  test_vfs_paths

    演示如何注册 IO 到 xf_vfs 中。

## 注意

### 关于版权

版权以及来源已在同级目录下 `NOTICE` 中声明。

原始许可证位于同级目录下 `LICENSE`。
