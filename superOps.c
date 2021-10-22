/*
 * @Author: Corvo Attano(fkxzz001@qq.com)
 * @Description: 
 * @LastEditors: Please set LastEditors
 */
#include"common.h"
#include<linux/buffer_head.h>
#include<linux/fs.h>
#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/slab.h>
#include<linux/statfs.h>
static struct kmem_cache *smfs_inode_cache;
/**
 * @description: release the super block in kernel memory
 * @param {super_block} *sb
 * @return {*}
 */
void smfs_put_super(struct super_block *sb)
{
    struct smfs_superBlock *smfsSb=sb->s_fs_info;
    if(smfsSb)
    {
        kfree(smfsSb->pDatabitmap);
        kfree(smfsSb->pInodebitmap);
        kfree(smfsSb);
    }
}
static struct inode *smfs_alloc_inode(struct super_block *sb)
{
    struct smfs_inode_kernel *smfsInode=kmem_cache_alloc(smfs_inode_cache,GFP_KERNEL);
    if(smfsInode==NULL) return NULL;
    
    inode_init_once(&(smfsInode->vfs_inode));
    return &(smfsInode->vfs_inode);
} 
void smfs_destroy_inode(struct inode *inode)
{
    struct smfs_inode_kernel *smfsInode=SMFS_INODE(inode);
    kmem_cache_free(smfs_inode_cache,smfsInode);
}
int smfs_write_inode(struct inode *inode,struct writeback_control *wbc)
{
    struct smfs_inode_kernel *smfsInode=SMFS_INODE(inode);
    struct smfs_inode *diskInode=NULL;
    struct super_block *sb=inode->i_sb;
    struct smfs_superBlock *smfsSb=sb->s_fs_info;
    struct buffer_head *bh=NULL;
    uint32_t index;
    uint32_t blockIndex;
    uint32_t offset;
    index=inode->i_ino;
    if(index > smfsSb->iInodenum) return 0;
    blockIndex=index/SMFS_INODE_PER_BLOCK+1;//and the super block
    offset=index % SMFS_INODE_PER_BLOCK;
    bh=sb_bread(sb,blockIndex);
    if(bh==NULL)
    {
        return -EIO;
    }
    diskInode=((struct smfs_inode*)bh->b_data)+offset;
    diskInode->iMode = inode->i_mode;
    diskInode->iUid = i_uid_read(inode);
    diskInode->iGid = i_gid_read(inode);
    diskInode->iSize = inode->i_size;
    diskInode->iCtime = inode->i_ctime.tv_sec;
    diskInode->iAtime = inode->i_atime.tv_sec;
    diskInode->iMtime = inode->i_mtime.tv_sec;
    diskInode->iBlocknum = inode->i_blocks;
    if(S_ISDIR(inode->i_mode))
    {
        diskInode->pDirblockptr=smfsInode->pDirblockptr;
    }
    else if(S_ISREG(inode->i_mode))
    {
        memcpy(diskInode->pFlieblockptr,smfsInode->pFlieblockptr,sizeof(diskInode->pFlieblockptr));
    }
    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    brelse(bh);
    return 0;
}
int smfs_sync_fs(struct super_block *sb,int wait)
{
    struct smfs_superBlock *smfsSb=sb->s_fs_info;
    struct smfs_superBlock *diskSb=NULL;
    struct buffer_head *bh=NULL;
    bh=sb_bread(sb,0);
    if(bh==NULL) return -EIO;
    diskSb=(struct smfs_superBlock*)bh->b_data;
    diskSb->iBlocknum=smfsSb->iBlocknum;
    diskSb->iDatabitmapblocks=smfsSb->iDatabitmapblocks;
    diskSb->iDatablocks=smfsSb->iDatablocks;
    diskSb->iDatafree=smfsSb->iDatafree;
    diskSb->iInodebitmapblocks=smfsSb->iInodebitmapblocks;
    diskSb->iInodefree=smfsSb->iInodefree;
    diskSb->iInodenum=smfsSb->iInodenum;
    diskSb->iInodestoreblocks=smfsSb->iInodestoreblocks;
    mark_buffer_dirty(bh);
    if(wait) sync_dirty_buffer(bh);
    brelse(bh);
    int i;
    for(i=0;i<smfsSb->iInodebitmapblocks;i++)
    {
        bh=sb_bread(sb,1+smfsSb->iInodestoreblocks+i);
        if(bh==NULL) return -EIO;
        memcpy(bh->b_data,(void*)smfsSb->pInodebitmap+i*SMFS_BLOCK_SIZE,SMFS_BLOCK_SIZE);
        mark_buffer_dirty(bh);
        if(wait) sync_dirty_buffer(bh);
        brelse(bh);
    }
    for(i=0;i<smfsSb->iDatabitmapblocks;i++)
    {
        bh=sb_bread(sb,1+smfsSb->iInodestoreblocks+smfsSb->iInodebitmapblocks);
        if(bh==NULL) return -EIO;
        memcpy(bh->b_data,(void*)smfsSb->pDatabitmap+i*SMFS_BLOCK_SIZE,SMFS_BLOCK_SIZE);
        mark_buffer_dirty(bh);
        if(wait) sync_dirty_buffer(bh);
        brelse(bh);
    }
    return 0;
}
int smfs_statfs(struct dentry *dentry,struct kstatfs *stat)
{
    struct super_block *sb=dentry->d_sb;
    struct smfs_superBlock *smfsSb=sb->s_fs_info;

    stat->f_type = SMFS_MAGIC;
    stat->f_bsize = SMFS_BLOCK_SIZE;
    stat->f_blocks = smfsSb->iBlocknum;
    stat->f_bfree = smfsSb->iDatafree;
    stat->f_bavail = smfsSb->iDatafree;
    stat->f_files = smfsSb->iInodenum-smfsSb->iInodefree;
    stat->f_ffree = smfsSb->iInodefree;
    stat->f_namelen = SMFS_MAX_FILENAME_LEN;
    return 0;
}
static struct super_operations simplefs_super_ops = {
    .put_super = smfs_put_super,
    .alloc_inode = smfs_alloc_inode,
    .destroy_inode = smfs_destroy_inode,
    .write_inode = smfs_write_inode,
    .sync_fs = smfs_sync_fs,
    .statfs = smfs_statfs,
};