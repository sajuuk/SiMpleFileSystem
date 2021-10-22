/*
 * @Author: Corvo Attano(fkxzz001@qq.com)
 * @Description: make file system
 * @LastEditors: Please set LastEditors
 */
#include"common.h"
#include<linux/fs.h>
#include<linux/kernel.h>
#include<sys/stat.h>
#include<unistd.h>
#include<math.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/ioctl.h>
void showHelp(char *name)
{
    printf("usage: %s disk.img\n",name);
}
/**
 * @description: alloc the super block
 * @param {int} fd -the disk image file
 * @param {smfs_superBlock} *sb -pointer of super block info container
 * @param {long int} diskSize -the size of disk image
 * @return {*}
 */
int allocSuperblock(int fd,struct smfs_superBlock *sb,long int diskSize)
{
    if(sb==NULL) return;
    const uint32_t k=(1<<15);//constant k,see README

    uint32_t blockNum=diskSize/SMFS_BLOCK_SIZE;
    double t=(double)(blockNum-1)*k/(64+33*k);
    uint32_t inodeBlocknum=(uint32_t)floor(t);
    double t=(double)inodeBlocknum*32/k;
    uint32_t bitmapBlocknum=(uint32_t)ceil(t);
    sb->iMagic=SMFS_MAGIC;
    sb->iBlocknum=blockNum;
    sb->iInodenum=inodeBlocknum*SMFS_INODE_PER_BLOCK;
    sb->iInodestoreblocks=inodeBlocknum;
    sb->iInodebitmapblocks=bitmapBlocknum;
    sb->iDatabitmapblocks=bitmapBlocknum;
    sb->iInodefree=sb->iInodenum-1;//inode 0 is reserved
    sb->iDatablocks = sb->iDatafree = blockNum-1-inodeBlocknum-bitmapBlocknum-bitmapBlocknum-1;//data block 0 reserved
    int ret=write(fd,sb,sizeof(struct smfs_superBlock));
    if (ret !=sizeof(struct smfs_superBlock))
    {
        perror("write super block info error");
        return 1;
    }
    unsigned char *zeros;
    int zeroSize=sizeof(char)*SMFS_BLOCK_SIZE-sizeof(struct smfs_superBlock);
    zeros=(unsigned char*)malloc(zeroSize);//alloc the rest of the block in padding
    ret=write(fd,zeros,zeroSize);
    if (ret != zeroSize)
    {
        perror("write super block payload error");
        free(zeros);
        return 1;
    }
    free(zeros);
    return 0;
}
/**
 * @description: alloc all inode blocks in disk, the fist inode(inode 0) is reserved
 * @param {int} fd -the disk image file 
 * @param {int} inodeBlockcnt -the count of inode blocks
 * @return {int} -0 if success
 */
int allocInodeblock(int fd,int inodeBlockcnt)
{
    struct smfs_inode inodeZero;
    inodeZero.iAtime=inodeZero.iCtime=inodeZero.iMtime=0;
    inodeZero.iBlocknum=1;
    inodeZero.iGid=inodeZero.iUid=0;
    inodeZero.iSize=SMFS_BLOCK_SIZE;
    inodeZero.iMode=S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IXUSR | S_IXGRP | S_IXOTH;
    memset(inodeZero.pFlieblockptr,0,sizeof(inodeZero.pFlieblockptr));//the fist data block is reserved too
    int ret=write(fd,inodeZero,sizeof(struct smfs_inode));
    if(ret != sizeof(struct smfs_inode))
    {
        perror("write inode 0 error");
        return 1;
    }
    unsigned char *zeros;
    int zeroSize=sizeof(char)*SMFS_BLOCK_SIZE-sizeof(struct smfs_inode);
    zeros=(unsigned char*)malloc(zeroSize);//alloc the rest of the block in padding
    ret=write(fd,zeros,zeroSize);
    if (ret != zeroSize)
    {
        perror("write inode 0 payload error");
        free(zeros);
        return 1;
    }
    free(zeros);
    unsigned char *padding;
    padding=(unsigned char*)malloc(SMFS_BLOCK_SIZE);
    for(int i=1;i<inodeBlockcnt;i++)
    {
        ret=write(fd,padding,SMFS_BLOCK_SIZE);
        if(ret != SMFS_BLOCK_SIZE)
        {
            perror("write inodes padding error");
            free(padding);
            return 1;
        }
    }
    free(padding);
    return 0;
}
/**
 * @description: alloc inode bitmap and data block bitmap, return 0 if success
 * @param {int} fd -disk image
 * @param {smfs_superBlock} *sb -super block pointer
 * @return {*}
 */
int allocBitmap(int fd,struct smfs_superBlock *sb)
{
    uint32_t bitMapblocknum = sb->iInodebitmapblocks;
    unsigned char* inodeBitmap=(unsigned char*)malloc(bitMapblocknum*SMFS_BLOCK_SIZE);//for inode bit map
    memset(inodeBitmap,255,sizeof(inodeBitmap));
    inodeBitmap[0]=254;//inode 0 reserved
    long offset=1+sb->iInodestoreblocks;
    lseek(fd,offset,0);
    int ret=write(fd,inodeBitmap,sizeof(inodeBitmap));
    if (ret != sizeof(inodeBitmap))
    {
        perror("write inode bitmap error");
        free(inodeBitmap);
        return 1;
    }
    free(inodeBitmap);
    bitMapblocknum=sb->iDatabitmapblocks;
    unsigned char* dataBitmap=(unsigned char*)malloc(bitMapblocknum*SMFS_BLOCK_SIZE);//for data bit map
    memset(dataBitmap,255,sizeof(dataBitmap));
    dataBitmap[0]=254;//data block 0 reserved
    ret=write(fd,dataBitmap,sizeof(dataBitmap));
    if(ret != sizeof(dataBitmap))
    {
        perror("write data block bitmap error");
        free(dataBitmap);
        return 1;
    }
    return 0;

}
int main(int argc, char **argv)
{
    long int minSize=SMFS_BLOCK_SIZE*10;
    if(argc !=2)
    {
        perror("error");
        showHelp(argv[0]);
        return 1;
    }
    int fd=open(argv[1],O_RDWR);
    if(fd==-1)
    {
        perror("cant open image");
        return 1;
    }
    struct stat statBuf;
    if(fstat(fd,&statBuf))
    {
        perror("cant get disk file stat");
        close(fd);
        return 1;
    }
    if ((statBuf.st_mode & S_IFMT) == S_IFBLK) {
        long int diskSize = 0;
        if (ioctl(fd, BLKGETSIZE64, &diskSize) != 0) {
            perror("cant get disk size");
            close(fd);
            return 1;
        }
    }
    if(diskSize<minSize)
    {
        perror("disk size is too small");
        close(fd);
        return 1;
    }
    struct smfs_superBlock *sb = (struct smfs_superBlock*)malloc(sizeof(struct smfs_superBlock));
    if(sb==NULL)
    {
        perror("memory alloc error");
        close(fd);
        free(sb);
        return 1;
    }
    if(allocSuperblock(fd,sb,diskSize))
    {
        close(fd);
        free(sb);
        return 1;
    }
    if(allocInodeblock(fd,sb->iInodestoreblocks))
    {
        close(fd);
        free(sb);
        return 1;
    }
    if(allocBitmap(fd,sb))
    {
        close(fd);
        free(sb);
        return 1;
    }
    return 0;
}

