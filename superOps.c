/*
 * @Author: Corvo Attano(fkxzz001@qq.com)
 * @Description: 
 * @LastEditors: Corvo Attano(fkxzz001@qq.com)
 */

#include<linux/buffer_head.h>
#include<linux/fs.h>
#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/slab.h>
#include<linux/statfs.h>
#include<linux/printk.h>
#include"common.h"
static struct kmem_cache *smfs_inode_cache;
int smfs_init_inode_cache(void)
{
    smfs_inode_cache = kmem_cache_create("smfs_cache", sizeof(struct smfs_inode_kernel), 0, 0, NULL);
    if (!smfs_inode_cache)
        return -ENOMEM;
    return 0;
}
void smfs_destroy_inode_cache(void)
{
    kmem_cache_destroy(smfs_inode_cache);
}
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
    
    inode_init_once(&smfsInode->vfs_inode);
    return &smfsInode->vfs_inode;
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
    printk("smfs_write_inode index:%u blockIndex:%u offset:%u",index,blockIndex,offset);
    bh=sb_bread(sb,blockIndex);
    if(bh==NULL)
    {
        return -EIO;
    }
    diskInode=(struct smfs_inode *)(bh->b_data);
    diskInode+=offset;
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
    printk("smfs_write_inode return");
    return 0;
}
int smfs_sync_fs(struct super_block *sb,int wait)
{
    struct smfs_superBlock *smfsSb=sb->s_fs_info;
    struct smfs_superBlock *diskSb=NULL;
    struct buffer_head *bh=NULL;
    bh=sb_bread(sb,0);
    if(bh==NULL) 
    {
        printk("smfs_sync_fs error");
        return -EIO;
    }
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
        printk("smfs_sync_fs inode bit map 0:%d",(int)(*(smfsSb->pInodebitmap+i*SMFS_BLOCK_SIZE)));
        memcpy(bh->b_data,smfsSb->pInodebitmap+i*SMFS_BLOCK_SIZE,SMFS_BLOCK_SIZE);
        mark_buffer_dirty(bh);
        if(wait) sync_dirty_buffer(bh);
        brelse(bh);
    }
    for(i=0;i<smfsSb->iDatabitmapblocks;i++)
    {
        bh=sb_bread(sb,1+smfsSb->iInodestoreblocks+smfsSb->iInodebitmapblocks);
        if(bh==NULL) return -EIO;
        printk("smfs_sync_fs data block bit map 0:%d",(int)(*(smfsSb->pDatabitmap+i*SMFS_BLOCK_SIZE)));
        memcpy(bh->b_data,smfsSb->pDatabitmap+i*SMFS_BLOCK_SIZE,SMFS_BLOCK_SIZE);
        mark_buffer_dirty(bh);
        if(wait) sync_dirty_buffer(bh);
        brelse(bh);
    }
    printk("smfs_sync_fs return");
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
static struct super_operations smfs_super_ops = {
    .put_super = smfs_put_super,
    .alloc_inode = smfs_alloc_inode,
    .destroy_inode = smfs_destroy_inode,
    .write_inode = smfs_write_inode,
    .sync_fs = smfs_sync_fs,
    .statfs = smfs_statfs,
};
int smfs_fill_super(struct super_block *sb, void *data, int silent)
{
    struct buffer_head *bh = NULL;
    struct smfs_superBlock *currentSb = NULL;
    struct smfs_superBlock *smfsSb = NULL;
    struct inode *rootInode = NULL;
    int ret = 0, i;

    /* Init sb */
    sb->s_magic = SMFS_MAGIC;
    sb_set_blocksize(sb, SMFS_BLOCK_SIZE);
    sb->s_maxbytes = SMFS_MAX_FILE_SIZE;
    sb->s_op = &smfs_super_ops;
    bh = sb_bread(sb, 0);
    if (!bh)
        return -EIO;
    currentSb = (struct smfs_superBlock *) bh->b_data;
    if (currentSb->iMagic != sb->s_magic) 
    {
        pr_err("Wrong magic number\n");
        ret = -EINVAL;
        goto release;
    }

    /* Alloc sb_info */
    smfsSb = kzalloc(sizeof(struct smfs_superBlock), GFP_KERNEL);
    if (!smfsSb) 
    {
        ret = -ENOMEM;
        goto release;
    }

    smfsSb->iBlocknum = currentSb->iBlocknum;
    smfsSb->iDatabitmapblocks=currentSb->iDatabitmapblocks;
    smfsSb->iDatablocks=currentSb->iDatablocks;
    smfsSb->iDatafree=currentSb->iDatafree;
    smfsSb->iInodebitmapblocks=currentSb->iInodebitmapblocks;
    smfsSb->iInodefree=currentSb->iInodefree;
    smfsSb->iInodenum=currentSb->iInodenum;
    smfsSb->iInodestoreblocks=currentSb->iInodestoreblocks;
    sb->s_fs_info=smfsSb;

    brelse(bh);

    smfsSb->pInodebitmap =kzalloc(smfsSb->iInodebitmapblocks * SMFS_BLOCK_SIZE, GFP_KERNEL);
    if (smfsSb->pInodebitmap==NULL) 
    {
        ret = -ENOMEM;
        goto free_smfssb;
    }
    for (i = 0; i < smfsSb->iInodebitmapblocks; i++) {
        int index = 1 + smfsSb->iInodestoreblocks + i;
        printk("indoe bitmap block index: %u",index);
        bh = sb_bread(sb, index);
        if (!bh) 
        {
            ret = -EIO;
            goto free_ifree;
        }

        memcpy((void *) smfsSb->pInodebitmap + i * SMFS_BLOCK_SIZE, bh->b_data,SMFS_BLOCK_SIZE);

        brelse(bh);
    }

    smfsSb->pDatabitmap =kzalloc(smfsSb->iDatabitmapblocks * SMFS_BLOCK_SIZE, GFP_KERNEL);
    if (smfsSb->pDatabitmap==NULL) 
    {
        ret = -ENOMEM;
        goto free_ifree;
    }

    for (i = 0; i < smfsSb->iDatabitmapblocks; i++) {
        int index = 1+smfsSb->iInodestoreblocks + smfsSb->iInodebitmapblocks + i;
        printk("data bitmap block index: %u",index);
        bh = sb_bread(sb, index);
        if (!bh) 
        {
            ret = -EIO;
            goto free_dfree;
        }

        memcpy((void *) smfsSb->pDatabitmap + i * SMFS_BLOCK_SIZE, bh->b_data,SMFS_BLOCK_SIZE);
        printk("data bitmap char 0: %d",(int)smfsSb->pDatabitmap[0]);
        brelse(bh);
    }

    /* Create root inode */
    rootInode = _getInode(sb, 0);
    if (IS_ERR(rootInode)) {
        ret = PTR_ERR(rootInode);
        goto free_dfree;
    }
    inode_init_owner(rootInode, NULL, rootInode->i_mode);
    sb->s_root = d_make_root(rootInode);
    if (!sb->s_root) {
        ret = -ENOMEM;
        goto iput;
    }

    return 0;

iput:
    iput(rootInode);
free_dfree:
    kfree(smfsSb->pDatabitmap);
free_ifree:
    kfree(smfsSb->pInodebitmap);
free_smfssb:
    kfree(smfsSb);
release:
    brelse(bh);

    return ret;
}