add_rules("mode.debug", "mode.release")
includes("xf_utils/xf_utils/xmake.lua")

-- xf_vfs 所有的内容
function add_xf_vfs() 
    add_xf_utils("xf_utils/xf_utils")
    add_files("src/**.c")
    add_includedirs("src")
end 

-- 模板化添加示例工程
function add_target(name) 
    target(name)
        set_kind("binary")
        add_cflags("-Wall")
        add_cflags("-std=gnu99 -O0")
        add_xf_vfs()
        add_files(string.format("example/%s/*.c", name))
        add_includedirs(string.format("example/%s", name))
end 

add_target("test_vfs_paths")
