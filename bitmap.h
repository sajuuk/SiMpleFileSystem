/*
 * @Author: Corvo Attano(391063482@qq.com)
 * @Date: 2021-10-21 15:21:10
 * @LastEditTime: 2021-10-28 09:43:28
 * @LastEditors: Corvo Attano(fkxzz001@qq.com)
 * @Description: bitmap functions
 * @FilePath: \SiMpleFileSystem\bitmap.h
 */
#ifndef BITMAP_H
#define BITMAP_H
//#include<string.h>
#include<linux/fs.h>
#include<linux/kernel.h>
#include<linux/buffer_head.h>
#include<linux/module.h>
#include<linux/printk.h>
#include"common.h"
static inline uint32_t ceil(uint32_t a, uint32_t b)
{
    uint32_t ret = a / b;
    if (a % b)
        return ret + 1;
    return ret;
}
/**
 * @description: 
 * @param {unsigned char*} bitMap -the pointer of bitmap
 * @param {uint32_t} index -index of BIT(not byte)
 * @param {int} value -0 or 1
 * @return {*}
 */
static void _setBitmap(unsigned char* bitMap,uint32_t index,int value)
{
    uint32_t byteIndex=index/8;
    uint32_t bitOffset=index%8;
    unsigned char* targetByte=bitMap+byteIndex;
    unsigned char temp;
    printk("_setBitmap target Byte before: %d",(int)(*targetByte));
    if(value)//set to 1
    {
        printk("_setBitmap set to 1 bitOffset: %u",bitOffset);
        temp=1<<bitOffset;
        (*targetByte)=(*targetByte) | temp;
    }
    else//set to 0
    {
        temp=254;
        int i;
        printk("_setBitmap set to 0 bitOffset: %u",bitOffset);
        for(i=0;i<bitOffset;i++)
        {
            temp=(temp<<1)+1;
        }
        printk("_setBitmap temp: %d",(int)temp);
        (*targetByte)=(*targetByte) & temp;
    }
    printk("_setBitmap target Byte after: %d",(int)(*targetByte));
}
/**
 * @description: find first free bit in bit map
 * @param {unsigned char*} -pointer of bitMap
 * @param {uint32_t} maxNum -max number of inodes
 * @return {uint32_t} return free bit index, if no free bit return 0
 */
static uint32_t _findFirstfree(unsigned char* bitMap,uint32_t maxNum)
{
    int i,j;
    unsigned char temp,mask;
    int maxByte=ceil(maxNum,8);
    printk("maxByte:%u",maxByte);
    for(i=0;i<maxByte;i++)
    {
        temp=bitMap[i];
        if(temp!=0)//have free bits
        {
            j=0;
            mask=1;
            printk("char:%d",(int)temp);
            while((mask & temp)==0) 
            {
                mask=mask<<1;
                j++;
            }
            printk("found bit:%u",i*8+j);
            return i*8+j;
        }
    }
    printk("not found");
    return 0;//no free bit
}
/**
 * @description: find a free inode and set its bit map to used 
 * @param {smfs_superBlock*} smfsSb
 * @return {uint32_t} inode index, 0 if no free inode
 */
static uint32_t getFreeinode(struct smfs_superBlock* smfsSb)
{
    uint32_t found = _findFirstfree(smfsSb->pInodebitmap,smfsSb->iInodenum);
    printk("free inode found %u\n",found);
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
static uint32_t releaseInode(struct smfs_superBlock* smfsSb,uint32_t index)
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
static uint32_t getFreedata(struct smfs_superBlock* smfsSb)
{
    uint32_t found = _findFirstfree(smfsSb->pDatabitmap,smfsSb->iDatablocks);
    printk("free data found %u\n",found);
    if(found)
    {
        _setBitmap(smfsSb->pDatabitmap,found,0);//set inode bit used
        return found;
    }
    return 0;
}
static uint32_t releaseData(struct smfs_superBlock* smfsSb,uint32_t index)
{
    if(index > smfsSb->iDatablocks) return 1;
    _setBitmap(smfsSb->pDatabitmap,index,1);
    return 0;
}
#endif