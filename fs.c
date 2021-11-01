/*
 * @Author: your name
 * @Date: 2021-10-22 14:42:33
 * @LastEditTime: 2021-10-22 18:53:14
 * @LastEditors: Corvo Attano(fkxzz001@qq.com)
 * @Description: In User Settings Edit
 * @FilePath: \SiMpleFileSystem\fs.c
 */

#include<linux/fs.h>
#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/slab.h>

#include"common.h"
struct dentry* smfs_mount(struct file_system_type *fsType, int flags, const char *devName, void *data)
{
    struct dentry* dentry=mount_bdev(fsType,flags,devName,data,smfs_fill_super);
    if(IS_ERR(dentry)) pr_err("mount failure");
    else pr_info("mount success");
    return dentry;
}
void smfs_kill_sb(struct super_block *sb)
{
    kill_block_super(sb);
    pr_info("unmount success");
}
static struct file_system_type smfs_file_system_type = {
    .owner = THIS_MODULE,
    .name = "SMFS",
    .mount = smfs_mount,
    .kill_sb = smfs_kill_sb,
    .fs_flags = FS_REQUIRES_DEV,
    .next = NULL,
};
static int __init smfs_init(void)
{
    int ret = smfs_init_inode_cache();
    if (ret) {
        pr_err("inode cache creation failed");
        return ret;
    }

    ret = register_filesystem(&smfs_file_system_type);
    if (ret) {
        pr_err("register_filesystem() failed");
        return ret;
    }

    pr_info("module loaded\n");
    return ret;
}

static void __exit smfs_exit(void)
{
    int ret = unregister_filesystem(&smfs_file_system_type);
    if (ret)
        pr_err("unregister_filesystem() failed\n");

    smfs_destroy_inode_cache();

    pr_info("module unloaded\n");
}

module_init(smfs_init);
module_exit(smfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Corvo Attano (391063482@qq.com)");
MODULE_DESCRIPTION("SiMple FileSystem");