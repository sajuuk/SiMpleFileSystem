/*
 * @Author: Corvo Attano(fkxzz001@qq.com)
 * @Description: make file system
 * @LastEditors: Corvo Attano(fkxzz001@qq.com)
 */

#include<fcntl.h>
#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>
#include<linux/fs.h>
#include<linux/kernel.h>
#include<sys/stat.h>
#include<unistd.h>
#include<math.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/ioctl.h>

#include"common.h"
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
int allocSuperblock(FILE* fp,struct smfs_superBlock *sb,long int diskSize)
{
    if(sb==NULL) return 1;
    const uint32_t k=(1<<15);//constant k,see README

    uint32_t blockNum=diskSize/SMFS_BLOCK_SIZE;
    double t=(double)(blockNum-1)*k/(64+33*k);
    uint32_t inodeBlocknum=(uint32_t)floor(t);
    t=(double)inodeBlocknum*32/k;
    uint32_t bitmapBlocknum=(uint32_t)ceil(t);
    sb->iMagic=SMFS_MAGIC;
    sb->iBlocknum=blockNum;
    sb->iInodenum=inodeBlocknum*SMFS_INODE_PER_BLOCK;
    sb->iInodestoreblocks=inodeBlocknum;
    sb->iInodebitmapblocks=bitmapBlocknum;
    sb->iDatabitmapblocks=bitmapBlocknum;
    sb->iInodefree=sb->iInodenum-1;//inode 0 is reserved
    sb->iDatablocks = sb->iDatafree = blockNum-1-inodeBlocknum-bitmapBlocknum-bitmapBlocknum-1;//data block 0 reserved
    //int ret=write(fd,sb,sizeof(struct smfs_superBlock));
    int ret=fwrite(sb,sizeof(struct smfs_superBlock),1,fp);
    if (ret != 1)
    {
        perror("write super block info error");
        return 1;
    }
    unsigned char *zeros;
    int zeroSize=sizeof(char)*SMFS_BLOCK_SIZE-sizeof(struct smfs_superBlock);
    zeros=(unsigned char*)malloc(zeroSize);//alloc the rest of the block in padding
    memset(zeros,0,zeroSize);
    //ret=write(fd,zeros,zeroSize);
    ret=fwrite(zeros,sizeof(unsigned char),zeroSize,fp);
    if (ret != zeroSize)
    {
        perror("write super block payload error");
        free(zeros);
        return 1;
    }
    free(zeros);
    fprintf(stderr,"Blocknum:%u\niInodenum:%u\niInodestoreblocks:%u\niInodebitmapblocks:%u\niDatabitmapblocks:%u\niDatablocks:%u\n",
            blockNum,sb->iInodenum,sb->iInodestoreblocks,sb->iInodebitmapblocks,sb->iDatabitmapblocks,sb->iDatablocks);
    return 0;
}
/**
 * @description: alloc all inode blocks in disk, the fist inode(inode 0) is reserved
 * @param {int} fd -the disk image file 
 * @param {int} inodeBlockcnt -the count of inode blocks
 * @return {int} -0 if success
 */
int allocInodeblock(FILE *fp,int inodeBlockcnt)
{
    struct smfs_inode inodeZero;
    inodeZero.iAtime=inodeZero.iCtime=inodeZero.iMtime=0;
    inodeZero.iBlocknum=1;
    inodeZero.iGid=inodeZero.iUid=0;
    inodeZero.iSize=SMFS_BLOCK_SIZE;
    inodeZero.iMode=S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IXUSR | S_IXGRP | S_IXOTH;
    memset(inodeZero.pFlieblockptr,0,sizeof(uint32_t)*SMFS_INODE_DATA_SEC);//the fist data block is reserved too
    //int ret=write(fd,&inodeZero,sizeof(struct smfs_inode));
    int ret=fwrite(&inodeZero,sizeof(struct smfs_inode),1,fp);
    if(ret != 1)
    {
        perror("write inode 0 error");
        return 1;
    }
    unsigned char *zeros;
    int zeroSize=sizeof(char)*SMFS_BLOCK_SIZE-sizeof(struct smfs_inode);
    zeros=(unsigned char*)malloc(zeroSize);//alloc the rest of the block in padding
    //ret=write(fd,zeros,zeroSize);
    ret=fwrite(zeros,sizeof(char),zeroSize,fp);
    if (ret != zeroSize)
    {
        perror("write inode 0 payload error");
        free(zeros);
        return 1;
    }
    free(zeros);
    unsigned char *padding;
    padding=(unsigned char*)malloc(SMFS_BLOCK_SIZE);
    memset(padding,0,SMFS_BLOCK_SIZE);
    int i;
    for(i=1;i<inodeBlockcnt;i++)
    {
        //ret=write(fd,padding,SMFS_BLOCK_SIZE);
        ret=fwrite(padding,sizeof(char),SMFS_BLOCK_SIZE,fp);
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
int allocBitmap(FILE *fp,struct smfs_superBlock *sb)
{
    uint32_t bitMapblocknum = sb->iInodebitmapblocks;
    unsigned char* inodeBitmap=(unsigned char*)malloc(bitMapblocknum*SMFS_BLOCK_SIZE);//for inode bit map
    int i;
    // for(i=0;i<bitMapblocknum*SMFS_BLOCK_SIZE;i++)
    // {
    //     inodeBitmap[i]=255;
    // }
    memset(inodeBitmap,255,bitMapblocknum*SMFS_BLOCK_SIZE);
    inodeBitmap[0]=254;//inode 0 reserved
    //long offset=1+sb->iInodestoreblocks;
    //lseek(fd,offset,0);
    //int ret=write(fd,inodeBitmap,bitMapblocknum*SMFS_BLOCK_SIZE);
    int ret=fwrite(inodeBitmap,sizeof(char),bitMapblocknum*SMFS_BLOCK_SIZE,fp);
    if (ret != bitMapblocknum*SMFS_BLOCK_SIZE)
    {
        perror("write inode bitmap error");
        free(inodeBitmap);
        return 1;
    }
    free(inodeBitmap);
    bitMapblocknum=sb->iDatabitmapblocks;
    unsigned char* dataBitmap=(unsigned char*)malloc(bitMapblocknum*SMFS_BLOCK_SIZE);//for data bit map
    memset(dataBitmap,255,bitMapblocknum*SMFS_BLOCK_SIZE);
    fprintf(stderr,"bitMapblocknum : %u\n",bitMapblocknum);
    // for(i=0;i<bitMapblocknum*SMFS_BLOCK_SIZE;i++)
    // {
    //     inodeBitmap[i]=255;
    // }
    dataBitmap[0]=254;//data block 0 reserved
    //ret=write(fd,dataBitmap,bitMapblocknum*SMFS_BLOCK_SIZE);
    ret=fwrite(dataBitmap,sizeof(char),bitMapblocknum*SMFS_BLOCK_SIZE,fp);
    if(ret != bitMapblocknum*SMFS_BLOCK_SIZE)
    {
        perror("write data block bitmap error");
        free(dataBitmap);
        return 1;
    }
    free(dataBitmap);
    return 0;

}
int allocBlockzero(FILE *fp)
{
    unsigned char* blockzero=(unsigned char*)malloc(SMFS_BLOCK_SIZE);
    memset(blockzero,0,SMFS_BLOCK_SIZE);
    int ret=fwrite(blockzero,sizeof(char),SMFS_BLOCK_SIZE,fp);
    if(ret != SMFS_BLOCK_SIZE)
    {
        perror("write data block zero error");
        free(blockzero);
        return 1;
    }
    free(blockzero);
    return 0;
}
int main(int argc, char **argv)
{
    long int minSize=SMFS_BLOCK_SIZE*10;
    long int diskSize = 0;
    if(argc !=2)
    {
        perror("error");
        showHelp(argv[0]);
        return 1;
    }
    FILE* fp=fopen(argv[1],"r+");
    int fd=fp->_fileno;
    //int fd=open(argv[1],O_RDWR);
    if(fp==NULL)
    {
        perror("cant open image");
        return 1;
    }
    struct stat statBuf;
    if(fstat(fd,&statBuf))
    {
        perror("cant get disk file stat");
        fclose(fp);
        //close(fd);
        return 1;
    }
    if ((statBuf.st_mode & S_IFMT) == S_IFBLK) {
        if (ioctl(fd, BLKGETSIZE64, &diskSize) != 0) {
            perror("cant get disk size");
            //close(fd);
            fclose(fp);
            return 1;
        }
        statBuf.st_size=diskSize;
    }
    if(statBuf.st_size<minSize)
    {
        fprintf(stderr,"disk size is too small (size=%ld , min=%ld)\n",statBuf.st_size,minSize);
        //close(fd);
        fclose(fp);
        return 1;
    }
    struct smfs_superBlock *sb = (struct smfs_superBlock*)malloc(sizeof(struct smfs_superBlock));
    if(sb==NULL)
    {
        perror("memory alloc error");
        //close(fd);
        fclose(fp);
        free(sb);
        return 1;
    }
    if(allocSuperblock(fp,sb,statBuf.st_size))
    {
        //close(fd);
        fclose(fp);
        free(sb);
        return 1;
    }
    if(allocInodeblock(fp,sb->iInodestoreblocks))
    {
        //close(fd);
        fclose(fp);
        free(sb);
        return 1;
    }
    if(allocBitmap(fp,sb))
    {
        //close(fd);
        fclose(fp);
        free(sb);
        return 1;
    }
    if(allocBlockzero(fp))
    {
        fclose(fp);
        free(sb);
        return 1;
    }
    //close(fd);
    fclose(fp);
    free(sb);
    return 0;
}

