/*
 * @Author: Corvo Attano(fkxzz001@qq.com)
 * @Description: make file system
 * @LastEditors: Corvo Attano(fkxzz001@qq.com)
 */
#include"common.h"
#include<linux/fs.h>
#include<linux/kernel.h>
#include<sys/stat.h>
#include<unistd.h>
#include<math.h>
#include<sys/stat.h>
#include<sys/ioctl.h>
void showHelp(char *name)
{
    printf("usage: %s disk.img\n",name);
}
void allocSuperblock(int fd,struct smfs_superBlock *sb,long int diskSize)
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
    sb->iDatafree=blockNum-1-inodeBlocknum-bitmapBlocknum-bitmapBlocknum;
    int ret=write(fd,sb,sizeof(struct smfs_superBlock));
    if (ret !=sizeof(struct smfs_superBlock))
    {
        
    }
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
        return 1;
    }
    allocSuperblock(fd,sb,diskSize);
    return 0;
}

