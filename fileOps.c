/*
 * @Author: Corvo Attano(fkxzz001@qq.com)
 * @Description: 
 * @LastEditors: Corvo Attano(fkxzz001@qq.com)
 */
#include<linux/buffer_head.h>
#include<linux/fs.h>
#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/mpage.h>
#include<linux/printk.h>
#include"bitmap.h"
#include"common.h"
/**
 * @description: map the i-th block to buffer
 * @param {inode} *inode
 * @param {sector_t} iblock
 * @param {buffer_head} *bh
 * @param {int} create
 * @return {*}
 */
int smfs_getBlock(struct inode *inode,sector_t iblock,struct buffer_head *bh,int create)
{
    printk("smfs_getBlock begin");
    struct super_block *sb=inode->i_sb;
    struct smfs_superBlock *smfsSb=sb->s_fs_info;
    struct smfs_inode_kernel *smfsInode=SMFS_INODE(inode);
    uint32_t index,ptr;
    if(iblock>=SMFS_INODE_DATA_SEC)
    {
        return -EFBIG;
    }

    if(iblock >= inode->i_blocks)// if block is not allocted and create is true
    {
        if(create==0)
        {
            return 0;
        }
        index=getFreedata(smfsSb);
        if(index==0)
        {
            return -EFBIG;
        }
        smfsInode->pFlieblockptr[iblock]=index + 1 + smfsSb->iInodestoreblocks + smfsSb->iInodebitmapblocks + smfsSb->iDatabitmapblocks;
    }
    ptr=smfsInode->pFlieblockptr[iblock];
    //mark_inode_dirty(inode);
    map_bh(bh,sb,ptr);
    printk("smfs_getBlock end");
    return 0;
}
int smfs_readpage(struct file *file,struct page *page)
{
    return mpage_readpage(page,smfs_getBlock);
}
int smfs_writepage(struct page *page,struct writeback_control *wbc)
{
    return mpage_writepage(page,smfs_getBlock,wbc);
}
int smfs_write_begin(struct file *file,
                     struct address_space *mapping,
                     loff_t pos,
                     unsigned int len,
                     unsigned int flags,
                     struct page **pagep,
                     void **fsdata)
{
    printk("smfs_write_begin Begin");
    uint32_t allocCount=0;
    if(pos+len>SMFS_MAX_FILE_SIZE) return -ENOSPC;
    int ret=block_write_begin(mapping,pos,len,flags,pagep,smfs_getBlock);
    printk("smfs_write_begin return");
    return ret;
}
int smfs_write_end(struct file *file,
                   struct address_space *mapping,
                   loff_t pos,
                   unsigned int len,
                   unsigned int copied,
                   struct page *page,
                   void *fsdata)
{
    printk("smfs_write_end Begin");
    struct inode *inode=file->f_inode;
    struct smfs_inode_kernel *smfsInode=SMFS_INODE(inode);
    int ret=generic_write_end(file,mapping,pos,len,copied,page,fsdata);
    if(ret < len) return ret;
    inode->i_blocks=inode->i_size/SMFS_BLOCK_SIZE;
    inode->i_mtime=current_time(inode);
    mark_inode_dirty(inode);
    printk("smfs_write_end return");
    return ret;
}
const struct address_space_operations smfsAops = {
    .readpage=smfs_readpage,
    .writepage=smfs_writepage,
    .write_begin=smfs_write_begin,
    .write_end=smfs_write_end,
};

const struct file_operations smfsFileops = {
    .llseek=generic_file_llseek,
    .read_iter=generic_file_read_iter,
    .fsync=generic_file_fsync,
    .write_iter=generic_file_write_iter,
};