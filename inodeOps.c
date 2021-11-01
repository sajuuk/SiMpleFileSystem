/*
 * @Author:Corvo Attano(391063482@qq.com)
 * @Date: 2021-10-20 10:49:22
 * @LastEditTime: 2021-11-01 10:09:21
 * @LastEditors: Corvo Attano(fkxzz001@qq.com)
 * @Description: inode_operations implement
 * @FilePath: \SiMpleFileSystem\inode.c
 */

//#include<string.h>
#include<linux/fs.h>
#include<linux/kernel.h>
#include<linux/buffer_head.h>
#include<linux/module.h>
#include<linux/printk.h>
#include"bitmap.h"
#include"common.h"
static const struct inode_operations smfsInodeops;
/**
 * @description: get a vfs inode from cache,if have't cached yet, call the page cache to read related block data and update the cache
 * @param {super_block} *sb
 * @param {unsigned long} index
 * @return {*}
 */
struct inode *_getInode(struct super_block *sb,unsigned long index)
{
    struct inode *inode=NULL;
    struct smfs_inode *smfsInode=NULL;
    struct smfs_inode_kernel *smfsInodekernel=NULL;
    struct smfs_superBlock *smfsSb=sb->s_fs_info;
    struct buffer_head *bh=NULL;
    uint32_t blockIndex=index/SMFS_INODE_PER_BLOCK+1;
    uint32_t offset=index%SMFS_INODE_PER_BLOCK;
    if(index>smfsSb->iInodenum)//check inode range
    {
        return ERR_PTR(-EINVAL);
    }
    inode=iget_locked(sb,index);
    /*Search for the inode specified by ino in the inode cache and if present return it with an increased reference count. This is for file systems where the inode number is sufficient for unique identification of an inode.
    If the inode is not in cache, allocate a new inode and return it locked, hashed, and with the I_NEW flag set. The file system gets to fill it in before unlocking it via unlock_new_inode.
    */
    if(inode==NULL)
    {
        return ERR_PTR(-ENOMEM);
    }
    if((inode->i_state & I_NEW)==0)//if is in cache
    {
        return inode;
    }
    smfsInodekernel=SMFS_INODE(inode);
    bh=sb_bread(sb,blockIndex);//read the node from disk
    if(bh==NULL)
    {
        brelse(bh);
        iget_failed(inode);
        return ERR_PTR(-EIO);
    }
    smfsInode=((struct smfs_inode*)bh->b_data)+offset;

    //set up the vfs inode
    inode->i_ino = index;
    inode->i_sb = sb;
    inode->i_op = &smfsInodeops;
    inode->i_mode = smfsInode->iMode;
    i_uid_write(inode, smfsInode->iUid);
    i_gid_write(inode, smfsInode->iGid);
    inode->i_size = smfsInode->iSize;
    inode->i_ctime.tv_sec = (time64_t) smfsInode->iCtime;
    inode->i_ctime.tv_nsec = 0;
    inode->i_atime.tv_sec = (time64_t) smfsInode->iAtime;
    inode->i_atime.tv_nsec = 0;
    inode->i_mtime.tv_sec = (time64_t) smfsInode->iMtime;
    inode->i_mtime.tv_nsec = 0;
    inode->i_blocks = smfsInode->iBlocknum;
    if(S_ISDIR(inode->i_mode))//is dir
    {
        smfsInodekernel->pDirblockptr=smfsInode->pDirblockptr;
        inode->i_fop=&smfsDirops;
    }
    else if(S_ISREG(inode->i_mode))// is file
    {
        memcpy(smfsInodekernel->pFlieblockptr,smfsInode->pFlieblockptr,SMFS_INODE_DATA_SEC);
        inode->i_fop=&smfsFileops;
        inode->i_mapping->a_ops=&smfsAops;
    }
    brelse(bh);
    unlock_new_inode(inode);
    return inode;
}
/**
 * @description: create a new inode of file or dir,new dir with a whole data block while new file has no data block. 
 * @param {inode} *dir
 * @param {mode_t} mode
 * @return {*}
 */
struct inode* _newInode(struct inode *dir,mode_t mode)
{
    struct super_block *sb=NULL;
    struct smfs_superBlock *smfsSb=NULL;
    struct inode *inode=NULL;
    struct smfs_inode_kernel *smfsInodekernel=NULL;
    uint32_t inodeIndex,blockIndex;
    if(!(S_ISDIR(mode) || S_ISREG(mode)))//SMFS only support dir and file
    {
        return ERR_PTR(-EINVAL);
    }
    sb=dir->i_sb;
    smfsSb=sb->s_fs_info;
    if(smfsSb->iDatafree==0|| smfsSb->iInodefree==0)//check if there has enough space
    {
        return ERR_PTR(-ENOSPC);
    }
    inodeIndex=getFreeinode(smfsSb);
    printk("_newInode inodeIndex %u",inodeIndex);
    if(inodeIndex==0)
    {
        return ERR_PTR(-ENOSPC);
    }
    inode=_getInode(sb,inodeIndex);//create a new inode in memory
    if(IS_ERR(inode))
    {
        printk("_newInode inode err");
        releaseInode(smfsSb,inodeIndex);//release the bitmap
        return  ERR_PTR(inode);
    }
    
    smfsInodekernel=SMFS_INODE(inode);
    inode_init_owner(inode,dir,mode);
    if(S_ISDIR(mode))
    {
        blockIndex=getFreedata(smfsSb);
        printk("_newInode blockIndex: %u",blockIndex);
        if(blockIndex==0)
        {
            iput(inode);
            releaseInode(smfsSb,inodeIndex);
            return ERR_PTR(-ENOSPC);
        }
        inode->i_blocks=1;
        inode->i_size=SMFS_BLOCK_SIZE;
        inode->i_fop=&smfsDirops;
        //set_nlink(inode, 2); not sure is it necessary
        smfsInodekernel->pDirblockptr=1 + smfsSb->iInodestoreblocks + smfsSb->iInodebitmapblocks + smfsSb->iDatabitmapblocks + blockIndex;
    }
    else if(S_ISREG(mode))
    {
        inode->i_blocks=0;
        inode->i_size=0;
        inode->i_fop=&smfsFileops;
        inode->i_mapping->a_ops=&smfsAops;
        memset(smfsInodekernel->pFlieblockptr,0,sizeof(smfsInodekernel->pFlieblockptr));
    }
    inode->i_ctime = inode->i_atime = inode->i_mtime = current_time(inode);
    printk("_newInode return");
    return inode;
}
/**
 * @description: lookup the dir or file by name, if found append to dentry
 * @param {inode} *
 * @param {dentry} *
 * @param {unsigned} int
 * @return {*}
 */
struct dentry *smfs_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags)
{
    struct super_block *sb=dir->i_sb;
    struct smfs_inode_kernel *smfs_dir=SMFS_INODE(dir);
    struct buffer_head *bh = NULL;
    struct smfs_dirBlock *dblock=NULL;
    struct smfs_dirMem *dmem=NULL;
    struct inode *inode=NULL;
    int found=0;
    if(dentry->d_name.len > SMFS_MAX_FILENAME_LEN)//check the file name
    {
        return ERR_PTR(-ENAMETOOLONG);
    }
    bh=sb_bread(sb,smfs_dir->pDirblockptr);//read the data block from page
    if(!bh)
    {
        return ERR_PTR(-EIO);
    }
    dblock=(struct smfs_dirBlock*)bh->b_data;
    int i;
    for(i=0;i<SMFS_MAX_DIR_MEM;i++)//find the file/dir
    {
        dmem=&(dblock->fileList[i]);
        if(dmem->iInode==0)
        {
            break;
        }
        if(strncmp(dmem->fileName,dentry->d_name.name,SMFS_MAX_FILENAME_LEN)==0)
        {
            inode=_getInode(sb,dmem->iInode);
            found=1;
            break;
        }
    }
    brelse(bh);
    dir->i_atime = current_time(dir);
    mark_inode_dirty(dir);
    if(found)
    {
        d_add(dentry,inode);
    }
    else
    {
        d_add(dentry,NULL);
    }
    return NULL;
}
/**
 * @description: create dir or file, if dir, clean the data block (files dont need to)
 * @param {inode} *dir
 * @param {dentry} *dentry
 * @param {umode_t} mode
 * @param {bool} excl
 * @return {*}
 */
int smfs_create(struct inode *dir,struct dentry *dentry,umode_t mode,bool excl)
{
    struct super_block *sb;
    struct inode *inode;
    struct smfs_inode_kernel *smfsDir;
    struct smfs_inode_kernel *smfsInode;
    struct smfs_dirBlock *parentDirdata;
    struct buffer_head *bhParent;
    struct buffer_head *bh;
    if(strlen(dentry->d_name.name)>SMFS_MAX_FILENAME_LEN)//check the name length first
    {
        return -ENAMETOOLONG;
    }
    //read parent dir data
    smfsDir=SMFS_INODE(dir);
    sb=dir->i_sb;
    bhParent=sb_bread(sb,smfsDir->pDirblockptr);
    if(bhParent==NULL) return -EIO;
    parentDirdata=(struct smfs_dirBlock *)(bhParent->b_data);
    //check if there have free space
    if(parentDirdata->fileList[SMFS_MAX_DIR_MEM-1].iInode)
    {
        brelse(bhParent);
        return -ENOSPC;
    }
    //create a new inode
    inode=_newInode(dir,mode);
    smfsInode=SMFS_INODE(inode);
    if(IS_ERR(inode))
    {
        brelse(bhParent);
        return PTR_ERR(inode);
    }
    if(S_ISDIR(mode))//if dir, clean the data block of new inode
    {
        bh=sb_bread(sb,smfsInode->pDirblockptr);
        if(bh==NULL)
        {
            struct smfs_superBlock *smfsSb=sb->s_fs_info;
            releaseData(sb->s_fs_info,smfsInode->pDirblockptr - 1 - smfsSb->iInodestoreblocks - smfsSb->iInodebitmapblocks);
            releaseInode(sb->s_fs_info,inode->i_ino);
            iput(inode);
            brelse(bhParent);
            return -EIO;
        }
        memset(bh->b_data,0,SMFS_BLOCK_SIZE);
        mark_buffer_dirty(bh);
        brelse(bh);
    }
    //find the first free space
    int i;
    for(i=0;i<SMFS_MAX_DIR_MEM;i++)
    {
        if(parentDirdata->fileList[i].iInode==0) break;
    }
    parentDirdata->fileList[i].iInode=inode->i_ino;
    strncpy(parentDirdata->fileList[i].fileName,dentry->d_name.name,SMFS_MAX_FILENAME_LEN);
    dir->i_mtime=dir->i_atime=current_time(dir);
    //if(S_ISDIR(mode)) inc_nlink(dir); not sure is it necessary
    mark_buffer_dirty(bhParent);
    mark_inode_dirty(inode);
    mark_inode_dirty(dir);
    brelse(bhParent);
    d_instantiate(dentry,inode);//fill in inode information in the entry.
    printk("smfs_create return");
    return 0;
}
int smfs_mkdir(struct inode *dir,struct dentry *dentry,umode_t mode)
{
    return smfs_create(dir,dentry,mode|S_IFDIR,0);
}
/**
 * @description: remove dir and sort the file list in data block
 * @param {inode} *dir -parent dir
 * @param {dentry} *dentry
 * @return {*}
 */
int smfs_rmdir(struct inode *dir,struct dentry *dentry)
{
    struct super_block *sb=dir->i_sb;
    struct smfs_superBlock *smfsSb=sb->s_fs_info;
    struct inode *inode=d_inode(dentry);
    struct smfs_inode_kernel *parentDirinode=SMFS_INODE(dir);
    struct smfs_inode_kernel *smfsInode=SMFS_INODE(inode);
    struct buffer_head *bh=sb_bread(sb,smfsInode->pDirblockptr);
    struct buffer_head *parentBh=NULL;
    struct smfs_dirBlock *smfsDirblock=NULL;
    struct smfs_dirBlock *smfsParentdirblk=NULL;

    uint32_t inodeIndex=inode->i_ino;
    printk("smfs_rmdir inode ino: %u",inodeIndex);
    int i;
    if(bh==NULL)
    {
        return -EIO;
    }
    smfsDirblock=(struct smfs_dirBlock *)bh->b_data;
    if(smfsDirblock->fileList[0].iInode)//check if the dir is empty
    {
        brelse(bh);
        return -ENOTEMPTY;
    }
    brelse(bh);
    parentBh=sb_bread(sb,parentDirinode->pDirblockptr);
    if(parentBh==NULL)
    {
        return -EIO;
    }
    smfsParentdirblk=(struct smfs_dirBlock *)parentBh->b_data;
    unsigned char *dirName=dentry->d_name.name;
    for(i=0;i<SMFS_MAX_DIR_MEM;i++)
    {
        if(strncmp(dirName,smfsParentdirblk->fileList[i].fileName,SMFS_MAX_FILENAME_LEN)==0) 
        {
            //inodeIndex=smfsParentdirblk->fileList[i].iInode;
            printk("smfs_rmdir inode ino in parent dir: %u",smfsParentdirblk->fileList[i].iInode);
            break;
        }
    }
    int j;
    for(j=i;j<SMFS_MAX_DIR_MEM-1;j++)//delete the dir and rerange
    {
        memcpy(smfsParentdirblk->fileList + j,smfsParentdirblk->fileList + j + 1,sizeof(struct smfs_dirMem));
    }
    memset(smfsParentdirblk->fileList + SMFS_MAX_DIR_MEM - 1,0,sizeof(struct smfs_dirMem));
    mark_buffer_dirty(parentBh);
    brelse(parentBh);
    dir->i_mtime=dir->i_atime=current_time(dir);
    mark_inode_dirty(dir);
    mark_inode_dirty(inode);
    printk("snfs_rmdir block ptr %u",smfsInode->pDirblockptr);
    releaseData(smfsSb,smfsInode->pDirblockptr - 1 - smfsSb->iInodestoreblocks - smfsSb->iInodebitmapblocks - smfsSb->iDatabitmapblocks);//release the bit map
    releaseInode(smfsSb,inodeIndex);
    printk("smfs_rmdir return");
    return 0;
}
static const struct inode_operations smfsInodeops = {
    .lookup = smfs_lookup,
    .create = smfs_create,
    .mkdir = smfs_mkdir,
    .rmdir = smfs_rmdir,
    //.rename = ,
};