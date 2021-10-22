/*
 * @Author: Corvo Attano(391063482@qq.com)
 * @Date: 2021-10-21 15:21:10
 * @LastEditTime: 2021-10-21 23:01:06
 * @LastEditors: Please set LastEditors
 * @Description: bitmap functions
 * @FilePath: \SiMpleFileSystem\bitmap.h
 */
#ifndef BITMAP_H
#define BITMAP_H
#include"common.h"
#include<math.h>
#include<string.h>
#include<linux/fs.h>
#include<linux/kernel.h>
#include<linux/buffer_head.h>
#include<linux/module.h>
/**
 * @description: 
 * @param {unsigned char*} bitMap -the pointer of bitmap
 * @param {uint32_t} index -index of BIT(not byte)
 * @param {int} value -0 or 1
 * @return {*}
 */
void _setBitmap(unsigned char* bitMap,uint32_t index,int value)
{
    uint32_t byteIndex=index/8;
    uint32_t bitOffset=index%8;
    unsigned char* targetByte=bitMap+byteIndex;
    unsigned char temp;
    if(value)//set to 1
    {
        temp=1<<bitOffset;
        (*targetByte)=(*targetByte) | temp;
    }
    else//set to 0
    {
        temp=254;
        int i;
        for(i=0;i<bitOffset;i++)
        {
            temp=(temp<<1)+1;
        }
        (*targetByte)=(*targetByte) & temp;
    }
}
/**
 * @description: find first free bit in bit map
 * @param {unsigned char*} -pointer of bitMap
 * @param {uint32_t} maxNum -max number of inodes
 * @return {uint32_t} return free bit index, if no free bit return 0
 */
uint32_t _findFirstfree(unsigned char* bitMap,uint32_t maxNum)
{
    int i,j;
    unsigned char temp,mask;
    int maxByte=ceil((double)maxNum/8.0);
    for(i=0;i<maxByte;i++)
    {
        temp=bitMap[i];
        if(bitMap!=0)//have free bits
        {
            j=0;
            mask=1;
            while((mask & temp)==0) maks<<1;
            return i*8+j;
        }
    }
    return 0;//no free bit
}
/**
 * @description: find a free inode and set its bit map to used 
 * @param {smfs_superBlock*} smfsSb
 * @return {uint32_t} inode index, 0 if no free inode
 */
uint32_t getFreeinode(struct smfs_superBlock* smfsSb)
{
    uint32_t found = _findFirstfree(smfsSb->pInodebitmap,smfsSb->iInodenum);
    if(found)
    {
        _setBitmap(smfsSb->pInodebitmap,found,0);//set inode bit used
        return found;
    }
    return 0;
}
/**
 * @description: set the inode bit map to free
 * @param {smfs_superBlock*} smfsSb
 * @param {uint32_t} index -the index of released inode
 * @return {*} 0 if success
 */
uint32_t releaseInode(struct smfs_superBlock* smfsSb,uint32_t index)
{
    if (index>smfsSb->iInodenum) return 1;
    _setBitmap(smfsSb->pInodebitmap,index,1);
    return 0;
}
/**
 * @description: find a free data block and set its bit map to used
 * @param {smfs_superBlock*} smfsSb
 * @return {*} data block index, 0 if no free data block
 */
uint32_t getFreedata(struct smfs_superBlock* smfsSb)
{
    uint32_t found = _findFirstfree(smfsSb->pDatabitmap,smfsSb->iDatablocks);
    if(found)
    {
        _setBitmap(ssmfsSb->pDatabitmap,found,0);//set inode bit used
        return found;
    }
    return 0;
}
uint32_t releaseData(struct smfs_superBlock* smfsSb,uint32_t index)
{
    if(index > smfsSb->iDatablocks) return 1;
    _setBitmap(smfsSb->pDatabitmap,index,1);
    return 0;
}
#endif