/*
 * @Author: Corvo Attano(fkxzz001@qq.com)
 * @Description: common for project
 * @LastEditors: Corvo Attano(fkxzz001@qq.com)
 */
#ifndef SMFS_COMMON_H
#define SMFS_COMMON_H
#include<stdint.h>
#include<stdio.h>


#define SMFS_BLOCK_SIZE (1<<12)
#define SMFS_INODE_SIZE 128
#define SMFS_INODE_DATA_SEC 24
//the size of inode's data section, 
//data section store the list of pointers of file data block 
//or the pointer of directory block
#define SMFS_MAX_FILE_SIZE (24*SMFS_BLOCK_SIZE)//at most 96KiB
#define SMFS_INODE_PER_BLOCK (SMFS_BLOCK_SIZE/SMFS_INODE_SIZE)

#define SMFS_MAX_FILENAME_LEN 28
#define SMFS_MAX_DIR_MEM 128
#define SMFS_DATA_BLOCK_SIZE 4096
#define SMFS_MAGIC 67176176

struct smfs_inode
{
    uint32_t iMode;
    uint32_t iUid;//owner id
    uint32_t iGid;//group id
    uint32_t iSize;//size
    uint32_t iCtime;//change time
    uint32_t iAtime;//access time
    uint32_t iMtime;//modify time
    uint32_t iBlocknum;//number of blocks
    union
    {
        uint32_t pFlieblockptr[SMFS_INODE_DATA_SEC];
        uint32_t pDirblockptr;
    };
};
#ifdef __KERNEL__
    //in memory
struct smfs_inode_kernel
{
    union
    {
        uint32_t pFlieblockptr[SMFS_INODE_DATA_SEC];
        uint32_t pDirblockptr;
    };
    struct inode vfs_inode;
}
#endif

struct smfs_superBlock
{
    uint32_t iMagic;
    uint32_t iBlocknum;//number of blocks in disk
    uint32_t iInodenum;//number of inodes in disk
    uint32_t iDatablocks;//number of blocks to store data
    uint32_t iInodestoreblocks;//number of blocks to store inodes
    uint32_t iInodebitmapblocks;//number of blocks to store inode bitmap
    uint32_t iDatabitmapblocks;//number of blocks to store data bitmap
    uint32_t iInodefree;//number of free inodes
    uint32_t iDatafree;//number of free data blocks
#ifdef __KERNEL__
    //in memory
    unsigned long *pInodebitmap;
    unsigned long *pDatabitmap;
#endif
};
#ifdef __KERNEL__
struct smfs_dirMem
{
    uint32_t iInode;
    char fileName[SMFS_MAX_FILENAME_LEN];
};
struct smfs_dirBlock
{
    struct smfs_dirMem fileList[SMFS_MAX_DIR_MEM];
};
struct smfs_dataBlock
{
    unsigned char [SMFS_DATA_BLOCK_SIZE];
};
// file functions
extern const struct file_operations smfsFileops;
extern const struct file_operations smfsDirops;
extern const struct address_space_operations smfsAops;
#define SMFS_INODE(inode) (container_of(inode, struct smfs_inode_kernel, vfs_inode))
#endif
#endif