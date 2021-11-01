/*
 * @Author: Corvo Attano(fkxzz001@qq.com)
 * @Description: 
 * @LastEditors: Corvo Attano(fkxzz001@qq.com)
 */

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/buffer_head.h>
#include <linux/module.h>
#include"common.h"

static int smfs_iter(struct file *dir,struct dir_context *ctx)
{
    struct inode *inode = file_inode(dir);
    struct super_block *sb=inode->i_sb;
    struct buffer_head *bh=NULL;
    struct smfs_inode_kernel *smfsInode=NULL;
    struct smfs_dirBlock *smfsDirblk=NULL;
    int i;
    smfsInode = SMFS_INODE(inode);
    if(!S_ISDIR(inode->i_mode))
    {
        return -ENOTDIR;
    }
    if (ctx->pos > SMFS_MAX_DIR_MEM + 2)//include . and ..
    {
        return 0;
    }
    if(!dir_emit_dots(dir,ctx)) return 0;
    bh=sb_bread(sb,smfsInode->pDirblockptr);
    if(bh==NULL) return -EIO;
    smfsDirblk=(struct smfs_dirBlock *)(bh->b_data);
    for(i=ctx->pos-2;i<SMFS_MAX_DIR_MEM;i++)
    {
        if(smfsDirblk->fileList[i].iInode==0) break;
        if(!dir_emit(ctx,smfsDirblk->fileList[i].fileName,SMFS_MAX_FILENAME_LEN,smfsDirblk->fileList[i].iInode,DT_UNKNOWN)) break;
        ctx->pos++;
    }
    brelse(bh);
    return 0;
}
const struct file_operations smfsDirops = {
    .owner = THIS_MODULE,
    .iterate_shared = smfs_iter
};